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
#include <getopt.h>
#include <syslog.h>

/* is the data is less than this, queue instead of hitting write */
#define CL_MAX_WAIT_SEND 16

#include "evnt.h"

#define EX_UTILS_NO_USE_OPEN  1
#define EX_UTILS_NO_USE_INIT  1
#define EX_UTILS_NO_USE_EXIT  1
#define EX_UTILS_NO_USE_BLOCK 1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_PUT   1
#include "ex_utils.h"

#define CONF_SERV_DEF_ADDR NULL
#define CONF_SERV_DEF_PORT   80

#define CONF_SERV_NAME "jhttpd/0.1.1 (Vstr)"

#define CONF_SERV_USE_VHOST 1

#define CONF_PUBLIC_ONLY TRUE

#include "ex_httpd_err_codes.h"
#include "ex_httpd_mime_types.h"

#define CONF_SOCK_LISTEN_NUM 512

#define CONF_BUF_SZ 128

#define CONF_INPUT_MAXSZ (1024 * 1024 * 4)
#define CONF_OUTPUT_SZ   (1024 * 128)

#include <assert.h>
#define ASSERT_NOT_REACHED() assert(FALSE)

/* is the cstr a prefix of the vstr */
#define VPREFIX(vstr, p, l, cstr)                    \
    (((l) >= strlen(cstr)) && vstr_cmp_bod_cstr_eq(vstr, p, l, cstr))
/* is the cstr a suffix of the vstr */
#define VSUFFIX(vstr, p, l, cstr)                    \
    (((l) >= strlen(cstr)) && vstr_cmp_eod_cstr_eq(vstr, p, l, cstr))

#define TRUE  1
#define FALSE 0

struct http_hdrs
{
 Vstr_sect_node hdr_ua[1]; /* logging headers */
 Vstr_sect_node hdr_referrer[1]; /* NOTE: fixed stupid spelling */

 Vstr_sect_node hdr_connection[1];
 Vstr_sect_node hdr_expect[1];
 Vstr_sect_node hdr_host[1];
 Vstr_sect_node hdr_if_match[1];
 Vstr_sect_node hdr_if_modified_since[1];
 Vstr_sect_node hdr_if_none_match[1];
 Vstr_sect_node hdr_if_range[1];
 Vstr_sect_node hdr_if_unmodified_since[1];
 Vstr_sect_node hdr_range[1];
};

struct con
{
 struct Evnt ev[1];
 int f_fd;
 struct sockaddr *sa;

 int parsed_method_ver_1_0;
 int keep_alive;
 
 struct http_hdrs http_hdrs[1];
};

#define CON_HDR_SET(con, h, p, l) do {                      \
      (con)-> http_hdrs -> hdr_ ## h ->pos = (p);           \
      (con)-> http_hdrs -> hdr_ ## h ->len = (l);           \
    } while (FALSE)

static Timer_q_base *cl_timeout_base = NULL;

static struct Evnt *acpt_evnt = NULL;

static unsigned int server_clients_count = 0;

static unsigned int client_timeout = (2 * 60); /* 2 minutes */

static const char *server_ipv4_address = CONF_SERV_DEF_ADDR;
static short server_port = CONF_SERV_DEF_PORT;

static unsigned int server_max_clients = 0;

static unsigned int server_max_header_sz = CONF_INPUT_MAXSZ;

static int server_daemon = FALSE;

static time_t server_beg = (time_t)-1;

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
  fprintf(ret ? stderr : stdout, "\n Format: %s [-dHhMnPtV] <dir>\n"
         " --daemon          - Become a daemon.\n"
         " --debug -d        - Enable/disable debug info.\n"
         " --host -H         - IPv4 address to bind (def: \"all\").\n"
         " --help -h         - Print this message.\n"
         " --max-clients -M  - Max clients allowed to connect (0 = no limit).\n"
         " --max-header-sz   - Max size of http header (0 = no limit).\n"
         " --nagle -n        - Toggle usage of nagle TCP option.\n"
         " --port -P         - Port to bind to.\n"
         " --timeout -t      - Timeout (usecs) for connections.\n"
         " --version -V      - Print the version string.\n",
          program_name);
  
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

  /* no passing of con'f to evnt */
  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$') ||
      !vstr_sc_fmt_add_all(NULL))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!(cl_dbg_log = vstr_make_base(NULL)))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  cl_signals();
}

static void req_split_method(Vstr_sects *sects,
                             Vstr_base *s1, size_t pos, size_t len,
                             int *ver_0_9)
{
  size_t el = 0;
  unsigned int orig_num = sects->num;

  /* split request */
  if (!(el = vstr_srch_chr_fwd(s1, pos, len, ' ')))
    return;
  vstr_sects_add(sects, pos, el - pos);
  len -= (el - pos) + 1;
  pos += (el - pos) + 1;

  if (!len)
    goto fail_req;

  if ((vstr_export_chr(s1, pos) != '/') && !VPREFIX(s1, pos, len, "http://"))
    goto fail_req;
  
  if (!(el = vstr_srch_chr_fwd(s1, pos, len, ' ')))
  {
    if (!VSUFFIX(s1, pos, len, "\r\n"))
      goto fail_req;
    *ver_0_9 = TRUE;
    vstr_sects_add(sects, pos, len - strlen("\r\n"));
    dbg("DBG: Method: ${vstr:%p%zu%zu%u} ${vstr:%p%zu%zu%u}\n",
        s1, VSTR_SECTS_NUM(sects, 1)->pos, VSTR_SECTS_NUM(sects, 1)->len,
        VSTR_TYPE_ADD_BUF_PTR,
        s1, VSTR_SECTS_NUM(sects, 2)->pos, VSTR_SECTS_NUM(sects, 2)->len,
        VSTR_TYPE_ADD_BUF_PTR);
    return;
  }
  vstr_sects_add(sects, pos, el - pos);
  len -= (el - pos) + strlen(" ");
  pos += (el - pos) + strlen(" ");
  
  if (!(el = vstr_srch_cstr_buf_fwd(s1, pos, len, "\r\n")))
    goto fail_req;
  vstr_sects_add(sects, pos, el - pos);

  return;
  
 fail_req:
  sects->num = orig_num;
}

static void req_split_headers(Vstr_sects *sects,
                              Vstr_base *s1, size_t pos, size_t len)
{
  size_t el = 0;
  size_t hpos = 0;
  unsigned int orig_num = sects->num;

  ASSERT(sects->num >= 3);
  
  dbg("DBG: Method: ${vstr:%p%zu%zu%u} ${vstr:%p%zu%zu%u} ${vstr:%p%zu%zu%u}\n",
      s1, VSTR_SECTS_NUM(sects, 1)->pos, VSTR_SECTS_NUM(sects, 1)->len,
      VSTR_TYPE_ADD_BUF_PTR,
      s1, VSTR_SECTS_NUM(sects, 2)->pos, VSTR_SECTS_NUM(sects, 2)->len,
      VSTR_TYPE_ADD_BUF_PTR,
      s1, VSTR_SECTS_NUM(sects, 3)->pos, VSTR_SECTS_NUM(sects, 3)->len,
      VSTR_TYPE_ADD_BUF_PTR);
    
  /* skip first line */
  el = (VSTR_SECTS_NUM(sects, sects->num)->pos +
        VSTR_SECTS_NUM(sects, sects->num)->len);

  ASSERT(vstr_cmp_cstr_eq(s1, el, strlen("\r\n"), "\r\n"));
  
  len -= (el - pos) + strlen("\r\n");
  pos += (el - pos) + strlen("\r\n");
  
  if (VPREFIX(s1, pos, len, "\r\n"))
    return; /* end of headers */
  
  dbg("%s", "DBG: -- HEADERS --\n");
  
  /* split headers */
  hpos = pos;
  while ((el = vstr_srch_cstr_buf_fwd(s1, pos, len, "\r\n")))
  {
    char chr = 0;
    
    len -= (el - pos) + strlen("\r\n");
    pos += (el - pos) + strlen("\r\n");
    
    if (!len) /* headers must end with a "...\r\n\r\n" */
      break;
    
    chr = vstr_export_chr(s1, pos);
    if (chr == ' ' || chr == '\t')
      continue;

    vstr_sects_add(sects, hpos, el - hpos);
    dbg("${vstr:%p%zu%zu%u}\n",
        s1, hpos, el - hpos, VSTR_TYPE_ADD_BUF_PTR);

    if (VPREFIX(s1, pos, len, "\r\n"))
      return; /* found end of headers */
    
    hpos = pos;
  }

  /* FAIL */
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

static void app_header_cstr(Vstr_base *out,
                            const char *hdr, const char *data)
{
  vstr_add_fmt(out, out->len, "%s: %s\r\n", hdr, data);
}

static void app_header_vstr(Vstr_base *out,
                            const char *hdr,
                            Vstr_base *s1, size_t vpos, size_t vlen,
                            unsigned int type)
{
  vstr_add_fmt(out, out->len, "%s: ${vstr:%p%zu%zu%u}\r\n", hdr,
               s1, vpos, vlen, type);
}

static void app_header_uintmax(Vstr_base *out,
                               const char *hdr, uintmax_t data)
{
  vstr_add_fmt(out, out->len, "%s: %ju\r\n", hdr, data);
}

static void app_def_headers(Vstr_base *out, int keep_alive, time_t now)
{
  app_header_cstr(out, "Date", serv_date_rfc1123(now));
  app_header_cstr(out, "Server", CONF_SERV_NAME);
  /* FIXME: test presistent connections */
  if (!keep_alive)
    app_header_cstr(out, "Connection", "close");
}

static int app_response_ok(Vstr_base *out, Vstr_base *hdrs,
                           struct http_hdrs *http_hdrs, time_t mtime,
                           time_t now, int *head_op, int ver_1_1)
{
  Vstr_sect_node *h_im = http_hdrs->hdr_if_match;
  Vstr_sect_node *h_ims = http_hdrs->hdr_if_modified_since;
  Vstr_sect_node *h_inm = http_hdrs->hdr_if_none_match;
  Vstr_sect_node *h_iums = http_hdrs->hdr_if_unmodified_since;
  int cached_output = FALSE;
  
  if (difftime(now, mtime) < 0) /* if mtime in future, chop it #14.29 */
    mtime = now;

  /* assumes time doesn't go backwards ... From rfc2616:
   *
   Note: When handling an If-Modified-Since header field, some
   servers will use an exact date comparison function, rather than a
   less-than function, for deciding whether to send a 304 (Not
   Modified) response. To get best results when sending an If-
   Modified-Since header field for cache validation, clients are
   advised to use the exact date string received in a previous Last-
   Modified header field whenever possible.
  */
  if (h_ims->pos &&
      (vstr_cmp_cstr_eq(hdrs, h_ims->pos, h_ims->len,
                        serv_date_rfc1123(mtime)) ||
       vstr_cmp_cstr_eq(hdrs, h_ims->pos, h_ims->len,
                        serv_date_rfc850(mtime))  ||
       vstr_cmp_cstr_eq(hdrs, h_ims->pos, h_ims->len,
                        serv_date_asctime(mtime))))
    cached_output = TRUE;

  if (ver_1_1 && h_iums->pos &&
      (vstr_cmp_cstr_eq(hdrs, h_iums->pos, h_iums->len,
                        serv_date_rfc1123(mtime)) ||
       vstr_cmp_cstr_eq(hdrs, h_iums->pos, h_iums->len,
                        serv_date_rfc850(mtime))  ||
       vstr_cmp_cstr_eq(hdrs, h_iums->pos, h_iums->len,
                        serv_date_asctime(mtime))))
    return (FALSE);

  if (ver_1_1 && h_inm->pos &&
      vstr_cmp_cstr_eq(hdrs, h_inm->pos, h_inm->len, "*"))
    cached_output = TRUE;

  if (ver_1_1 && h_im->pos &&
      !vstr_cmp_cstr_eq(hdrs, h_inm->pos, h_inm->len, "*"))
    return (FALSE);
  
  if (cached_output)
  {
    *head_op = TRUE;
    vstr_add_fmt(out, out->len, "%s %03d %s\r\n",
                 ver_1_1 ? "HTTP/1.1" : "HTTP/1.0", 304, "NOT MODIFIED");
  }
  else
    vstr_add_fmt(out, out->len, "%s %03d %s\r\n",
                 ver_1_1 ? "HTTP/1.1" : "HTTP/1.0", 200, "OK");

  return (TRUE);
}

/* vprefix, with local knowledge */
#define HDR__EQ(x)                                                    \
    ((len > (name_len = strlen(x ":"))) &&                            \
     vstr_cmp_buf_eq(data, pos, name_len, x ":", name_len))
#define HDR__SET(h) do {                                        \
      len -= name_len; pos += name_len;                         \
      name_len = vstr_spn_cstr_chrs_fwd(data, pos, len, " \t"); \
      len -= name_len; pos += name_len;                         \
      http_hdrs -> hdr_ ## h ->pos = pos;                       \
      http_hdrs -> hdr_ ## h ->len = len;                       \
    } while (FALSE)
static void parse_hdrs(Vstr_base *data, struct http_hdrs *http_hdrs,
                       Vstr_sects *sects, unsigned int num)
{
  while (++num <= sects->num)
  {
    size_t pos = VSTR_SECTS_NUM(sects, num)->pos;
    size_t len = VSTR_SECTS_NUM(sects, num)->len;
    size_t name_len = 0;

    if (0) { /* nothing */ }
    else if (HDR__EQ("User-Agent"))          HDR__SET(ua);
    else if (HDR__EQ("Referer"))             HDR__SET(referrer);
    else if (HDR__EQ("Connection"))          HDR__SET(connection);
    else if (HDR__EQ("Expect"))              HDR__SET(expect);
    else if (HDR__EQ("Host"))                HDR__SET(host);
    else if (HDR__EQ("If-Match"))            HDR__SET(if_match);
    else if (HDR__EQ("If-Modified-Since"))   HDR__SET(if_modified_since);
    else if (HDR__EQ("If-None-Match"))       HDR__SET(if_none_match);
    else if (HDR__EQ("If-Range"))            HDR__SET(if_range);
    else if (HDR__EQ("If-Unmodified-Since")) HDR__SET(if_unmodified_since);
    else if (HDR__EQ("Range"))               HDR__SET(range);
  }
}
#undef HDR__EQ
#undef HDR__SET

static void serv_clear_hdrs(struct con *con)
{
  CON_HDR_SET(con, ua, 0, 0);
  CON_HDR_SET(con, referrer, 0, 0);

  CON_HDR_SET(con, connection, 0, 0);
  CON_HDR_SET(con, expect, 0, 0);
  CON_HDR_SET(con, host, 0, 0);
  CON_HDR_SET(con, if_match, 0, 0);
  CON_HDR_SET(con, if_modified_since, 0, 0);
  CON_HDR_SET(con, if_none_match, 0, 0);
  CON_HDR_SET(con, if_range, 0, 0);
  CON_HDR_SET(con, if_unmodified_since, 0, 0);
  CON_HDR_SET(con, range, 0, 0);
}

/* might have been able to do it with string matches, but getting...
 * "HTTP/1.1" = OK
 * "HTTP/1.10" = OK
 * "HTTP/1.10000000000000" = BAD
 * ...seemed not as easy */
static int serv_http_parse_version(Vstr_base *data,
                                   size_t op_pos, size_t op_len, int *ver_1_1)
{
  unsigned int major = 0;
  unsigned int minor = 0;
  size_t num_len = strlen("HTTP/");
  /* is HTTP/01.01 valid ? ... I'm not going to allow it */
  unsigned int num_flags = 10 | (VSTR_FLAG_PARSE_NUM_NO_BEG_PM |
                                 VSTR_FLAG_PARSE_NUM_NO_BEG_ZERO |
                                 VSTR_FLAG_PARSE_NUM_OVERFLOW);
  
  if (!VPREFIX(data, op_pos, op_len, "HTTP/"))
    return (400);
  
  op_len -= num_len; op_pos += num_len;
  major = vstr_parse_uint(data, op_pos, op_len, num_flags,
                          &num_len, NULL);
  op_len -= num_len; op_pos += num_len;
    
  if (!num_len || !op_len || (vstr_export_chr(data, op_pos) != '.'))
    return (400);
    
  op_len -= 1; op_pos += 1;
  minor = vstr_parse_uint(data, op_pos, op_len, num_flags,
                          &num_len, NULL);
  op_len -= num_len; op_pos += num_len;
  if (!num_len || op_len)
    return (400);

  if (0) { } /* not allowing HTTP/0.9 here */
  else if ((major == 1) && (minor >= 1))
    *ver_1_1 = TRUE;
  else if ((major == 1) && (minor == 0))
  { /* do nothing */ }
  else
    return (505);
        
  return (0);
}

static int serv_http_parse_1_0(struct con *con,
                               Vstr_base *data, Vstr_sects *sects,
                               int *ver_1_1)
{
  Vstr_sect_node *h_c = con->http_hdrs->hdr_connection;
  size_t op_pos = VSTR_SECTS_NUM(sects, 3)->pos;
  size_t op_len = VSTR_SECTS_NUM(sects, 3)->len;
  int ret = 0;
  
  if ((ret = serv_http_parse_version(data, op_pos, op_len, ver_1_1)))
    return (ret);
  
  parse_hdrs(data, con->http_hdrs, sects, 3);
        
  if (*ver_1_1 && (!h_c->pos ||
                   !vstr_cmp_cstr_eq(data, h_c->pos, h_c->len, "close")))
    con->keep_alive = TRUE;

  return (0);
}

static int serv_http_absolute_uri(struct con *con, Vstr_base *data,
                                  size_t *passed_op_pos, size_t *passed_op_len,
                                  int ver_1_1)
{
  size_t op_pos = *passed_op_pos;
  size_t op_len = *passed_op_len;
  
  /* check for absolute URIs */
  if (VPREFIX(data, op_pos, op_len, "http://"))
  { /* ok, be forward compatible */
    size_t tmp = strlen("http://");
    
    op_len -= tmp; op_pos += tmp;
    tmp = vstr_srch_chr_fwd(data, op_pos, op_len, '/');
    if (!tmp)
    {
      CON_HDR_SET(con, host, op_pos, op_len);
      op_len = 1;
      --op_pos;
    }
    else
    { /* found end of host ... */
      size_t host_len = vstr_sc_posdiff(op_pos, tmp);
      
      CON_HDR_SET(con, host, op_pos, host_len);
      op_len -= host_len; op_pos += host_len;
    }
    ASSERT(vstr_cmp_cstr_eq(data, op_pos, op_len, "/"));
  }

   /* HTTP/1.1 requires a host -- allow blank hostnames ? */
  if (ver_1_1 && !con->http_hdrs->hdr_host->pos)
    return (FALSE);
  
  if (con->http_hdrs->hdr_host->pos)
  { /* check host looks valid */
    size_t pos = con->http_hdrs->hdr_host->pos;
    size_t len = con->http_hdrs->hdr_host->len;
    size_t tmp = 0;

    /* leaving out deep checks for ".." or invalid chars in hostnames etc.
       as the default filename checks shoudl catch them
     */
    
    if ((tmp = vstr_srch_chr_fwd(data, pos, len, ':')))
    { /* NOTE: not sure if we have to 400 if the port doesn't match
       * or if it's an "invalid" port number (Ie. == 0 || > 65535) */
      len -= vstr_sc_posdiff(pos, tmp); pos += vstr_sc_posdiff(pos, tmp);
      tmp = vstr_spn_cstr_chrs_fwd(data, pos, len, "0123456789");
      if (tmp != len)
        return (FALSE);
    }
  }

  *passed_op_pos = op_pos;
  *passed_op_len = op_len;
  
  return (TRUE);
}

static int serv_recv(struct con *con)
{
  int ret = evnt_recv(con->ev);
  Vstr_base *data = con->ev->io_r;
  Vstr_base *out  = con->ev->io_w;
  Vstr_base *fname = NULL;
  size_t req_len = 0;
  unsigned int http_error_code = 0;
  const char *http_error_line = "";
  const char *http_error_msg  = "";
  Vstr_sects *sects = NULL;
  int ver_0_9 = FALSE;
  int ver_1_1 = FALSE;
  int redirect_loc = FALSE;
  int redirect_http_error_msg = FALSE;
  const char *eor = "\r\n";
  
  if (!ret && !data->len && (out->len || (con->f_fd != -1)))
  { /* untested -- getting EOF while still doing an old request */
    SOCKET_POLL_INDICATOR(con->ev->ind)->events &= ~POLLIN;
    return (TRUE);
  }

  if (!ret) /* maybe finish outputting last req, first ? */
    goto con_cleanup;

  if (con->parsed_method_ver_1_0) /* wait for all the headers */
    eor = "\r\n\r\n";
  
  if (!(req_len = vstr_srch_cstr_buf_fwd(data, 1, data->len, eor)))
  {
    if (server_max_header_sz && (data->len > server_max_header_sz))
      goto con_cleanup;
    
    return (TRUE);
  }
  req_len += strlen(eor) - 1;

  if (!(sects = vstr_sects_make(8)))
    goto con_cleanup;
  
  con->keep_alive = FALSE;
  req_split_method(sects, data, 1, req_len, &ver_0_9);
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

    if (!ver_0_9)
    { /* need to get headers */

      if (!con->parsed_method_ver_1_0)
      {
        con->parsed_method_ver_1_0 = TRUE;
        eor = "\r\n\r\n";
      
        if (!(req_len = vstr_srch_cstr_buf_fwd(data, 1, data->len, eor)))
        {
          if (server_max_header_sz && (data->len > server_max_header_sz))
            goto con_cleanup;
          
          vstr_sects_free(sects);
          
          return (TRUE);
        }
        req_len += strlen(eor) - 1;
      }
      
      req_split_headers(sects, data, 1, req_len);
    }
    
    assert(((sects->num >= 3) && !ver_0_9) || (sects->num == 2));
    
    op_pos = VSTR_SECTS_NUM(sects, 1)->pos;
    op_len = VSTR_SECTS_NUM(sects, 1)->len;

    if (0) { }
    else if (vstr_cmp_cstr_eq(data, op_pos, op_len, "GET") ||
             (!ver_0_9 &&
              (head_op = vstr_cmp_cstr_eq(data, op_pos, op_len, "HEAD"))))
    {
      struct stat64 f_stat;  
      const char *http_req_content_type = "application/octet-stream";
      const char *fname_cstr = NULL;
      
      if (!ver_0_9)
      {
        int ecode = serv_http_parse_1_0(con, data, sects, &ver_1_1);

        if (ecode)
          switch (ecode)
          {
            case 400: HTTPD_ERR(400, head_op); goto con_fin_error_code;
            case 505: HTTPD_ERR(505, head_op); goto con_fin_error_code;
            default: ASSERT_NOT_REACHED();
          }
      }
      
      op_pos = VSTR_SECTS_NUM(sects, 2)->pos;
      op_len = VSTR_SECTS_NUM(sects, 2)->len;

      if (!serv_http_absolute_uri(con, data, &op_pos, &op_len, ver_1_1))
      {
        HTTPD_ERR(400, head_op);
        goto con_fin_error_code;
      }
      
      if (!(fname = vstr_dup_vstr(NULL,
                                  data, op_pos, op_len, VSTR_TYPE_ADD_BUF_PTR)))
        goto con_cleanup;
      
      vstr_conv_decode_uri(fname, 1, fname->len);
            
      ASSERT(vstr_export_chr(fname, 1) == '/');

      /* allow pre 1.1 to pass a host header */
      if (CONF_SERV_USE_VHOST && con->http_hdrs->hdr_host->pos)
      {
        vstr_add_vstr(fname, 0, data,
                      con->http_hdrs->hdr_host->pos,
                      con->http_hdrs->hdr_host->len, VSTR_TYPE_ADD_BUF_PTR);
        vstr_add_cstr_ptr(fname, 0, "/");
      }
      
      /* check path ... */
      if (vstr_srch_chr_fwd(fname, 1, fname->len, 0))
      { /* POSIX APIs don't like embedded NILs */
        HTTPD_ERR(400, head_op);
        goto con_fin_error_code;
      }
      
      if (vstr_srch_cstr_buf_fwd(fname, 1, fname->len, "/../"))
      {
        HTTPD_ERR(403, head_op); /* or 400 ? */
        goto con_fin_error_code;
      }

      /* check Vhost */
      
      vstr_del(fname, 1, 1);
      
      if (!fname->len)
        vstr_add_cstr_ptr(fname, fname->len, "index.html");
      
     retry_req:
      HTTP_REQ_MIME_TYPE(fname, http_req_content_type);
      
      fname_cstr = vstr_export_cstr_ptr(fname, 1, fname->len);
      
      if (fname->conf->malloc_bad)
        goto con_cleanup;
      
      /* simple logger */
      if (server_daemon)
        syslog(LOG_NOTICE,
               "REQ %s @[%s] from[%s] t[%s]: %s\n", head_op ? "HEAD" : "GET",
               serv_date_rfc1123(time(NULL)),
               inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr),
               http_req_content_type, fname_cstr);
      else
      {
        printf("REQ %s @[%s] from[%s] t[%s]: %s\n", head_op ? "HEAD" : "GET",
               serv_date_rfc1123(time(NULL)),
               inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr),
               http_req_content_type, fname_cstr);
        fflush(NULL);
      }
      
      if ((con->f_fd = EX_UTILS_OPEN(fname_cstr, O_RDONLY | O_NOCTTY)) == -1)
      {
        if (0) { }
        else if (errno == EISDIR)
        {
          HTTP_REQ_CHK_DIR(fname, con_fin_error_code);
          vstr_add_cstr_ptr(fname, fname->len, "index.html");
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
      if (fstat64(con->f_fd, &f_stat) == -1)
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
        HTTP_REQ_CHK_DIR(fname, con_close_fin_error_code);

        close(con->f_fd);
        con->f_fd = -1;
        vstr_add_cstr_ptr(fname, fname->len, "index.html");
        goto retry_req;
      }
      
      vstr_free_base(fname);
      fname      = NULL;
      
      if (!ver_0_9)
      {
        time_t now = time(NULL);

        if (ver_1_1 && con->http_hdrs->hdr_expect->len)
        { /* I'm pretty sure we can ignore 100-continue, as no request will
           * have a body */
          HTTPD_ERR(417, head_op);
          goto con_close_fin_error_code;
        }
          
        if (!app_response_ok(out, data, con->http_hdrs, f_stat.st_mtime, now,
                             &head_op, ver_1_1))
        {
          HTTPD_ERR(412, head_op);
          goto con_close_fin_error_code;
        }
        
        app_def_headers(out, con->keep_alive, now);
        app_header_cstr(out, "Last-Modified",
                        serv_date_rfc1123(f_stat.st_mtime));
        app_header_cstr(out, "Content-Type",
                        http_req_content_type);
        app_header_uintmax(out, "Content-Length", f_stat.st_size);
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
    else if (!ver_0_9 &&
             (vstr_cmp_cstr_eq(data, op_pos, op_len, "OPTIONS") ||
              vstr_cmp_cstr_eq(data, op_pos, op_len, "POST") ||
              vstr_cmp_cstr_eq(data, op_pos, op_len, "PUT") ||
              vstr_cmp_cstr_eq(data, op_pos, op_len, "DELETE") ||
              vstr_cmp_cstr_eq(data, op_pos, op_len, "TRACE") ||
              vstr_cmp_cstr_eq(data, op_pos, op_len, "CONNECT") ||
              FALSE))
    {
      HTTPD_ERR(403, head_op);
      goto con_fin_error_code;
    }
    else
    {
      HTTPD_ERR(501, head_op);
      goto con_fin_error_code;
    }
  }
  ASSERT_NOT_REACHED();
  
 con_fin_error_code:
  if (!ver_0_9)
  {
    vstr_add_fmt(out, out->len, "%s %03u %s\r\n\r\r",
                 ver_1_1 ? "HTTP/1.1" : "HTTP/1.0",
                 http_error_code, http_error_line);
    app_def_headers(out, con->keep_alive, time(NULL));
    if (http_error_code == 403)
    app_header_cstr(out, "Allow", "GET, HEAD");      
    if (redirect_loc)
      app_header_vstr(out, "Location", fname, 1, fname->len, 0);
    app_header_cstr(out, "Last-Modified",
                    serv_date_rfc1123(server_beg));
    app_header_cstr(out, "Content-Type", "text/html");
    if (redirect_loc)
      app_header_uintmax(out, "Content-Length",
                         CONF_MSG_LEN_301(fname, 1, fname->len));
    else
      app_header_uintmax(out, "Content-Length",
                         strlen(http_error_msg));
    vstr_add_cstr_ptr(out, out->len, "\r\n\r\r");
  }
  
  if (redirect_loc && redirect_http_error_msg)
    vstr_add_fmt(out, out->len, CONF_MSG_FMT_301,
                 CONF_MSG__FMT_301_BEG,
                 fname, 1, fname->len, VSTR_TYPE_ADD_DEF,
                 CONF_MSG__FMT_301_END);
  else
    vstr_add_cstr_ptr(out, out->len, http_error_msg);

 con_do_write:
  serv_clear_hdrs(con);
  /* Here as all the headers use it */
  vstr_del(data, 1, req_len);  
  con->parsed_method_ver_1_0 = FALSE; /* for possible next method req */
  
  vstr_free_base(fname);  fname = NULL;
  vstr_sects_free(sects); sects = NULL;

  if (evnt_send_add(con->ev, FALSE, CL_MAX_WAIT_SEND))
    return (TRUE);
  
 con_cleanup:
  vstr_free_base(fname);
  con->ev->io_r->conf->malloc_bad = FALSE;
  con->ev->io_w->conf->malloc_bad = FALSE;
  
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
    return (con->keep_alive || (con->ev->io_w->len != 0));
  
  while (io_get(con->ev->io_w, con->f_fd) != IO_EOF)
  {
    evnt_send_add(con->ev, TRUE, CL_MAX_WAIT_SEND);
    
    if (con->ev->io_w->len >= CONF_OUTPUT_SZ)
      return (TRUE);
  }
  
  close(con->f_fd);
  con->f_fd = -1;
  
  evnt_send_add(con->ev, TRUE, CL_MAX_WAIT_SEND);
  
  if (!con->keep_alive && !con->ev->io_w->len)
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
  struct con *con = NULL;
  struct timeval tv;

  ASSERT(!server_max_clients || (server_clients_count <= server_max_clients));
  if (server_max_clients && (server_clients_count >= server_max_clients))
    goto make_acpt_fail;
  
  if (!(con = malloc(sizeof(struct con))) ||
      !evnt_make_acpt(con->ev, fd, sa, len))
    goto make_acpt_fail;

  tv = con->ev->ctime;
  TIMER_Q_TIMEVAL_ADD_SECS(&tv, 0, client_timeout);

  con->keep_alive = FALSE;
  con->parsed_method_ver_1_0 = FALSE;

  serv_clear_hdrs(con);
  
  if (!(con->ev->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                         TIMER_Q_FLAG_NODE_DEFAULT)))
    goto timer_add_fail;

  if (!(con->sa = malloc(len)))
    goto malloc_sa_fail;
  memcpy(con->sa, sa, len);
  
  /* simple logger */
  if (server_daemon)
    syslog(LOG_NOTICE, "CONNECT @[%s] from[%s]\n",
           serv_date_rfc1123(time(NULL)),
           inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr));
  else
  {
    printf("CONNECT @[%s] from[%s]\n",
           serv_date_rfc1123(time(NULL)),
           inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr));
    fflush(NULL);
  }
  
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

static void cl_cmd_line(int argc, char *argv[])
{
  char optchar = 0;
  const char *program_name = "jhttpd";
  struct option long_options[] =
  {
   {"help", no_argument, NULL, 'h'},
   {"daemon", no_argument, NULL, 99},
   {"debug", no_argument, NULL, 'd'},
   {"host", required_argument, NULL, 'H'},
   {"port", required_argument, NULL, 'P'},
   {"nagle", optional_argument, NULL, 'n'},
   {"max-clients", required_argument, NULL, 'M'},
   {"max-header-sz", required_argument, NULL, 1},
   {"timeout", required_argument, NULL, 't'},
   {"version", no_argument, NULL, 'V'},
   {NULL, 0, NULL, 0}
  };
  
  if (argv[0])
  {
    if ((program_name = strrchr(argv[0], '/')))
      ++program_name;
    else
      program_name = argv[0];
  }

  while ((optchar = getopt_long(argc, argv, "d:hH:M:nP:t:V",
                                long_options, NULL)) != EOF)
  {
    switch (optchar)
    {
      case '?':
        fprintf(stderr, " That option is not valid.\n");
      case 'h':
        usage(program_name, 'h' == optchar);
        
      case 'V':
        printf(" %s version 0.1.1, compiled on %s.\n",
               program_name,
               __DATE__);
        printf(" %s compiled on %s.\n", program_name, __DATE__);
        exit (EXIT_SUCCESS);

      case 't': client_timeout       = atoi(optarg); break;
      case 'H': server_ipv4_address  = optarg;       break;
      case   1: server_max_header_sz = atoi(optarg); break;
      case 'M': server_max_clients   = atoi(optarg); break;
      case 'P': server_port          = atoi(optarg); break;
        
      case 'd': cl_dbg_opt           = !cl_dbg_opt;  break;
        
      case 'n':
        if (!optarg)
        { evnt_opt_nagle = !evnt_opt_nagle; }
        else if (!strcasecmp("true", optarg))   evnt_opt_nagle = TRUE;
        else if (!strcasecmp("1", optarg))      evnt_opt_nagle = TRUE;
        else if (!strcasecmp("false", optarg))  evnt_opt_nagle = FALSE;
        else if (!strcasecmp("0", optarg))      evnt_opt_nagle = FALSE;
        break;

      case 99:
        if (daemon(FALSE, FALSE) == -1)
          err(EXIT_FAILURE, "daemon");
        server_daemon = TRUE;
        openlog("jhttpd", 0, LOG_DAEMON);
        break;
        
      default:
        abort();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 1)
    usage(program_name, EXIT_FAILURE);

  if (chdir(argv[0]) == -1)
    err(EXIT_FAILURE, "chdir(%s)", argv[1]);
}

int main(int argc, char *argv[])
{
  serv_init();

  cl_cmd_line(argc, argv);
  
  cl_timeout_base = timer_q_add_base(cl_timer_con, TIMER_Q_FLAG_BASE_DEFAULT);

  if (!cl_timeout_base)
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  if (!(acpt_evnt = malloc(sizeof(struct Evnt))))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  
  if (!evnt_make_bind_ipv4(acpt_evnt, server_ipv4_address, server_port))
    errx(EXIT_FAILURE, __func__);

  SOCKET_POLL_INDICATOR(acpt_evnt->ind)->events |= POLLIN;

  acpt_evnt->cbs->cb_func_accept = serv_cb_func_accept;
  acpt_evnt->cbs->cb_func_free   = serv_cb_func_acpt_free;

  server_beg = time(NULL);
  if (server_daemon)
    syslog(LOG_NOTICE, "READY @[%s]!\n", serv_date_rfc1123(time(NULL)));
  else
  {
    printf("READY @[%s]!\n", serv_date_rfc1123(time(NULL)));
    fflush(NULL);
  }
  
  while (acpt_evnt || server_clients_count)
  {
    int ready = evnt_poll();
    struct timeval tv;
    
    if ((ready == -1) && (errno != EINTR))
      err(EXIT_FAILURE, __func__);
    if (ready == -1)
      continue;
    
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
