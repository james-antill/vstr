#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <socket_poll.h>

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
#include <sys/poll.h>
#include <sys/stat.h>

#define EX_UTILS_NO_USE_OPEN 1
#define EX_UTILS_NO_USE_INIT 1
#define EX_UTILS_NO_USE_EXIT 1
#include "ex_utils.h"

#define CONF_SERV_DEF_ADDR INADDR_ANY
#define CONF_SERV_DEF_PORT 8008

#define CONF_SERV_NAME "ex_httpd/0.1.1 (Vstr)"

#define CONF_PUBLIC_ONLY TRUE

#include "ex_httpd_err_codes.h"

#define CONF_SOCK_LISTEN_NUM 512

#define CONF_BUF_SZ 128

#define CONF_MAXSIZE (1024 * 1024 * 4)

#include <assert.h>
#define ASSERT_NOT_REACHED() assert(FALSE)

#define TRUE  1
#define FALSE 0

#define HTTP_REQ_TYPE(end, T) do {                                      \
      if ((out->len >= strlen(end)) &&                                  \
          vstr_cmp_case_eod_cstr_eq(out, 1, out->len, end))             \
        http_req_content_type = (T);                                    \
      else { }                                                          \
} while (FALSE)

static unsigned int srv_sock_bind(long addr, short int port)
{
  unsigned int server_ind = 0;
  struct sockaddr_in socket_bind_in;
  int fd = -1;
  int sock_opt_t = TRUE;
  
  memset(&socket_bind_in, 0, sizeof(struct sockaddr_in));
  
  socket_bind_in.sin_family = AF_INET;
  socket_bind_in.sin_addr.s_addr = htonl(addr);
  socket_bind_in.sin_port = htons(port);
  
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    err(EXIT_FAILURE, "socket");

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt_t, sizeof(int)) == -1)
    err(EXIT_FAILURE, "setsocketopt(SO_REUSEADDR)");
  
  if (fcntl(fd, F_SETFD, TRUE) == -1)
    err(EXIT_FAILURE, "fcntl(F_SETFD, TRUE)");

  if (!io_fd_set_o_nonblock(fd))
    err(EXIT_FAILURE, "fcntl(F_SETFL, O_NONBLOCK)");

  if (bind(fd, (struct sockaddr *) &socket_bind_in,
           sizeof(struct sockaddr_in)) == -1)
    err(EXIT_FAILURE, "bind()");

  if (listen(fd, CONF_SOCK_LISTEN_NUM) == -1)
    err(EXIT_FAILURE, "listen(%d)", CONF_SOCK_LISTEN_NUM);
  
  server_ind = socket_poll_add(fd);
  if (!server_ind)
    errno = ENOMEM, err(EXIT_FAILURE, "poll_add()");

  SOCKET_POLL_INDICATOR(server_ind)->events |= POLLIN;

  return (server_ind);
}

static Vstr_base *serr = NULL;

static void usage(const char *program_name, int ret)
{
  vstr_add_fmt(serr, serr->len, " Format: %s <dir> [port]\n",
               program_name);
  while (serr->len &&
         vstr_sc_write_fd(serr, 1, serr->len,
                          (!ret) ? STDOUT_FILENO : STDERR_FILENO, NULL))
  { /* nothing */ }
  
  exit (ret);
}

static void serv_init(Vstr_base **s1, Vstr_base **s2)
{
  Vstr_conf *conf = NULL;

  if (!vstr_init()) /* init the library */
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (!(conf = vstr_make_conf()))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, CONF_BUF_SZ))
    warnx("Couldn't alter node size to match block size");
  if (!vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, CONF_BUF_SZ))
    warnx("Couldn't alter node size to match block size");

  if (vstr_make_spare_nodes(NULL, VSTR_TYPE_NODE_BUF, 32) != 32)
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  if (vstr_make_spare_nodes(conf, VSTR_TYPE_NODE_BUF, 32) != 32)
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  *s1  = vstr_make_base(NULL);
  *s2  = vstr_make_base(NULL);
  serr = vstr_make_base(conf);
  if (!s1 || !s2 || !serr)
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  vstr_free_conf(conf);
}


static void req_split(Vstr_sects *sects, Vstr_base *s1)
{
  size_t el = 0;
  const char *line_end_marker = "\r\n";
  size_t pos = 1;
  size_t len = s1->len;
  
  while ((el = vstr_srch_cstr_buf_fwd(s1, pos, len, line_end_marker)))
  {
    size_t ll = el - pos;
    
    if (!ll)
      return; /* end of headers */
    
    vstr_sects_add(sects, pos, ll);
    
    ll  += strlen(line_end_marker);
    pos += ll;
    len -= ll;
  }
  
  sects->num = 0;
}

static const char *serv_strftime(time_t val)
{
  static char ret[4096];
  struct tm *tm_val = gmtime(&val);
  
  if (!tm_val)
    err(EXIT_FAILURE, "gmtime");
      
  strftime(ret, sizeof(ret), "%a, %d %b %Y %T GMT", tm_val);

  return (ret);
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
  add_header_cstr(out, out->len, "Date", serv_strftime(time(NULL)));
  add_header_cstr(out, out->len, "Server", CONF_SERV_NAME);
  /*  add_header_cstr(out, out->len, "Connection", "close"); */
}



int main(int argc, char *argv[])
{
  long  addr = CONF_SERV_DEF_ADDR;
  short port = CONF_SERV_DEF_PORT;
  unsigned int ind = 0;
  Vstr_base *s1 = NULL;
  Vstr_base *s2 = NULL;
  const char *serv_dir = NULL;
  
  serv_init(&s1, &s2);
  
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

  ind = srv_sock_bind(addr, port);

  while (socket_poll_update_all(-1) != -1)
  {
    struct sockaddr_storage s_addr_store;
    struct sockaddr_in *si_addr = (struct sockaddr_in *)&s_addr_store;
    struct sockaddr *s_addr = (struct sockaddr *)&s_addr_store;
    socklen_t addr_len = sizeof(s_addr_store);
    int con_fd = -1;
    unsigned int ern = VSTR_TYPE_SC_READ_FD_ERR_NONE;
    unsigned int http_error_code = 0;
    const char *http_error_msg = "";
    Vstr_base *out = vstr_make_base(NULL);
    Vstr_sects *sects = vstr_sects_make(8);
    
    if (!out || !sects)
      continue;

    con_fd = accept(SOCKET_POLL_INDICATOR(ind)->fd, s_addr, &addr_len);
    if (con_fd == -1)
      continue;

    if (si_addr->sin_family != AF_INET) /* FIXME: ipv6 */
    {
      close(con_fd);
      continue;
    }
    
    if (!io_fd_set_o_nonblock(con_fd))
    {
      warn("fcntl(F_SETFL, O_NONBLOCK)");
      close(con_fd);
      continue;
    }
    
    while ((out->len <= CONF_MAXSIZE) && (ern == VSTR_TYPE_SC_READ_FD_ERR_NONE))
    {
      vstr_sc_read_iov_fd(out, out->len, con_fd, 4, 32, &ern);

      switch (ern)
      {
        case VSTR_TYPE_SC_READ_FD_ERR_NONE:
          if (vstr_srch_cstr_buf_fwd(out, 1, out->len, "\r\n\r\n"))
            goto con_fin;
          break;
        case VSTR_TYPE_SC_READ_FD_ERR_MEM:
          err(EXIT_FAILURE, __func__);
          
        case VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO:
          if (errno == EAGAIN)
          {
            io_block(con_fd, -1);
            ern = VSTR_TYPE_SC_READ_FD_ERR_NONE;
            continue;
          }
          warn("read_iov() = %d", ern);
          /* FALL THROUGH */
          
        case VSTR_TYPE_SC_READ_FD_ERR_EOF:
          goto con_fin;
          
        default: /* unknown */
          err(EXIT_FAILURE, "read_iov() = %d", ern);
      }
    }
   con_fin:
    if (out->conf->malloc_bad || (out->len > CONF_MAXSIZE))
      goto con_cleanup;

    req_split(sects, out);
    if (!sects->num || sects->malloc_bad)
       goto con_cleanup;
    else
    {
      size_t op_pos = VSTR_SECTS_NUM(sects, 1)->pos;
      size_t op_len = VSTR_SECTS_NUM(sects, 1)->len;
      VSTR_SECTS_DECL(req_path, 3);
      int head_op = FALSE;
      
      VSTR_SECTS_DECL_INIT(req_path);
      
      if (vstr_split_cstr_buf(out, op_pos, op_len, " ", req_path, 4, 0) != 3)
      {
        HTTPD_ERR(400, head_op);
        goto con_fin_error_code;
      }
        
      op_pos = VSTR_SECTS_NUM(req_path, 1)->pos;
      op_len = VSTR_SECTS_NUM(req_path, 1)->len;
      
      if (0) { }
      else if (vstr_cmp_case_cstr_eq(out, op_pos, op_len, "get") ||
               (head_op = vstr_cmp_case_cstr_eq(out, op_pos, op_len, "head")))
      {
        const char *fname = NULL;
        int f_fd = -1;
        struct stat f_stat;  
        const char *http_req_content_type = "application/octet-stream";
        
        op_pos = VSTR_SECTS_NUM(req_path, 3)->pos;
        op_len = VSTR_SECTS_NUM(req_path, 3)->len;
        if ((op_len <= strlen("http/1.")) ||
            !vstr_cmp_case_bod_cstr_eq(out, op_pos, op_len, "http/1."))
        {
          HTTPD_ERR(505, head_op);
          goto con_fin_error_code;
        }
        
        op_pos = VSTR_SECTS_NUM(req_path, 2)->pos;
        op_len = VSTR_SECTS_NUM(req_path, 2)->len;

        vstr_del(out, 1, op_pos - 1);
        vstr_sc_reduce(out, 1, out->len, out->len - op_len);

        if (!vstr_conv_decode_uri(out, 1, out->len))
          goto con_cleanup;
        
        if (vstr_srch_chr_fwd(out, 1, out->len, 0))
        {
          HTTPD_ERR(400, head_op);
          goto con_fin_error_code;
        }
        
        if ((vstr_export_chr(out, 1) != '/') ||
             vstr_srch_cstr_buf_fwd(out, 1, out->len, "/../"))
        {
          HTTPD_ERR(403, head_op);
          goto con_fin_error_code;
        }
        
        vstr_del(out, 1, 1);

        if (!out->len)
          vstr_add_cstr_ptr(out, out->len, "index.html");

       retry_req:
        {
          HTTP_REQ_TYPE("README", "text/plain");
          HTTP_REQ_TYPE("Makefile", "text/plain");
          HTTP_REQ_TYPE(".txt", "text/plain");
          HTTP_REQ_TYPE(".c", "text/plain");
          HTTP_REQ_TYPE(".h", "text/plain");
          HTTP_REQ_TYPE(".css", "text/css");
          HTTP_REQ_TYPE(".html", "text/html");
        
          if (!(fname = vstr_export_cstr_ptr(out, 1, out->len)))
            goto con_cleanup;
        }
        
        if (out->conf->malloc_bad)
        {
          close(f_fd);
          goto con_cleanup;
        }

        /* simple logger */
        warnx("REQ %s @[%s] from[%s] t[%s]: %s", head_op ? "HEAD" : "GET",
              serv_strftime(time(NULL)), inet_ntoa(si_addr->sin_addr),
              http_req_content_type, fname);
        if ((f_fd = open64(fname, O_RDONLY | O_NOCTTY)) == -1)
        {
          if (0) { }
          else if (errno == EISDIR)
          {
            vstr_add_cstr_ptr(out, out->len, "/index.html");
            goto retry_req;
          }
          else if (errno == EACCES)
            HTTPD_ERR(403, head_op);
          else if ((errno == ENOENT) ||
                   (errno == ENODEV) ||
                   (errno == ENXIO) ||
                   (errno == ELOOP) ||
                   FALSE)
            HTTPD_ERR(404, head_op);
          else if (errno == ENAMETOOLONG)
            HTTPD_ERR(414, head_op);            
          else
            HTTPD_ERR(500, head_op);
            
          goto con_fin_error_code;
        }
        if (fstat(f_fd, &f_stat) == -1)
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
          close(f_fd);
          vstr_add_cstr_ptr(out, out->len, "/index.html");
          goto retry_req;
        }

        fname = NULL;
        vstr_del(out, 1, out->len);

        vstr_add_fmt(out, out->len, "%s %03d %s\r\n",
                     "HTTP/1.0", 200, "OK");

        add_def_headers(out);
        add_header_cstr(out, out->len, "Last-Modified",
                        serv_strftime(f_stat.st_mtime));
        add_header_cstr(out, out->len, "Content-Type",
                        http_req_content_type);
        add_header_uintmax(out, out->len, "Content-Length", f_stat.st_size);
        vstr_add_cstr_ptr(out, out->len, "\r\n");

        if (out->conf->malloc_bad)
        {
          close(f_fd);
          goto con_cleanup;
        }
        
        while (!head_op)
        {
          int io_w_state = IO_OK;
          int io_r_state = io_get(out, f_fd);
          
          if (io_r_state == IO_EOF)
            break;
          
          io_w_state = io_put(out, con_fd);
          
          io_limit(io_r_state, f_fd, io_w_state, con_fd, out);    
        }
        io_put_all(out, con_fd);
        
        close(f_fd);
        goto con_cleanup;

       con_close_fin_error_code:
        close(f_fd);
        goto con_fin_error_code;        
      }
      else if (FALSE && vstr_cmp_case_cstr_eq(out, op_pos, op_len, "post"))
      {
      }
      else if (FALSE && vstr_cmp_case_cstr_eq(out, op_pos, op_len, "head"))
      {
      }
      else if (FALSE && vstr_cmp_case_cstr_eq(out, op_pos, op_len, "options"))
      {
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
    vstr_del(out, 1, out->len);
    vstr_add_fmt(out, out->len, "%s %03u %s\r\n\r\r",
                 "HTTP/1.0", http_error_code, "BAD");
    add_def_headers(out);
    add_header_cstr(out, out->len, "Last-Modified", serv_strftime(0));
    add_header_cstr(out, out->len, "Content-Type", "text/html");
    add_header_uintmax(out, out->len, "Content-Length", strlen(http_error_msg));
    vstr_add_cstr_ptr(out, out->len, "\r\n\r\r");
    vstr_add_cstr_ptr(out, out->len, http_error_msg);

    if (!out->conf->malloc_bad)
      io_put_all(out, con_fd);
    
   con_cleanup:
    out->conf->malloc_bad = FALSE;
    
    vstr_free_base(out);
    vstr_sects_free(sects);
    close(con_fd);
  }
  socket_poll_del(ind);

  vstr_free_base(s1);
  vstr_free_base(s2);
  vstr_free_base(serr);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
