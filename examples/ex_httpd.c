#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <socket_poll.h>
#include <timer_q.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>

/* is the data is less than this, queue instead of hitting write */
#define CL_MAX_WAIT_SEND 16

#include "evnt.h"

#define EX_UTILS_NO_USE_OPEN 1
#define EX_UTILS_NO_USE_INIT 1
#define EX_UTILS_NO_USE_EXIT 1
#define EX_UTILS_NO_USE_BLOCK 1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_PUT 1
#include "ex_utils.h"

#define CONF_SERV_DEF_ADDR NULL
#define CONF_SERV_DEF_PORT 8008

#define CONF_SERV_NAME "jhttpd/0.1.1 (Vstr)"

#define CONF_PUBLIC_ONLY TRUE

#include "ex_httpd_err_codes.h"
#include "ex_httpd_mime_types.h"

#define CONF_SOCK_LISTEN_NUM 512

#define CONF_BUF_SZ 128

#define CONF_INPUT_MAXSZ (1024 * 1024 * 4)
#define CONF_OUTPUT_SZ   (1024 * 128)

#include <assert.h>
#define ASSERT_NOT_REACHED() assert(FALSE)

#define CSTREQ(x, y) (strcmp(x, y) == 0)

#define TRUE  1
#define FALSE 0

struct con
{
 struct Evnt ev[1];
 int f_fd;
 struct sockaddr *sa;
};

static Timer_q_base *cl_timeout_base = NULL;

static struct Evnt *acpt_evnt = NULL;

static unsigned int server_clients_count = 0;

static unsigned int client_timeout = (2 * 60); /* 2 minutes */

static unsigned int cl_dbg_opt = FALSE;
static Vstr_base *cl_dbg_log = NULL;

static void dbg(const char *fmt, ... )
   VSTR__COMPILE_ATTR_FMT(1, 2);
static void dbg(const char *fmt, ... )
{
  Vstr_base *dlog = cl_dbg_log;
  va_list ap;

  if (!cl_dbg_opt)
    return;
  
  va_start(ap, fmt);
  vstr_add_vfmt(dlog, dlog->len, fmt, ap);
  va_end(ap);

  /* Only flush entire lines... */
  while (vstr_srch_chr_fwd(dlog, 1, dlog->len, '\n'))
  {
    if (!vstr_sc_write_fd(dlog, 1, dlog->len, STDERR_FILENO, NULL) &&
        (errno != EAGAIN))
      err(EXIT_FAILURE, "dbg");
  }
}


static void usage(const char *program_name, int ret)
{
  Vstr_base *serr = NULL;

  if ((serr = vstr_make_base(NULL)))
  {
    vstr_add_fmt(serr, serr->len, " Format: %s <dir> [port]\n",
                 program_name);
    while (serr->len &&
           vstr_sc_write_fd(serr, 1, serr->len,
                            (!ret) ? STDOUT_FILENO : STDERR_FILENO, NULL))
    { /* nothing */ }
  }
  
  exit (ret);
}

static void cl_signals(void)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    err(EXIT_FAILURE, __func__);
  
  /* don't use SA_RESTART ... */
  sa.sa_flags = 0;
  /* ignore it... we don't have a use for it */
  sa.sa_handler = SIG_IGN;
  
  if (sigaction(SIGPIPE, &sa, NULL) == -1)
    err(EXIT_FAILURE, __func__);
}

static void serv_init(void)
{
  if (!vstr_init()) /* init the library */
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  cl_signals();
}

static void req_split(Vstr_sects *sects,
                      Vstr_base *s1, size_t pos, size_t len,
                      int *ver_0_9)
{
  size_t el = 0;
  const char *const line_end_marker = "\r\n";
  size_t hpos = pos;
  unsigned int orig_num = sects->num;

  /* split request */
  if (!(el = vstr_srch_chr_fwd(s1, pos, len, ' ')))
    return;
  vstr_sects_add(sects, pos, el - pos);
  len -= (el - pos) + 1;
  pos += (el - pos) + 1;

  if (!len)
    goto fail_req;

  if (vstr_export_chr(s1, pos) != '/') /* Allow http://.../... for proxies */
    goto fail_req;
  
  if (!(el = vstr_srch_chr_fwd(s1, pos, len, ' ')))
  {
    if ((len < 2) || !vstr_cmp_cstr_eq(s1, len - 1, 2, "\r\n"))
      goto fail_req;
    *ver_0_9 = TRUE;
    vstr_sects_add(sects, pos, len - 2);
    return;
  }
  vstr_sects_add(sects, pos, el - pos);
  len -= (el - pos) + 1;
  pos += (el - pos) + 1;

  if (!(el = vstr_srch_cstr_buf_fwd(s1, pos, len, line_end_marker)))
    goto fail_req;
  vstr_sects_add(sects, pos, el - pos);
  len -= (el - pos) + strlen(line_end_marker);
  pos += (el - pos) + strlen(line_end_marker);

  if (len < strlen(line_end_marker))
    goto fail_req;
  
  if (vstr_cmp_cstr_eq(s1, pos, strlen(line_end_marker), line_end_marker))
    return; /* end of headers */
  
  /* split headers */
  hpos = pos;
  while ((el = vstr_srch_cstr_buf_fwd(s1, pos, len, line_end_marker)))
  {
    len -= (el - pos) + strlen(line_end_marker);
    pos += (el - pos) + strlen(line_end_marker);
    
    if (len)
    {
      char chr = vstr_export_chr(s1, pos);

      if (chr == ' ' || chr == '\t')
        continue;

      vstr_sects_add(sects, hpos, el - hpos);
    
      if ((len > 1) &&
          vstr_cmp_cstr_eq(s1, pos, strlen(line_end_marker), line_end_marker))
        return; /* end of headers */

      hpos = pos;
    }
  }

 fail_req:
  sects->num = orig_num;
}

/* use macro so we pass a constant fmt string for gcc's checker */
#define SERV__STRFTIME(val, fmt) do {           \
      static char ret[4096];                    \
      struct tm *tm_val = gmtime(&val);         \
                                                \
      if (!tm_val)                              \
        err(EXIT_FAILURE, "gmtime");            \
                                                \
      strftime(ret, sizeof(ret), fmt, tm_val);  \
                                                \
      return (ret);                             \
    } while (FALSE)

static const char *serv_date_rfc1123(time_t val)
{
  SERV__STRFTIME(val, "%a, %d %b %Y %T GMT");
}
static const char *serv_date_rfc850(time_t val)
{
  SERV__STRFTIME(val, "%A, %d-%b-%y %T GMT");
}
static const char *serv_date_asctime(time_t val)
{
  SERV__STRFTIME(val, "%a %b %e %T %Y");
}

static void add_header_cstr(Vstr_base *out, size_t len,
                            const char *hdr, const char *data)
{
  vstr_add_fmt(out, len, "%s: %s\r\n", hdr, data);
}

static void add_header_uintmax(Vstr_base *out, size_t len,
                               const char *hdr, uintmax_t data)
{
  vstr_add_fmt(out, len, "%s: %ju\r\n", hdr, data);
}

static void add_def_headers(Vstr_base *out)
{
  add_header_cstr(out, out->len, "Date", serv_date_rfc1123(time(NULL)));
  add_header_cstr(out, out->len, "Server", CONF_SERV_NAME);
  /*  add_header_cstr(out, out->len, "Connection", "close"); */
}

static int serv_recv(struct con *con)
{
  int ret = evnt_recv(con->ev);
  Vstr_base *data = con->ev->io_r;
  Vstr_base *out  = con->ev->io_w;
  size_t h_end = 0;
  unsigned int http_error_code = 0;
  const char *http_error_msg = "";
  Vstr_sects *sects = NULL;
  char *req_head_if_mod_since = NULL;
  int ver_0_9 = FALSE;

  if (ret)
  {
    if (!(h_end = vstr_srch_cstr_buf_fwd(data, 1, data->len, "\r\n\r\n")))
    {
      if (data->len > CONF_INPUT_MAXSZ)
        goto con_cleanup;
      
      return (TRUE);
    }
  }
  else
    h_end = vstr_srch_cstr_buf_fwd(data, 1, data->len, "\r\n\r\n");

  SOCKET_POLL_INDICATOR(con->ev->ind)->events &= ~POLLIN;

  if (!h_end)
    goto con_cleanup;

  if (!(sects = vstr_sects_make(8)))
    goto con_cleanup;
  
  req_split(sects, data, 1, h_end + 3, &ver_0_9);
  if (sects->malloc_bad)
    goto con_cleanup;
  else if (sects->num < 2)
  {
    HTTPD_ERR(400, FALSE);
    goto con_fin_error_code;
  }
  else
  {
    size_t op_pos = 0;
    size_t op_len = 0;
    int head_op = FALSE;
    
    assert(((sects->num >= 3) && !ver_0_9) || (sects->num == 2));
    
    op_pos = VSTR_SECTS_NUM(sects, 1)->pos;
    op_len = VSTR_SECTS_NUM(sects, 1)->len;
    
    if (0) { }
    else if (vstr_cmp_cstr_eq(data, op_pos, op_len, "GET") ||
             (!ver_0_9 &&
              (head_op = vstr_cmp_cstr_eq(data, op_pos, op_len, "HEAD"))))
    {
      const char *fname = NULL;
      struct stat f_stat;  
      const char *http_req_content_type = "application/octet-stream";
      
      if (!ver_0_9)
      {
        unsigned int num = 3;
        
        op_pos = VSTR_SECTS_NUM(sects, 3)->pos;
        op_len = VSTR_SECTS_NUM(sects, 3)->len;
        if (!vstr_cmp_cstr_eq(data, op_pos, op_len, "HTTP/0.9") &&
            !vstr_cmp_cstr_eq(data, op_pos, op_len, "HTTP/1.0") &&
            !vstr_cmp_cstr_eq(data, op_pos, op_len, "HTTP/1.1"))
        {
          HTTPD_ERR(505, head_op);
          goto con_fin_error_code;
        }
        
        while (++num <= sects->num)
        {
          op_pos = VSTR_SECTS_NUM(sects, 3)->pos;
          op_len = VSTR_SECTS_NUM(sects, 3)->len;
          
          if (op_len > strlen("If-Modified-Since: ") &&
              vstr_cmp_bod_cstr_eq(data, op_pos, op_len,
                                   "If-Modified-Since: "))
          {
            char *tmp = NULL;
            
            op_pos += strlen("If-Modified-Since: ");
            op_len -= strlen("If-Modified-Since: ");
            if (!(tmp = vstr_export_cstr_malloc(data, op_pos, op_len)))
              goto con_cleanup;
            req_head_if_mod_since = tmp;
          }
        }
      }
      
      op_pos = VSTR_SECTS_NUM(sects, 2)->pos;
      op_len = VSTR_SECTS_NUM(sects, 2)->len;
      
      vstr_del(data, 1, op_pos - 1);
      vstr_sc_reduce(data, 1, data->len, data->len - op_len);
      
      if (!vstr_conv_decode_uri(data, 1, data->len))
        goto con_cleanup;
      
      if (vstr_srch_chr_fwd(data, 1, data->len, 0))
      {
        HTTPD_ERR(400, head_op);
        goto con_fin_error_code;
        }
      
      /* req_split() will give 400 otherwise */
      assert(vstr_export_chr(data, 1) == '/');
      
      if ((vstr_export_chr(data, 1) != '/') ||
          vstr_srch_cstr_buf_fwd(data, 1, data->len, "/../"))
      {
        HTTPD_ERR(403, head_op);
        goto con_fin_error_code;
      }
      
      vstr_del(data, 1, 1);
      
      if (!data->len)
        vstr_add_cstr_ptr(data, data->len, "index.html");
      
     retry_req:
      HTTP_REQ_MIME_TYPE(data, http_req_content_type);
      
      if (!(fname = vstr_export_cstr_ptr(data, 1, data->len)))
        goto con_cleanup;
      
      if (data->conf->malloc_bad)
        goto con_close_cleanup;
      
      /* simple logger */
      printf("REQ %s @[%s] from[%s] t[%s]: %s\n", head_op ? "HEAD" : "GET",
             serv_date_rfc1123(time(NULL)),
             inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr),
             http_req_content_type, fname);
      fflush(NULL);
      
      if ((con->f_fd = open64(fname, O_RDONLY | O_NOCTTY)) == -1)
      {
        if (0) { }
        else if (errno == EISDIR)
        {
          vstr_add_cstr_ptr(data, data->len, "/index.html");
          goto retry_req;
        }
        else if (errno == EACCES)
          HTTPD_ERR(403, head_op);
        else if ((errno == ENOENT) ||
                 (errno == ENODEV) ||
                 (errno == ENXIO) ||
                 (errno == ELOOP) ||
                 (errno == ENAMETOOLONG) ||
                 FALSE)
          HTTPD_ERR(404, head_op);
        /*
         * else if (errno == ENAMETOOLONG)
         *   HTTPD_ERR(414, head_op);
         */
        else
          HTTPD_ERR(500, head_op);
        
        goto con_fin_error_code;
      }
      if (fstat(con->f_fd, &f_stat) == -1)
      {
        HTTPD_ERR(500, head_op);
        goto con_close_fin_error_code;
      }
      if (CONF_PUBLIC_ONLY && !(f_stat.st_mode & S_IROTH))
      {
        HTTPD_ERR(403, head_op);
        goto con_close_fin_error_code;
      }
      
      if (S_ISDIR(f_stat.st_mode))
      {
        close(con->f_fd);
        con->f_fd = -1;
        vstr_add_cstr_ptr(data, data->len, "/index.html");
        goto retry_req;
      }
      
      fname = NULL;
      vstr_del(data, 1, data->len);
      
      if (!ver_0_9)
      {
        const char *tmp = req_head_if_mod_since;
        
        /* assumes time doesn't go backwards */
        if (tmp &&
            (CSTREQ(tmp, serv_date_rfc1123(f_stat.st_mtime)) ||
             CSTREQ(tmp, serv_date_rfc850(f_stat.st_mtime))  ||
             CSTREQ(tmp, serv_date_asctime(f_stat.st_mtime))))
          head_op = TRUE;
        
        vstr_add_fmt(out, out->len, "%s %03d %s\r\n",
                     "HTTP/1.0", 200, "OK");
        
        add_def_headers(out);
        add_header_cstr(out, out->len, "Last-Modified",
                        serv_date_rfc1123(f_stat.st_mtime));
        add_header_cstr(out, out->len, "Content-Type",
                        http_req_content_type);
        add_header_uintmax(out, out->len, "Content-Length", f_stat.st_size);
        vstr_add_cstr_ptr(out, out->len, "\r\n");
        
        if (out->conf->malloc_bad)
          goto con_close_cleanup;
      }
      
      if (!head_op)
        io_fd_set_o_nonblock(con->f_fd);
      else
      {
        close(con->f_fd);
        con->f_fd = -1;
      }
      
      goto con_do_write;
      
     con_close_cleanup:
      close(con->f_fd);
      goto con_cleanup;
      
     con_close_fin_error_code:
      close(con->f_fd);
      con->f_fd = -1;
      goto con_fin_error_code;
    }
    else if (FALSE &&
             (!ver_0_9 && vstr_cmp_cstr_eq(data, op_pos, op_len, "POST")))
    {
      /* do 405 instead ? */
    }
    else
    {
      http_error_msg  = CONF_MSG_RET_501;
      http_error_code = 501;
      goto con_fin_error_code;
    }
  }
  ASSERT_NOT_REACHED();
  
 con_fin_error_code:
  vstr_del(data, 1, data->len);
  if (!ver_0_9)
  {
    vstr_add_fmt(out, out->len, "%s %03u %s\r\n\r\r",
                 "HTTP/1.0", http_error_code, "BAD");
    add_def_headers(out);
    add_header_cstr(out, out->len, "Last-Modified", serv_date_rfc1123(0));
    add_header_cstr(out, out->len, "Content-Type", "text/html");
    add_header_uintmax(out, out->len, "Content-Length",
                       strlen(http_error_msg));
    vstr_add_cstr_ptr(out, out->len, "\r\n\r\r");
  }
  
  vstr_add_cstr_ptr(out, out->len, http_error_msg);

 con_do_write:
  if (evnt_send_add(con->ev, FALSE, CL_MAX_WAIT_SEND))
  {
    free(req_head_if_mod_since);
    vstr_sects_free(sects);

    return (TRUE);
  }
  
 con_cleanup:
  free(req_head_if_mod_since);
  data->conf->malloc_bad = FALSE;
  out->conf->malloc_bad  = FALSE;
  
  vstr_sects_free(sects);
  
  evnt_close(con->ev);
    
  return (FALSE);
}

static int serv_cb_func_recv(struct Evnt *evnt)
{
  return (serv_recv((struct con *)evnt));
}

static int serv_cb_func_send(struct Evnt *evnt)
{
  struct con *con = (struct con *)evnt;

  /* FIXME: sendfile too */
  if (!evnt_send(evnt))
    return (FALSE);
  
  if (con->f_fd == -1)
    return (con->ev->io_w->len != 0);
  
  while (io_get(con->ev->io_w, con->f_fd) != IO_EOF)
  {
    evnt_send_add(con->ev, TRUE, CL_MAX_WAIT_SEND);
    
    if (con->ev->io_w->len >= CONF_OUTPUT_SZ)
      return (TRUE);
  }
  
  close(con->f_fd);
  con->f_fd = -1;
    
  if (!con->ev->io_w->len)
    return (FALSE);
  
  return (TRUE);
}

static void serv_cb_func_free(struct Evnt *evnt)
{
  struct con *con = (struct con *)evnt;

  if (con->f_fd != -1)
  {
    close(con->f_fd);
    con->f_fd = -1;
  }
  free(con->sa);
  
  free(con);

  --server_clients_count;
}

static void serv_cb_func_acpt_free(struct Evnt *evnt)
{
  free(evnt);

  ASSERT(acpt_evnt == evnt);
  
  acpt_evnt = NULL;
}

static struct Evnt *serv_cb_func_accept(int fd,
                                        struct sockaddr *sa, socklen_t len)
{
  struct con *con = malloc(sizeof(struct con));
  struct timeval tv;

  if (!con || !evnt_make_acpt(con->ev, fd, sa, len))
    goto make_acpt_fail;

  tv = con->ev->ctime;
  TIMER_Q_TIMEVAL_ADD_SECS(&tv, 0, client_timeout);
  
  if (!(con->ev->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                         TIMER_Q_FLAG_NODE_DEFAULT)))
    goto timer_add_fail;

  if (!(con->sa = malloc(len)))
    goto malloc_sa_fail;
  memcpy(con->sa, sa, len);
  
  /* simple logger */
  printf("CONNECT @[%s] from[%s]\n",
         serv_date_rfc1123(time(NULL)),
         inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr));
  fflush(NULL);
  
  con->ev->cbs->cb_func_recv = serv_cb_func_recv;
  con->ev->cbs->cb_func_send = serv_cb_func_send;
  con->ev->cbs->cb_func_free = serv_cb_func_free;

  con->f_fd = -1;
  
  ++server_clients_count;
  
  return (con->ev);

 malloc_sa_fail:
  timer_q_quick_del_node(con->ev->tm_o);
 timer_add_fail:
  errno = ENOMEM, warn(__func__);
  evnt_free(con->ev);
 make_acpt_fail:
  free(con);
  return (NULL);
}

static void cl_timer_con(int type, void *data)
{
  struct con *con = NULL;
  struct timeval tv;
  unsigned long diff = 0;
  
  if (type == TIMER_Q_TYPE_CALL_DEL)
    return;

  con = data;

  gettimeofday(&tv, NULL);
  diff = timer_q_timeval_udiff_secs(&tv, &con->ev->mtime);
  if (diff > client_timeout)
  {
    dbg("timeout = %p (%lu, %lu)\n", con, diff, (unsigned long)client_timeout);
    evnt_close(con->ev);
    return;
  }
  
  if (type == TIMER_Q_TYPE_CALL_RUN_ALL)
    return;
  
  TIMER_Q_TIMEVAL_ADD_SECS(&tv, (client_timeout - diff) + 1, 0);
  if (!(con->ev->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                         TIMER_Q_FLAG_NODE_DEFAULT)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
}

int main(int argc, char *argv[])
{
  const char *addr = CONF_SERV_DEF_ADDR;
  short port = CONF_SERV_DEF_PORT;
  
  serv_init();
  
  switch (argc)
  {
    case 3:
      port = atoi(argv[2]);
    case 2:
      if (chdir(argv[1]) == -1)
        err(EXIT_FAILURE, "chdir(%s)", argv[1]);
      break;
      
    default:
      usage(argv[0], EXIT_FAILURE);
      break;
  }

  cl_timeout_base = timer_q_add_base(cl_timer_con, TIMER_Q_FLAG_BASE_DEFAULT);

  if (!cl_timeout_base)
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  if (!(acpt_evnt = malloc(sizeof(struct Evnt))))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  
  if (!evnt_make_bind_ipv4(acpt_evnt, addr, port))
    err(EXIT_FAILURE, "bind(%s:%hu)", addr, port);

  SOCKET_POLL_INDICATOR(acpt_evnt->ind)->events |= POLLIN;

  acpt_evnt->cbs->cb_func_accept = serv_cb_func_accept;
  acpt_evnt->cbs->cb_func_free   = serv_cb_func_acpt_free;

  while (acpt_evnt || server_clients_count)
  {
    int ready = evnt_poll();
    struct timeval tv;
    
    if ((ready == -1) && (errno != EINTR))
      err(EXIT_FAILURE, __func__);

    dbg("1 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_recv, q_none);
    evnt_scan_fds(ready, CL_MAX_WAIT_SEND);
    dbg("2 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_recv, q_none);
    evnt_scan_send_fds();
    dbg("3 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_recv, q_none);
    
    gettimeofday(&tv, NULL);
    timer_q_run_norm(&tv);

    dbg("4 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_recv, q_none);
    evnt_scan_send_fds();
  }
  dbg("E a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_recv, q_none);

  timer_q_del_base(cl_timeout_base);

  evnt_close_all();

  vstr_free_base(cl_dbg_log);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}