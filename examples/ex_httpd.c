#include <vstr.h>

#include <socket_poll.h>
#include <timer_q.h>

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

#include <sys/stat.h>
#include <signal.h>
#include <grp.h>

#ifdef __linux__
# include <sys/prctl.h>
#else
# define prctl(x1, x2, x3, x4, x5) 0
#endif

#include "opt.h"

/* is the data is less than this, queue instead of hitting write */
#define CL_MAX_WAIT_SEND 16

#include "evnt.h"

#define EX_UTILS_NO_USE_INIT  1
#define EX_UTILS_NO_USE_EXIT  1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_BLOCKING_OPEN 1
#define EX_UTILS_USE_NONBLOCKING_OPEN 1
#define EX_UTILS_RET_FAIL     1
#include "ex_utils.h"

#include "cntl.h"
#include "date.h"
#include "mime_types.h"

#define CONF_SERV_DEF_ADDR NULL
#define CONF_SERV_DEF_PORT   80

#define CONF_SERV_VERSION "0.9.1"

#if !defined(SO_DETACH_FILTER) || !defined(SO_ATTACH_FILTER)
# define CONF_SERV_USE_SOCKET_FILTERS FALSE
struct sock_fprog { int dummy; };
# define SO_DETACH_FILTER 0
# define SO_ATTACH_FILTER 0
#else
# define CONF_SERV_USE_SOCKET_FILTERS TRUE

/* not in glibc... hope it's not in *BSD etc. */
struct sock_filter
{
 uint16_t   code;   /* Actual filter code */
 uint8_t    jt;     /* Jump true */
 uint8_t    jf;     /* Jump false */
 uint32_t   k;      /* Generic multiuse field */
};

struct sock_fprog
{
 unsigned short len;    /* Number of filter blocks */
 struct sock_filter *filter;
};
#endif

/* defaults for runtime conf */
#define CONF_SERV_DEF_NAME "jhttpd/" CONF_SERV_VERSION " (Vstr)"
#define CONF_SERV_USE_MMAP FALSE
#define CONF_SERV_USE_SENDFILE TRUE
#define CONF_SERV_USE_KEEPA TRUE
#define CONF_SERV_USE_KEEPA_1_0 TRUE
#define CONF_SERV_USE_VHOST FALSE
#define CONF_SERV_USE_RANGE TRUE
#define CONF_SERV_USE_RANGE_1_0 TRUE
#define CONF_SERV_USE_PUBLIC_ONLY FALSE
#define CONF_SERV_USE_GZIP_CONTENT_REPLACEMENT TRUE
#define CONF_SERV_DEF_PROCS 1 /* HC usage txt */
#define CONF_SERV_DEF_TCP_DEFER_ACCEPT 16 /* HC usage txt */
#define CONF_SERV_DEF_DIR_FILENAME "index.html"
#define CONF_SERV_USE_DEFAULT_MIME_TYPE FALSE
#define CONF_SERV_DEF_MIME_TYPE "application/octet-stream"
#define CONF_SERV_MIME_TYPE_MAIN "/etc/mime.types"
#define CONF_SERV_MIME_TYPE_XTRA NULL

#include "ex_httpd_err_codes.h"

#define CONF_BUF_SZ (128 - sizeof(Vstr_node_buf))
#define CONF_MEM_PREALLOC_MIN (16  * 1024)
#define CONF_MEM_PREALLOC_MAX (128 * 1024)

#ifdef NDEBUG
# define CONF_HTTP_MMAP_LIMIT_MIN (16 * 1024) /* a couple of pages */
#else
# define CONF_HTTP_MMAP_LIMIT_MIN 8 /* debug... */
#endif
#define CONF_HTTP_MMAP_LIMIT_MAX (50 * 1024 * 1024)

#define CONF_FS_READ_CALL_LIMIT 8

/* this is configurable, but is much higher than EX_MAX_R_DATA_INCORE due to
 * allowing largish requests with HTTP */
#ifdef NDEBUG
# define CONF_INPUT_MAXSZ (  1 * 1024 * 1024)
#else
# define CONF_INPUT_MAXSZ (128 * 1024) /* debug */
#endif

#define CONF_ZEROPAD_SZ ((4 * 1024) - sizeof(Vstr_ref))

/* Linear Whitespace, a full stds. check for " " and "\t" */
#define HTTP_LWS " \t"

/* End of Line */
#define HTTP_EOL "\r\n"

/* End of Request - blank line following previous line in request */
#define HTTP_END_OF_REQUEST HTTP_EOL HTTP_EOL

/* HTTP crack -- Implied linear whitespace between tokens, note that it
 * might have a \r\n at the start */
#define HTTP_SKIP_LWS(s1, p, l) do {                                    \
      size_t lws__len = 0;                                              \
                                                                        \
      if (VPREFIX(s1, p, l, HTTP_EOL)) {                                \
        l -= strlen(HTTP_EOL); p += strlen(HTTP_EOL);                   \
      }                                                                 \
                                                                        \
      lws__len = vstr_spn_cstr_chrs_fwd(s1, p, l, HTTP_LWS);            \
      l -= lws__len;                                                    \
      p += lws__len;                                                    \
    } while (FALSE)

/* is the cstr a prefix of the vstr */
#define VPREFIX(vstr, p, l, cstr)                                       \
    (((l) >= strlen(cstr)) && vstr_cmp_bod_cstr_eq(vstr, p, l, cstr))
/* is the cstr a suffix of the vstr */
#define VSUFFIX(vstr, p, l, cstr)                                       \
    (((l) >= strlen(cstr)) && vstr_cmp_eod_cstr_eq(vstr, p, l, cstr))

/* is the cstr a prefix of the vstr, no case */
#define VIPREFIX(vstr, p, l, cstr)                                      \
    (((l) >= strlen(cstr)) && vstr_cmp_case_bod_cstr_eq(vstr, p, l, cstr))

#define TRUE  1
#define FALSE 0

struct Http_hdrs
{
 Vstr_sect_node hdr_ua[1]; /* logging headers */
 Vstr_sect_node hdr_referer[1]; /* NOTE: referrer */

 Vstr_sect_node hdr_accept_encoding[1];
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

struct Http_req_data
{
 Vstr_base *fname;
 size_t len;
 size_t path_pos;
 size_t path_len;
 unsigned int error_code;
 const char * error_line;
 const char * error_msg;
 size_t       error_len;
 Vstr_sects *sects;
 struct stat64 f_stat[1];
 size_t orig_io_w_len;
 Vstr_base *f_mmap;
 const Vstr_base *content_type_vs1;
 size_t           content_type_pos;
 size_t           content_type_len;
 unsigned int ver_0_9 : 1;
 unsigned int ver_1_1 : 1;
 unsigned int use_mmap : 1;
 unsigned int head_op : 1;
 unsigned int advertise_accept_ranges : 1;

 unsigned int content_encoding_gzip  : 1;
 unsigned int content_encoding_xgzip : 1; /* only valid if gzip is TRUE */

 unsigned int using_req : 1;
 unsigned int done_once : 1;

 unsigned int malloc_bad : 1;
};

struct Con
{
 struct Evnt ev[1];

 int f_fd;
 VSTR_AUTOCONF_uintmax_t f_off;
 VSTR_AUTOCONF_uintmax_t f_len;

 unsigned int parsed_method_ver_1_0 : 1;
 unsigned int keep_alive : 3;
 
 unsigned int use_sendfile : 1;
 
 struct Http_hdrs http_hdrs[1];
};

#define HTTP_NIL_KEEP_ALIVE 0
#define HTTP_1_0_KEEP_ALIVE 1
#define HTTP_1_1_KEEP_ALIVE 2

#define CON_HDR_SET(con, h, p, l) do {                      \
      (con)-> http_hdrs -> hdr_ ## h ->pos = (p);           \
      (con)-> http_hdrs -> hdr_ ## h ->len = (l);           \
    } while (FALSE)

static Timer_q_base *cl_timeout_base = NULL;

static struct Evnt *acpt_evnt = NULL;

static unsigned int client_timeout = (2 * 60); /* 2 minutes */

static const char *server_ipv4_address = CONF_SERV_DEF_ADDR;
static short server_port = CONF_SERV_DEF_PORT;

static unsigned int server_max_clients = 0;

static unsigned int server_max_header_sz = CONF_INPUT_MAXSZ;

static time_t server_beg = (time_t)-1;

static Vstr_ref *zeropad_data = NULL;

static struct 
{
 const char * server_name;

 unsigned int use_mmap : 1;
 unsigned int use_sendfile : 1;
 unsigned int use_keep_alive : 1;
 unsigned int use_keep_alive_1_0 : 1;
 unsigned int use_vhosts : 1;
 unsigned int use_range : 1;
 unsigned int use_range_1_0 : 1;
 unsigned int use_public_only : 1; /* 8th bitfield */
 unsigned int use_gzip_content_replacement : 1;
 unsigned int use_default_mime_type : 1;
 unsigned int num_procs : 8; /* 1 => 255 */
 unsigned int defer_accept : 14; /* 0 => 16384 (4hrs) */
 
 const char * dir_filename;
 const char * mime_types_default_type;
 const char * mime_types_main;
 const char * mime_types_xtra;
} serv[1] = {{CONF_SERV_DEF_NAME,
              CONF_SERV_USE_MMAP, CONF_SERV_USE_SENDFILE,
              CONF_SERV_USE_KEEPA, CONF_SERV_USE_KEEPA_1_0,
              CONF_SERV_USE_VHOST,
              CONF_SERV_USE_RANGE, CONF_SERV_USE_RANGE_1_0,
              CONF_SERV_USE_PUBLIC_ONLY,
              CONF_SERV_USE_GZIP_CONTENT_REPLACEMENT,
              CONF_SERV_USE_DEFAULT_MIME_TYPE,
              CONF_SERV_DEF_PROCS,
              CONF_SERV_DEF_TCP_DEFER_ACCEPT,
              CONF_SERV_DEF_DIR_FILENAME,
              CONF_SERV_DEF_MIME_TYPE,
              CONF_SERV_MIME_TYPE_MAIN,
              CONF_SERV_MIME_TYPE_XTRA}};

static Vlg *vlg = NULL;

static void usage(const char *program_name, int ret, const char *prefix)
{
  Vstr_base *out = vstr_make_base(NULL);

  if (!out)
    errno = ENOMEM, err(EXIT_FAILURE, "usage");

  vstr_add_fmt(out, 0, "%s\n\
 Format: %s [-dHhMnPtV] <dir>\n\
    --daemon          - Toggle becoming a daemon%s.\n\
    --chroot          - Change root.\n\
    --drop-privs      - Toggle droping privilages%s.\n\
    --priv-uid        - Drop privilages to this uid.\n\
    --priv-gid        - Drop privilages to this gid.\n\
    --pid-file        - Log pid to file.\n\
    --cntl-file       - Create control file.\n\
    --accept-filter-file - Load Linux Socket Filter code for accept().\n\
    --mmap            - Toggle use of mmap() to load files%s.\n\
    --sendfile        - Toggle use of sendfile() to load files%s.\n\
    --keep-alive      - Toggle use of Keep-Alive handling%s.\n\
    --keep-alive-1.0  - Toggle use of Keep-Alive handling for HTTP/1.0%s.\n\
    --virtual-hosts   - Toggle use of Host header%s.\n\
    --range           - Toggle use of partial responces%s.\n\
    --range-1.0       - Toggle use of partial responces for HTTP/1.0%s.\n\
    --public-only     - Toggle use of public only privilages%s.\n\
    --dir-filename    - Filename to use when requesting directories.\n\
    --server-name     - Contents of server header used in replies.\n\
    --gzip-content-replacement\n\
                      - Toggle use of gzip content replacement%s.\n\
    --send-default-mime-type\n\
                      - Toggle sending a default mime type%s.\n\
    --default-mime-type\n\
                      - Default mime type (application/octet-stream).\n\
    --mime-types-main - Main mime types filename (default: /etc/mime.types).\n\
    --mime-types-xtra - Additional mime types filename.\n\
    --processes       - Number of processes to use (default: 1).\n\
    --defer-accept    - Time to defer dataless connections (default: 16s).\n\
    --debug -d        - Raise debug level (can be used upto 3 times).\n\
    --host -H         - IPv4 address to bind (default: \"all\").\n\
    --help -h         - Print this message.\n\
    --max-clients -M  - Max clients allowed to connect (0 = no limit).\n\
    --max-header-sz   - Max size of http header (0 = no limit).\n\
    --nagle -n        - Toggle usage of nagle TCP option%s.\n\
    --port -P         - Port to bind to.\n\
    --timeout -t      - Timeout (usecs) for connections.\n\
    --version -V      - Print the version string.\n\
",
               prefix, program_name,
               opt_def_toggle(FALSE), opt_def_toggle(FALSE),
               opt_def_toggle(CONF_SERV_USE_MMAP),
               opt_def_toggle(CONF_SERV_USE_SENDFILE),
               opt_def_toggle(CONF_SERV_USE_KEEPA),
               opt_def_toggle(CONF_SERV_USE_KEEPA_1_0),
               opt_def_toggle(CONF_SERV_USE_VHOST),
               opt_def_toggle(CONF_SERV_USE_RANGE),
               opt_def_toggle(CONF_SERV_USE_RANGE_1_0),
               opt_def_toggle(CONF_SERV_USE_PUBLIC_ONLY),
               opt_def_toggle(CONF_SERV_USE_GZIP_CONTENT_REPLACEMENT),
               opt_def_toggle(CONF_SERV_USE_DEFAULT_MIME_TYPE),
               opt_def_toggle(evnt_opt_nagle));

  if (io_put_all(out, ret ? STDERR_FILENO : STDOUT_FILENO) == IO_FAIL)
    err(EXIT_FAILURE, "write");
  
  exit (ret);
}

static void ex_http__sig_crash(int s_ig_num)
{
  vlg_abort(vlg, "SIG: %d\n", s_ig_num);
}

/*
static void ex_http__sig_shutdown(int s_ig_num)
{
  assert(s_ig_num == SIGTERM);
  _exit(0);
}
*/

static volatile sig_atomic_t child_exited = FALSE;
static void ex_http__sig_child(int s_ig_num)
{
  ASSERT(s_ig_num == SIGCHLD);
  child_exited = TRUE;
}

static void serv_signals(void)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    err(EXIT_FAILURE, "signal init");
  
  /* don't use SA_RESTART ... */
  sa.sa_flags   = 0;
  /* ignore it... we don't have a use for it */
  sa.sa_handler = SIG_IGN;
  
  if (sigaction(SIGPIPE, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");

  sa.sa_handler = ex_http__sig_crash;
  
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  if (sigaction(SIGBUS, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  if (sigaction(SIGILL, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  if (sigaction(SIGFPE, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  if (sigaction(SIGXFSZ, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");

  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = ex_http__sig_child;
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  
  /*
  sa.sa_handler = ex_http__sig_shutdown;
  if (sigaction(SIGTERM, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  */
}

static void cl_timer_con(int type, void *data)
{
  struct Con *con = NULL;
  struct timeval tv;
  unsigned long diff = 0;
  
  if (type == TIMER_Q_TYPE_CALL_DEL)
    return;

  con = data;

  gettimeofday(&tv, NULL);
  diff = timer_q_timeval_udiff_secs(&tv, &con->ev->mtime);
  if (diff > client_timeout)
  {
    vlg_dbg2(vlg, "timeout = %p (%lu, %lu)\n",
             con, diff, (unsigned long)client_timeout);
    evnt_close(con->ev);
    return;
  }
  
  if (type == TIMER_Q_TYPE_CALL_RUN_ALL)
    return;
  
  TIMER_Q_TIMEVAL_ADD_SECS(&tv, (client_timeout - diff) + 1, 0);
  if (!(con->ev->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                         TIMER_Q_FLAG_NODE_DEFAULT)))
  {
    errno = ENOMEM, vlg_warn(vlg, "%s: %m\n", "timer reinit");
    evnt_close(con->ev);
  }
}

/* prints out headers in human friedly way for log files */
#define PCUR (pos + (base->len - orig_len))
static int http_app_vstr_escape(Vstr_base *base, size_t pos,
                                Vstr_base *sf, size_t sf_pos, size_t sf_len)
{
  unsigned int sf_flags = VSTR_TYPE_ADD_BUF_PTR;
  Vstr_iter iter[1];
  size_t orig_len = base->len;
  int saved_malloc_bad = FALSE;
  size_t norm_chr = 0;

  if (!sf_len) /* hack around !sf_pos */
    return (TRUE);
  
  if (!vstr_iter_fwd_beg(sf, sf_pos, sf_len, iter))
    return (FALSE);

  saved_malloc_bad = base->conf->malloc_bad;
  base->conf->malloc_bad = FALSE;
  while (sf_len)
  { /* assumes ASCII */
    char scan = vstr_iter_fwd_chr(iter, NULL);

    if ((scan >= ' ') && (scan <= '~') && (scan != '"') && (scan != '\\'))
      ++norm_chr;
    else
    {
      vstr_add_vstr(base, PCUR, sf, sf_pos, norm_chr, sf_flags);
      sf_pos += norm_chr;
      norm_chr = 0;
      
      if (0) {}
      else if (scan == '"')  vstr_add_cstr_buf(base, PCUR, "\\\"");
      else if (scan == '\\') vstr_add_cstr_buf(base, PCUR, "\\");
      else if (scan == '\t') vstr_add_cstr_buf(base, PCUR, "\\t");
      else if (scan == '\v') vstr_add_cstr_buf(base, PCUR, "\\v");
      else if (scan == '\r') vstr_add_cstr_buf(base, PCUR, "\\r");
      else if (scan == '\n') vstr_add_cstr_buf(base, PCUR, "\\n");
      else if (scan == '\b') vstr_add_cstr_buf(base, PCUR, "\\b");
      else
        vstr_add_sysfmt(base, PCUR, "\\x%02hhx", scan);
      ++sf_pos;
    }
    
    --sf_len;
  }

  vstr_add_vstr(base, PCUR, sf, sf_pos, norm_chr, sf_flags);
  
  if (base->conf->malloc_bad)
    return (FALSE);
  
  base->conf->malloc_bad = saved_malloc_bad;
  return (TRUE);
}
#undef PCUR

static int http__fmt__add_vstr_add_vstr(Vstr_base *base, size_t pos,
                                        Vstr_fmt_spec *spec)
{
  Vstr_base *sf = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_pos = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);
  size_t sf_len = VSTR_FMT_CB_ARG_VAL(spec, size_t, 2);
  
  return (http_app_vstr_escape(base, pos, sf, sf_pos, sf_len));
}
static int http__fmt_add_vstr_add_vstr(Vstr_conf *conf, const char *name)
{
  return (vstr_fmt_add(conf, name, http__fmt__add_vstr_add_vstr,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_SIZE_T,
                       VSTR_TYPE_FMT_SIZE_T,
                       VSTR_TYPE_FMT_END));
}
static int http__fmt__add_vstr_add_sect_vstr(Vstr_base *base, size_t pos,
                                             Vstr_fmt_spec *spec)
{
  Vstr_base *sf     = VSTR_FMT_CB_ARG_PTR(spec, 0);
  Vstr_sects *sects = VSTR_FMT_CB_ARG_PTR(spec, 1);
  unsigned int num  = VSTR_FMT_CB_ARG_VAL(spec, unsigned int, 2);
  size_t sf_pos     = VSTR_SECTS_NUM(sects, num)->pos;
  size_t sf_len     = VSTR_SECTS_NUM(sects, num)->len;
  
  return (http_app_vstr_escape(base, pos, sf, sf_pos, sf_len));
}
static int http__fmt_add_vstr_add_sect_vstr(Vstr_conf *conf, const char *name)
{
  return (vstr_fmt_add(conf, name, http__fmt__add_vstr_add_sect_vstr,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_UINT,
                       VSTR_TYPE_FMT_END));
}

static void serv_init(void)
{
  if (!vstr_init()) /* init the library */
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  vlg_init();

  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_TYPE_GRPALLOC_CACHE,
                      VSTR_TYPE_CNTL_CONF_GRPALLOC_CSTR))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, CONF_BUF_SZ))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  /* no passing of conf to evnt */
  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$') ||
      !vstr_sc_fmt_add_all(NULL) ||
      !vlg_sc_fmt_add_all(NULL) ||
      !VSTR_SC_FMT_ADD(NULL, http__fmt_add_vstr_add_vstr,
                       "<http-esc.vstr", "p%zu%zu", ">") ||
      !VSTR_SC_FMT_ADD(NULL, http__fmt_add_vstr_add_sect_vstr,
                       "<http-esc.vstr.sect", "p%p%u", ">"))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!(zeropad_data = vstr_ref_make_malloc(CONF_ZEROPAD_SZ)))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  memset(zeropad_data->ptr, 0, CONF_ZEROPAD_SZ);
  
  if (!(vlg = vlg_make()))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!VSTR_SC_FMT_ADD(vlg->out_vstr->conf, http__fmt_add_vstr_add_vstr,
                       "<http-esc.vstr", "p%zu%zu", ">") ||
      !VSTR_SC_FMT_ADD(vlg->out_vstr->conf, http__fmt_add_vstr_add_sect_vstr,
                       "<http-esc.vstr.sect", "p%p%u", ">"))

  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  evnt_logger(vlg);
  evnt_epoll_init();
  
  serv_signals();

  cl_timeout_base = timer_q_add_base(cl_timer_con, TIMER_Q_FLAG_BASE_DEFAULT);

  if (!cl_timeout_base)
    errno = ENOMEM, err(EXIT_FAILURE, "timer init");

  /* FIXME: massive hack 1.0.5 is broken */
  timer_q_cntl_base(cl_timeout_base,
                    TIMER_Q_CNTL_BASE_SET_FLAG_INSERT_FROM_END, FALSE);
}

static void serv_cntl_resources(void)
{ /* cap the amount of "wasted" resources we're using */
  
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BASE, 0, 20);

  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BUF,
                 0, (CONF_MEM_PREALLOC_MAX / CONF_BUF_SZ));
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_PTR,
                 0, (CONF_MEM_PREALLOC_MAX / CONF_BUF_SZ));
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_REF,
                 0, (CONF_MEM_PREALLOC_MAX / CONF_BUF_SZ));
}


static void http_req_split_method(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *s1 = con->ev->io_r;
  size_t pos = 1;
  size_t len = req->len;
  size_t el = 0;
  unsigned int orig_num = req->sects->num;
  
  assert(VSUFFIX(s1, pos, len, HTTP_EOL));
  
  /* split request */
  if (!(el = vstr_srch_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
    return;
  vstr_sects_add(req->sects, pos, el - pos);
  len -= (el - pos); pos += (el - pos);
  HTTP_SKIP_LWS(s1, pos, len);

  ASSERT(len); /* must have at least EOL */

  if (!VPREFIX(s1, pos, len, "/") && !VIPREFIX(s1, pos, len, "http://"))
  {
    req->sects->num = orig_num;
    return;
  }
  
  if (!(el = vstr_srch_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
  {
    req->ver_0_9 = TRUE;
    vstr_sects_add(req->sects, pos, len - strlen(HTTP_EOL));
    vlg_dbg1(vlg, "Method(0.9):"
             " $<http-esc.vstr.sect:%p%p%u> $<http-esc.vstr.sect:%p%p%u>\n",
             s1, req->sects, 1, s1, req->sects, 2);
    return;
  }
  vstr_sects_add(req->sects, pos, el - pos);
  len -= (el - pos); pos += (el - pos);
  HTTP_SKIP_LWS(s1, pos, len);

  el = vstr_srch_cstr_buf_fwd(s1, pos, len, HTTP_EOL);
  ASSERT(el);
  vstr_sects_add(req->sects, pos, el - pos);
}

static void http_req_split_hdrs(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *s1 = con->ev->io_r;
  size_t pos = 1;
  size_t len = req->len;
  size_t el = 0;
  size_t hpos = 0;

  ASSERT(req->sects->num >= 3);
  
  vlg_dbg1(vlg, "Method(1.x):"
           " $<http-esc.vstr.sect:%p%p%u> $<http-esc.vstr.sect:%p%p%u>"
           " $<http-esc.vstr.sect:%p%p%u>\n",
           s1, req->sects, 1, s1, req->sects, 2, s1, req->sects, 3);
    
  /* skip first line */
  el = (VSTR_SECTS_NUM(req->sects, req->sects->num)->pos +
        VSTR_SECTS_NUM(req->sects, req->sects->num)->len);
  
  ASSERT(vstr_cmp_cstr_eq(s1, el, strlen(HTTP_EOL), HTTP_EOL));
  len -= (el - pos) + strlen(HTTP_EOL); pos += (el - pos) + strlen(HTTP_EOL);
  
  if (VPREFIX(s1, pos, len, HTTP_EOL))
    return; /* end of headers */
  
  vlg_dbg1(vlg, "%s", " -- HEADERS --\n");

  ASSERT(vstr_srch_cstr_buf_fwd(s1, pos, len, HTTP_END_OF_REQUEST));
  /* split headers */
  hpos = pos;
  while ((el = vstr_srch_cstr_buf_fwd(s1, pos, len, HTTP_EOL)) != pos)
  {
    char chr = 0;
    
    len -= (el - pos) + strlen(HTTP_EOL); pos += (el - pos) + strlen(HTTP_EOL);

    chr = vstr_export_chr(s1, pos);
    if (chr == ' ' || chr == '\t') /* header continues to next line */
      continue;

    vstr_sects_add(req->sects, hpos, el - hpos);
    vlg_dbg1(vlg, "$<http-esc.vstr:%p%zu%zu>\n", s1, hpos, el - hpos);
    
    hpos = pos;
  }
}

static void http__app_header_hdr(Vstr_base *out, const char *hdr)
{
  vstr_add_cstr_buf(out, out->len, hdr);
  vstr_add_cstr_buf(out, out->len, ": ");  
}
static void http__app_header_eol(Vstr_base *out)
{
  vstr_add_cstr_buf(out, out->len, HTTP_EOL);
}

static void http_app_header_cstr(Vstr_base *out, const char *hdr,
                                 const char *data)
{
  http__app_header_hdr(out, hdr);
  vstr_add_cstr_buf(out, out->len, data);
  http__app_header_eol(out);
}

static void http_app_header_vstr(Vstr_base *out, const char *hdr,
                                 const Vstr_base *s1, size_t vpos, size_t vlen,
                                 unsigned int type)
{
  http__app_header_hdr(out, hdr);
  vstr_add_vstr(out, out->len, s1, vpos, vlen, type);
  http__app_header_eol(out);
}

static void http_app_header_fmt(Vstr_base *out, const char *hdr,
                                const char *fmt, ...)
   VSTR__COMPILE_ATTR_FMT(3, 4);
static void http_app_header_fmt(Vstr_base *out, const char *hdr,
                                const char *fmt, ...)
{
  va_list ap;

  http__app_header_hdr(out, hdr);
  
  va_start(ap, fmt);
  vstr_add_vfmt(out, out->len, fmt, ap);
  va_end(ap);

  http__app_header_eol(out);
}

static void http_app_header_uintmax(Vstr_base *out,
                                    const char *hdr,
                                    VSTR_AUTOCONF_uintmax_t data)
{
  http__app_header_hdr(out, hdr);
  vstr_add_fmt(out, out->len, "%ju", data);
  http__app_header_eol(out);
}

static void http_app_def_hdrs(struct Con *con, struct Http_req_data *req,
                              time_t now, time_t mtime,
                              const char *custom_content_type,
                              VSTR_AUTOCONF_uintmax_t content_length)
{
  Vstr_base *out = con->ev->io_w;

  http_app_header_cstr(out, "Date", date_rfc1123(now));
  http_app_header_cstr(out, "Server", serv->server_name);
  
  if (difftime(now, mtime) < 0) /* if mtime in future, chop it #14.29 */
    mtime = now;

  http_app_header_cstr(out, "Last-Modified", date_rfc1123(mtime));

  switch (con->keep_alive)
  {
    case HTTP_NIL_KEEP_ALIVE:
      http_app_header_cstr(out, "Connection", "close");
      break;
      
    case HTTP_1_0_KEEP_ALIVE:
      http_app_header_cstr(out, "Connection", "Keep-Alive");
      /* FALLTHROUGH */
    case HTTP_1_1_KEEP_ALIVE:
      if (0) /* do we really want this info. public ? */
        http_app_header_fmt(out, "Keep-Alive",
                            "%s=%u", "timeout", client_timeout); /* max=xxx ? */
      
      ASSERT_NO_SWITCH_DEF();
  }
  
  if (req->advertise_accept_ranges)
    http_app_header_cstr(out, "Accept-Ranges", "bytes");
  
  if (custom_content_type)
    http_app_header_cstr(out, "Content-Type", custom_content_type);
  else if (req->content_type_vs1) /* possible we don't send one */
    http_app_header_vstr(out, "Content-Type", req->content_type_vs1,
                         req->content_type_pos, req->content_type_len,
                         VSTR_TYPE_ADD_DEF);
  
  if (req->content_encoding_gzip)
  {
    if (req->content_encoding_xgzip)
      http_app_header_cstr(out, "Content-Encoding", "x-gzip");
    else
      http_app_header_cstr(out, "Content-Encoding", "gzip");
  }
  
  http_app_header_uintmax(out, "Content-Length", content_length);
}
static void http_app_end_hdrs(Vstr_base *out)
{
  http__app_header_eol(out);
}

static int http_app_response_ok(struct Con *con, struct Http_req_data *req,
                                time_t now,
                                unsigned int *http_ret_code,
                                const char ** http_ret_line)
{
  Vstr_base *hdrs = con->ev->io_r;
  time_t mtime = req->f_stat->st_mtime;
  Vstr_sect_node *h_im   = con->http_hdrs->hdr_if_match;
  Vstr_sect_node *h_ims  = con->http_hdrs->hdr_if_modified_since;
  Vstr_sect_node *h_inm  = con->http_hdrs->hdr_if_none_match;
  Vstr_sect_node *h_ir   = con->http_hdrs->hdr_if_range;
  Vstr_sect_node *h_iums = con->http_hdrs->hdr_if_unmodified_since;
  Vstr_sect_node *h_r    = con->http_hdrs->hdr_range;
  int h_ir_tst      = FALSE;
  int h_iums_tst    = FALSE;
  int req_if_range  = FALSE;
  int cached_output = FALSE;
  const char *date = NULL;
  
  if (difftime(now, mtime) < 0) /* if mtime in future, chop it #14.29 */
    mtime = now;

  if (req->ver_1_1 && h_iums->pos)
    h_iums_tst = TRUE;

  if (req->ver_1_1 && h_ir->pos && h_r->pos)
    h_ir_tst = TRUE;
  
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
  date = date_rfc1123(mtime);
  if (h_ims->pos && vstr_cmp_cstr_eq(hdrs, h_ims->pos, h_ims->len, date))
    cached_output = TRUE;
  if (h_iums_tst && vstr_cmp_cstr_eq(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst && vstr_cmp_cstr_eq(hdrs, h_ir->pos, h_ir->len, date))
    req_if_range = TRUE;
  
  date = date_rfc850(mtime);
  if (h_ims->pos && vstr_cmp_cstr_eq(hdrs, h_ims->pos, h_ims->len, date))
    cached_output = TRUE;
  if (h_iums_tst && vstr_cmp_cstr_eq(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst && vstr_cmp_cstr_eq(hdrs, h_ir->pos, h_ir->len, date))
    req_if_range = TRUE;
  
  date = date_asctime(mtime);
  if (h_ims->pos && vstr_cmp_cstr_eq(hdrs, h_ims->pos, h_ims->len, date))
    cached_output = TRUE;
  if (h_iums_tst && vstr_cmp_cstr_eq(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst && vstr_cmp_cstr_eq(hdrs, h_ir->pos, h_ir->len, date))
    req_if_range = TRUE;

  if (h_ir_tst && !req_if_range)
    h_r->pos = 0;
  
  if (req->ver_1_1)
  {
    if (h_inm->pos &&  vstr_cmp_cstr_eq(hdrs, h_inm->pos, h_inm->len, "*"))
      cached_output = TRUE;

    if (h_im->pos  && !vstr_cmp_cstr_eq(hdrs, h_inm->pos, h_inm->len, "*"))
      return (FALSE);
  }
  
  if (cached_output)
  {
    req->head_op = TRUE;
    *http_ret_code = 304;
    *http_ret_line = "Not Modified";
  }
  else if (h_r->pos)
  {
    *http_ret_code = 206;
    *http_ret_line = "Partial content";
  }

  return (TRUE);
}

/* vprefix, with local knowledge */
#define HDR__EQ(x)                                                      \
    ((len > (name_len = strlen(x ":"))) &&                              \
     vstr_cmp_case_buf_eq(data, pos, name_len, x ":", name_len))
/* remove LWS from front and end... what a craptastic std. */
#define HDR__SET(h) do {                                                \
      len -= name_len; pos += name_len;                                 \
                                                                        \
      HTTP_SKIP_LWS(data, pos, len);                                    \
      len -= vstr_spn_cstr_chrs_rev(data, pos, len, HTTP_LWS);          \
      if (VSUFFIX(data, pos, len, HTTP_EOL))                            \
        len -= strlen(HTTP_EOL);                                        \
                                                                        \
      http_hdrs -> hdr_ ## h ->pos = pos;                               \
      http_hdrs -> hdr_ ## h ->len = len;                               \
    } while (FALSE)
static int parse_hdrs(Vstr_base *data, struct Http_hdrs *http_hdrs,
                      Vstr_sects *sects, unsigned int num)
{
  while (++num <= sects->num)
  {
    size_t pos = VSTR_SECTS_NUM(sects, num)->pos;
    size_t len = VSTR_SECTS_NUM(sects, num)->len;
    size_t name_len = 0;

    if (0) { /* nothing */ }
    else if (HDR__EQ("User-Agent"))          HDR__SET(ua);
    else if (HDR__EQ("Referer"))             HDR__SET(referer);
    
    else if (HDR__EQ("Accept-Encoding"))     HDR__SET(accept_encoding);
    else if (HDR__EQ("Connection"))          HDR__SET(connection);
    else if (HDR__EQ("Expect"))              HDR__SET(expect);
    else if (HDR__EQ("Host"))                HDR__SET(host);
    else if (HDR__EQ("If-Match"))            HDR__SET(if_match);
    else if (HDR__EQ("If-Modified-Since"))   HDR__SET(if_modified_since);
    else if (HDR__EQ("If-None-Match"))       HDR__SET(if_none_match);
    else if (HDR__EQ("If-Range"))            HDR__SET(if_range);
    else if (HDR__EQ("If-Unmodified-Since")) HDR__SET(if_unmodified_since);
    else if (HDR__EQ("Range"))               HDR__SET(range);

    /* in theory 0 bytes is ok ... who cares */
    else if (HDR__EQ("Content-Length"))      return (FALSE);
    /* in theory identity is ok? ... who cares */
    else if (HDR__EQ("Transfer-Encoding"))   return (FALSE);
  }

  return (TRUE);
}
#undef HDR__EQ
#undef HDR__SET

static void serv_clear_hdrs(struct Con *con)
{
  CON_HDR_SET(con, ua, 0, 0);
  CON_HDR_SET(con, referer, 0, 0);

  CON_HDR_SET(con, accept_encoding, 0, 0);
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
 * ...seemed not as easy. It also seems like you have to accept...
 * "HTTP / 01 . 01" as "HTTP/1.1"
 */
static int http_req_parse_version(struct Con *con, struct Http_req_data *req,
                                  size_t op_pos, size_t op_len)
{
  Vstr_base *data = con->ev->io_r;
  unsigned int major = 0;
  unsigned int minor = 0;
  size_t num_len = 0;
  unsigned int num_flags = 10 | (VSTR_FLAG_PARSE_NUM_NO_BEG_PM |
                                 VSTR_FLAG_PARSE_NUM_OVERFLOW);
  
  if (!VPREFIX(data, op_pos, op_len, "HTTP"))
    HTTPD_ERR_RET(req, 400, FALSE);

  op_len -= strlen("HTTP"); op_pos += strlen("HTTP");
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  if (!VPREFIX(data, op_pos, op_len, "/"))
    HTTPD_ERR_RET(req, 400, FALSE);
  op_len -= strlen("/"); op_pos += strlen("/");
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  major = vstr_parse_uint(data, op_pos, op_len, num_flags, &num_len, NULL);
  op_len -= num_len; op_pos += num_len;
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  if (!num_len || !VPREFIX(data, op_pos, op_len, "."))
    HTTPD_ERR_RET(req, 400, FALSE);

  op_len -= strlen("."); op_pos += strlen(".");
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  minor = vstr_parse_uint(data, op_pos, op_len, num_flags, &num_len, NULL);
  op_len -= num_len; op_pos += num_len;
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  if (!num_len || op_len)
    HTTPD_ERR_RET(req, 400, FALSE);
  
  if (0) { } /* not allowing HTTP/0.9 here */
  else if ((major == 1) && (minor >= 1))
    req->ver_1_1 = TRUE;
  else if ((major == 1) && (minor == 0))
  { /* do nothing */ }
  else
    HTTPD_ERR_RET(req, 505, FALSE);
        
  return (TRUE);
}

/* parse >= 1.0 things like, version and headers */
static int http_req_parse_1_x(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->ev->io_r;
  Vstr_sect_node *h_c = con->http_hdrs->hdr_connection;
  size_t op_pos = VSTR_SECTS_NUM(req->sects, 3)->pos;
  size_t op_len = VSTR_SECTS_NUM(req->sects, 3)->len;

  ASSERT(!req->ver_0_9);
  
  if (!http_req_parse_version(con, req, op_pos, op_len))
    return (FALSE);
  
  if (!parse_hdrs(data, con->http_hdrs, req->sects, 3))
    HTTPD_ERR_RET(req, 413, FALSE); /* don't allow a request entity */

  if (!serv->use_keep_alive)
    return (TRUE);
  
  if (req->ver_1_1 &&
      (!h_c->pos || !vstr_cmp_case_cstr_eq(data, h_c->pos, h_c->len, "close")))
    con->keep_alive = HTTP_1_1_KEEP_ALIVE;
  else if (serv->use_keep_alive_1_0 && h_c->pos &&
           vstr_cmp_case_cstr_eq(data, h_c->pos, h_c->len, "keep-alive"))
    con->keep_alive = HTTP_1_0_KEEP_ALIVE;
  
  return (TRUE);
}

static int serv_http_absolute_uri(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->ev->io_r;
  size_t op_pos = req->path_pos;
  size_t op_len = req->path_len;
  
  /* check for absolute URIs */
  if (VIPREFIX(data, op_pos, op_len, "http://"))
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
      size_t host_len = tmp - op_pos;
      
      CON_HDR_SET(con, host, op_pos, host_len);
      op_len -= host_len; op_pos += host_len;
    }
    assert(VPREFIX(data, op_pos, op_len, "/"));
  }

  /* HTTP/1.1 requires a host -- allow blank hostnames */
  if (req->ver_1_1 && !con->http_hdrs->hdr_host->pos)
    return (FALSE);
  
  if (con->http_hdrs->hdr_host->len)
  { /* check host looks valid ... header must exist, but can be empty */
    size_t pos = con->http_hdrs->hdr_host->pos;
    size_t len = con->http_hdrs->hdr_host->len;
    size_t tmp = 0;

    /* leaving out most checks for ".." or invalid chars in hostnames etc.
       as the default filename checks should catch them
     */

    /*  Check for Host header with extra / ...
     * Ie. only allow a single directory name.
     *  We could just leave this (it's not a security check, /../ is checked
     * for at filepath time), but I feel like being anal and this way there
     * aren't multiple urls to a single path. */
    if (vstr_srch_chr_fwd(data, pos, len, '/'))
      return (FALSE);

    /* because we only parse for single ones, and some proxies/clients parse for
     * either ... make sure we don't have enbedded ones to remove possability
     * of request splitting */
    if (vstr_srch_chr_fwd(data, pos, len, '\r') ||
        vstr_srch_chr_fwd(data, pos, len, '\n'))
      return (FALSE);

    if ((tmp = vstr_srch_chr_fwd(data, pos, len, ':')))
    { /* NOTE: not sure if we have to 400 if the port doesn't match
       * or if it's an "invalid" port number (Ie. == 0 || > 65535) */
      len -= tmp - pos; pos = tmp;

      /* if it's port 80, pretend it's not there */
      if (vstr_cmp_cstr_eq(data, pos, len, ":80") ||
          vstr_cmp_cstr_eq(data, pos, len, ":"))
        con->http_hdrs->hdr_host->len -= len;
      else
      {
        len -= 1; pos += 1; /* skip the ':' */
        if (vstr_spn_cstr_chrs_fwd(data, pos, len, "0123456789") != len)
          return (FALSE);
      }
    }
  }

  /* do similar request splitting checks */
  if (vstr_srch_chr_fwd(data, op_pos, op_len, '\r') ||
      vstr_srch_chr_fwd(data, op_pos, op_len, '\n'))
    return (FALSE);
  
  /* uri#fragment ... craptastic clients pass this and assume it is ignored */
  op_len = vstr_cspn_cstr_chrs_fwd(data, op_pos, op_len, "#");
  
  req->path_pos = op_pos;
  req->path_len = op_len;
  
  return (TRUE);
}

static void http__parse_skip_blanks(Vstr_base *data,
                                    size_t *passed_pos, size_t *passed_len)
{
  size_t pos = *passed_pos;
  size_t len = *passed_len;
  
  HTTP_SKIP_LWS(data, pos, len);
  while (VPREFIX(data, pos, len, ",")) /* http crack */
  {
    len -= strlen(","); pos += strlen(",");
    HTTP_SKIP_LWS(data, pos, len);
  }

  *passed_pos = pos;
  *passed_len = len;
}


/* Allow...
   bytes=NUM-NUM
   bytes=-NUM
   bytes=NUM-
   ...and due to LWS etc. http crapola parsing, even...
   bytes = , , NUM - NUM , ,
   ...not allowing multiple ranges at once though, as multipart/byteranges
   is too much crack, I think this is stds. compliant.
 */
static int serv_http_parse_range(Vstr_base *data, size_t pos, size_t len,
                                 VSTR_AUTOCONF_uintmax_t fsize,
                                 VSTR_AUTOCONF_uintmax_t *range_beg,
                                 VSTR_AUTOCONF_uintmax_t *range_end)
{
  unsigned int num_flags = 10 | (VSTR_FLAG_PARSE_NUM_NO_BEG_PM |
                                 VSTR_FLAG_PARSE_NUM_OVERFLOW);
  size_t num_len = 0;
  
  *range_beg = 0;
  *range_end = 0;
  
  if (!VPREFIX(data, pos, len, "bytes"))
    return (0);
  len -= strlen("bytes"); pos += strlen("bytes");

  HTTP_SKIP_LWS(data, pos, len);
  
  if (!VPREFIX(data, pos, len, "="))
    return (0);
  len -= strlen("="); pos += strlen("=");

  http__parse_skip_blanks(data, &pos, &len);
  
  if (VPREFIX(data, pos, len, "-"))
  { /* num bytes at end */
    VSTR_AUTOCONF_uintmax_t tmp = 0;

    len -= strlen("-"); pos += strlen("-");
    HTTP_SKIP_LWS(data, pos, len);

    tmp = vstr_parse_uintmax(data, pos, len, num_flags, &num_len, NULL);
    len -= num_len; pos += num_len;
    if (!num_len)
      return (0);

    if (!tmp)
      return (416);
    
    if (tmp >= fsize)
      return (0);
    
    *range_beg = fsize - tmp;
    *range_end = fsize - 1;
  }
  else
  { /* offset - [end] */
    *range_beg = vstr_parse_uintmax(data, pos, len, num_flags, &num_len, NULL);
    len -= num_len; pos += num_len;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "-"))
      return (0);
    len -= strlen("-"); pos += strlen("-");
    HTTP_SKIP_LWS(data, pos, len);

    if (!len || VPREFIX(data, pos, len, ","))
      *range_end = fsize - 1;
    else
    {
      *range_end = vstr_parse_uintmax(data, pos, len, num_flags, &num_len,NULL);
      len -= num_len; pos += num_len;
      if (!num_len)
        return (0);
      
      if (*range_end >= fsize)
        *range_end = fsize - 1;
    }
    
    if ((*range_beg >= fsize) || (*range_beg > *range_end))
      return (416);
    
    if ((*range_beg == 0) && 
        (*range_end == (fsize - 1)))
      return (0);
  }

  http__parse_skip_blanks(data, &pos, &len);
  
  if (len) /* after all that, ignore if there is more than one range */
    return (0);

  return (200);
}

static void serv_conf_file(struct Con *con, struct Http_req_data *req,
                           VSTR_AUTOCONF_uintmax_t f_off,
                           VSTR_AUTOCONF_uintmax_t f_len)
{
  ASSERT(!req->head_op);
  ASSERT(!req->use_mmap);
  
  if (!con->use_sendfile)
    return;
  
  con->f_off = f_off;
  con->f_len = f_len;
}

static void serv_call_mmap(struct Con *con, struct Http_req_data *req,
                           VSTR_AUTOCONF_uintmax_t off,
                           VSTR_AUTOCONF_uintmax_t f_len)
{
  Vstr_base *data = con->ev->io_r;
  VSTR_AUTOCONF_uintmax_t mmoff = off;
  VSTR_AUTOCONF_uintmax_t mmlen = f_len;
  
  ASSERT(!req->f_mmap || !req->f_mmap->len);
  ASSERT(!req->use_mmap);
  ASSERT(!req->head_op);

  if (con->use_sendfile)
    return;

  if (!serv->use_mmap ||
      (f_len < CONF_HTTP_MMAP_LIMIT_MIN) || (f_len > CONF_HTTP_MMAP_LIMIT_MAX))
    return;
  
  /* mmap offset needs to be aligned - so tweak offset before and after */
  mmoff /= 4096;
  mmoff *= 4096;
  ASSERT(mmoff <= off);
  mmlen += off - mmoff;

  if (!req->f_mmap && !(req->f_mmap = vstr_make_base(data->conf)))
    VLG_WARN_RET_VOID((vlg, /* fall back to read */
                       "failed to allocate mmap Vstr.\n"));
  ASSERT(!req->f_mmap->len);
  
  if (!vstr_sc_mmap_fd(req->f_mmap, 0, con->f_fd, mmoff, mmlen, NULL))
    VLG_WARN_RET_VOID((vlg, /* fall back to read */
                       "mmap($<http-esc.vstr:%p%zu%zu>,"
                            "(%ju,%ju)->(%ju,%ju)): %m\n",
                       req->fname, 1, req->fname->len,
                       off, f_len, mmoff, mmlen));

  req->use_mmap = TRUE;  
  vstr_del(req->f_mmap, 1, off - mmoff);
    
  ASSERT(req->f_mmap->len == f_len);
  
  /* possible range request successful, alter response length */
  con->f_len = f_len;
}

static int serv_call_seek(struct Con *con, struct Http_req_data *req,
                          VSTR_AUTOCONF_uintmax_t f_off,
                          VSTR_AUTOCONF_uintmax_t f_len,
                          unsigned int *http_ret_code,
                          const char ** http_ret_line)
{
  ASSERT(!req->head_op);
  
  if (req->use_mmap || con->use_sendfile)
    return (FALSE);

  if (f_off && (lseek64(con->f_fd, f_off, SEEK_SET) == -1))
  { /* this should be impossible for normal files AFAIK */
    vlg_warn(vlg, "lseek($<http-esc.vstr:%p%zu%zu>,off=%ju): %m\n",
             req->fname, 1, req->fname->len, f_off);
    /* req->advertise_accept_ranges = FALSE; - turn off? */
    con->http_hdrs->hdr_range->pos = 0;
    *http_ret_code = 200;
    *http_ret_line = "OK - Range Failed";
    return (FALSE);
  }
  
  con->f_len = f_len;

  return (TRUE);
}

/* is the quality set at zero -- might only work for accept-encoding */
static int http_parse_quality_zero(Vstr_base *data, size_t pos, size_t len,
                                   unsigned int *ern)
{
  ASSERT(ern);
  
  *ern = 0;
  
  HTTP_SKIP_LWS(data, pos, len);

  if (!len)
    return (FALSE);
  
  if (0) { }
  else if (VPREFIX(data, pos, len, ";"))
  {
    size_t zeros  = 0;
    size_t digits = 0;
    int lead_zero = TRUE;
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "q"))
      goto syntax_error;
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "="))
      goto syntax_error;
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);

    /* if it's 0[.000] TRUE, 0[.\d\d\d] or 1[.000] is FALSE */
    if (!(lead_zero = VPREFIX(data, pos, len, "0")) &&
        !VPREFIX(data, pos, len, "1"))
      goto syntax_error;
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
    if (!len)
      return (lead_zero);
    if (VPREFIX(data, pos, len, ","))
      return (lead_zero);

    if (!VPREFIX(data, pos, len, "."))
      goto syntax_error;
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);

    digits = vstr_spn_cstr_chrs_fwd(data, pos, len, "0123456789");
    if (digits != 3)
      goto syntax_error;
    
    zeros  = vstr_spn_cstr_chrs_fwd(data, pos, len, "0");
    if (digits != zeros)
    {
      if (!lead_zero)
        goto syntax_error;
      else
        lead_zero = FALSE;
    }
    ASSERT(zeros <= digits);
    len -= digits; pos += digits;
    
    HTTP_SKIP_LWS(data, pos, len);
    if (!len)
      return (lead_zero);
    if (VPREFIX(data, pos, len, ","))
      return (lead_zero);
  }
  else if (VPREFIX(data, pos, len, ","))
    return (FALSE);

 syntax_error:
  *ern = 1;
  return (FALSE);
}

static int http_parse_accept_encoding_gzip(struct Con *con,
                                           struct Http_req_data *req)
{
  Vstr_base *data = con->ev->io_r;
  size_t pos = 0;
  size_t len = 0;
  int ret = FALSE;
  
  if (!serv->use_gzip_content_replacement)
    return (FALSE);

  pos = con->http_hdrs->hdr_accept_encoding->pos;
  len = con->http_hdrs->hdr_accept_encoding->len;

  if (!len)
    return (FALSE);
  
  req->content_encoding_xgzip = FALSE;
  
  /* search for end of current token */
  while (len)
  {
    unsigned int ern = 0;
    size_t tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len,
                                         HTTP_EOL HTTP_LWS ";,");
    
    if (0) { }
    else if (vstr_cmp_cstr_eq(data, pos, tmp, "gzip"))
    {
      len -= tmp; pos += tmp;
      return (!http_parse_quality_zero(data, pos, len, &ern) && !ern);
    }
    else if (vstr_cmp_cstr_eq(data, pos, tmp, "x-gzip"))
    {
      req->content_encoding_xgzip = TRUE;
      return (TRUE); /* ignore quality on x-gzip */
    }
    else if (vstr_cmp_cstr_eq(data, pos, tmp, "*"))
    { /* "*;q=0,gzip" means TRUE ... and "*;q=1,gzip;q=0" means FALSE */
      len -= tmp; pos += tmp;
      ret = !http_parse_quality_zero(data, pos, len, &ern);
      if (ern)
        return (FALSE);
    }
    else
      len -= tmp; pos += tmp;

    /* skip to end, or after next ',' */
    tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len, ",");    
    len -= tmp; pos += tmp;
    if (!len)
      break;
    assert(VPREFIX(data, pos, len, ","));
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
  }

  return (ret);
}

static int serv_http_req_1_x(struct Con *con, struct Http_req_data *req,
                             unsigned int *http_ret_code,
                             const char **http_ret_line)
{
  Vstr_base *data = con->ev->io_r;
  Vstr_base *out = con->ev->io_w;
  VSTR_AUTOCONF_uintmax_t range_beg = 0;
  VSTR_AUTOCONF_uintmax_t range_end = 0;
  VSTR_AUTOCONF_uintmax_t f_len     = 0;
  time_t now = time(NULL);
  Vstr_sect_node *h_r = con->http_hdrs->hdr_range;
  
  if (req->ver_1_1 && con->http_hdrs->hdr_expect->len)
    /* I'm pretty sure we can ignore 100-continue, as no request will
     * have a body */
    HTTPD_ERR_RET(req, 417, FALSE);
          
  if (h_r->pos)
  {
    int ret_code = 0;

    if (!(serv->use_range && (req->ver_1_1 || serv->use_range_1_0)))
      h_r->pos = 0;
    else if (!(ret_code = serv_http_parse_range(data, h_r->pos, h_r->len,
                                                req->f_stat->st_size,
                                                &range_beg, &range_end)))
      h_r->pos = 0;
    ASSERT(!ret_code || (ret_code == 200) || (ret_code == 416));
    if (ret_code == 416)
    {
      if (!con->http_hdrs->hdr_if_range->pos)
        HTTPD_ERR_RET(req, 416, FALSE);
      h_r->pos = 0;
    }
  }

  /* would normally add "!req->head_op && ..." but
   * http://www.w3.org/TR/chips/#gl6 says that's bad */
  if (!h_r->pos && http_parse_accept_encoding_gzip(con, req))
  { /* If not doing range requests, try a gzip content-encoding on body */
    const char *fname_cstr = NULL;
    int fd = -1;
      
    vstr_add_cstr_ptr(req->fname, req->fname->len, ".gz");

    fname_cstr = vstr_export_cstr_ptr(req->fname, 1, req->fname->len);
    if (!fname_cstr)
      vlg_warn(vlg, "Failed to export cstr for gzip\n");
    else if ((fd = io_open_nonblock(fname_cstr)) != -1)
    {
      struct stat64 f_stat[1];
      
      if (fstat64(fd, f_stat) == -1)
        vlg_warn(vlg, "fstat: %m\n");
      else if ((serv->use_public_only && !(f_stat->st_mode & S_IROTH)) ||
               (S_ISDIR(f_stat->st_mode)) || (!S_ISREG(f_stat->st_mode)) ||
               (req->f_stat->st_mtime >  f_stat->st_mtime) ||
               (req->f_stat->st_size  <= f_stat->st_size))
      { /* ignore the gzip else */ }
      else
      {
        int tmp = fd; /* swap, close the old (later) and use the new */
        fd = con->f_fd;
        con->f_fd = tmp;
        
        ASSERT(con->f_len == (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);
        /* only copy the new size over, mtime etc. is from the original */
        con->f_len = req->f_stat->st_size = f_stat->st_size;
        req->content_encoding_gzip = TRUE;
      }
      close(fd);
    }
  }
  
  if (!http_app_response_ok(con, req, now, http_ret_code, http_ret_line))
    HTTPD_ERR_RET(req, 412, FALSE);

  if (h_r->pos)
    f_len = (range_end - range_beg) + 1;
  else
  {
    range_beg = 0;
    f_len     = con->f_len;
  }
  ASSERT(con->f_len >= f_len);

  if (req->head_op)
    con->f_len = f_len;
  else
  {
    serv_conf_file(con, req, range_beg, f_len);
    serv_call_mmap(con, req, range_beg, f_len);
    serv_call_seek(con, req, range_beg, f_len, http_ret_code, http_ret_line);
  }

  vstr_add_fmt(out, out->len, "%s %03d %s" HTTP_EOL, "HTTP/1.1",
               *http_ret_code, *http_ret_line);
  http_app_def_hdrs(con, req, now, req->f_stat->st_mtime, NULL, con->f_len);
  if (h_r->pos)
    http_app_header_fmt(out, "Content-Range",
                        "%s %ju-%ju/%ju", "bytes", range_beg, range_end,
                        (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);
  
  http_app_end_hdrs(out);
  
  return (TRUE);
}

static void http_vlg_def(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->ev->io_r;
  Vstr_sect_node *h_h  = con->http_hdrs->hdr_host;
  Vstr_sect_node *h_ua = con->http_hdrs->hdr_ua;
  Vstr_sect_node *h_r  = con->http_hdrs->hdr_referer;

  vlg_info(vlg, (" host[\"$<http-esc.vstr:%p%zu%zu>\"]"
                 " UA[\"$<http-esc.vstr:%p%zu%zu>\"]"
                 " ref[\"$<http-esc.vstr:%p%zu%zu>\"]"),
           data, h_h->pos, h_h->len,    
           data, h_ua->pos, h_ua->len,
           data, h_r->pos, h_r->len);

  if (req->ver_0_9)
    vlg_info(vlg, " ver[\"HTTP/0.9]\"");
  else
    vlg_info(vlg, " ver[\"$<vstr.sect:%p%p%u>\"]", data, req->sects, 3);

  if (req->path_pos)
    vlg_info(vlg, ": $<http-esc.vstr:%p%zu%zu>\n",
             data, req->path_pos, req->path_len);
  else
    vlg_info(vlg, ": $<http-esc.vstr.sect:%p%p%u>\n", data, req->sects, 2);
}

static int http_req_make_path(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->ev->io_r;
  Vstr_base *fname = req->fname;
  
  ASSERT(!fname->len);

  assert(VPREFIX(data, req->path_pos, req->path_len, "/"));

  /* don't allow encoded slashes */
  if (vstr_srch_case_cstr_buf_fwd(data, req->path_pos, req->path_len, "%2f"))
    HTTPD_ERR_RET(req, 403, FALSE);
  
  vstr_add_vstr(fname, 0,
                data, req->path_pos, req->path_len, VSTR_TYPE_ADD_BUF_PTR);
  vstr_conv_decode_uri(fname, 1, fname->len);

  if (fname->conf->malloc_bad) /* dealt with in req_op_get */
    return (TRUE);
  
  /* FIXME: maybe call stat() to see if vhost dir exists ? ---
   * maybe when have stat() cache */

  
  if (serv->use_vhosts && con->http_hdrs->hdr_host->len)
  { /* add as buf's, for lowercase op */
    Vstr_sect_node *h_h  = con->http_hdrs->hdr_host;
    
    if (vstr_add_vstr(fname, 0, data, h_h->pos, h_h->len, VSTR_TYPE_ADD_DEF))
      vstr_conv_lowercase(fname, 1, h_h->len);
    vstr_add_cstr_ptr(fname, 0, "/");
  }
  else if (serv->use_vhosts)
  { /* if vhost but no host header, "default" dir is it */
    vstr_add_vstr(fname, 0, data,
                  con->http_hdrs->hdr_host->pos,
                  con->http_hdrs->hdr_host->len, VSTR_TYPE_ADD_BUF_PTR);
    vstr_add_cstr_ptr(fname, 0, "/default");
  }
  
  /* check path ... */
  if (vstr_srch_chr_fwd(fname, 1, fname->len, 0))
    HTTPD_ERR_RET(req, 403, FALSE);
      
  if (vstr_srch_cstr_buf_fwd(fname, 1, fname->len, "/../"))
    HTTPD_ERR_RET(req, 403, FALSE);

  ASSERT(fname->len);
  assert(VPREFIX(fname, 1, fname->len, "/") || fname->conf->malloc_bad);
      
  if (vstr_export_chr(fname, fname->len) == '/')
    vstr_add_cstr_ptr(fname, fname->len, serv->dir_filename);

  return (TRUE);
}

static void serv_fd_close(struct Con *con)
{
  con->f_len = 0;
  if (con->f_fd != -1)
    close(con->f_fd);
  con->f_fd = -1;
}

static int serv_send(struct Con *con); /* fwd reference */

static int http_parse_wait_io_r(struct Con *con)
{
  if (con->ev->io_r_shutdown)
    return (FALSE);

  evnt_fd_set_cork(con->ev, FALSE);
  return (TRUE);
}

static struct Http_req_data *http_make_req(struct Con *con)
{
  static struct Http_req_data real_req[1];
  struct Http_req_data *req = real_req;

  ASSERT(!req->using_req);

  if (!req->done_once)
  {
    Vstr_conf *conf = NULL;

    if (con)
      conf = con->ev->io_w->conf;
      
    if (!(req->fname = vstr_make_base(conf)) ||
        !(req->sects = vstr_sects_make(8)))
      return (NULL);
  }
  
  vstr_del(req->fname, 1, req->fname->len);
  req->len = 0;

  req->path_pos = 0;
  req->path_len = 0;

  req->error_code = 0;
  req->error_line = "";
  req->error_msg  = "";
  req->error_len  = 0;

  req->sects->num = 0;
  /* f_stat */
  if (con)
    req->orig_io_w_len = con->ev->io_w->len;

  if (req->f_mmap)
    vstr_del(req->f_mmap, 1, req->f_mmap->len);

  req->content_type_vs1 = NULL;
  req->content_type_pos = 0;
  req->content_type_len = 0;
  
  req->sects->malloc_bad = FALSE;

  req->advertise_accept_ranges = CONF_SERV_USE_RANGE;
  req->content_encoding_gzip   = FALSE;
  
  req->ver_0_9    = FALSE;
  req->ver_1_1    = FALSE;
  req->use_mmap   = FALSE;
  req->head_op    = FALSE;

  req->done_once  = TRUE;
  req->using_req  = TRUE;

  req->malloc_bad = FALSE;

  return (req);
}

static void http_req_free(struct Http_req_data *req)
{
  if (!req) /* for if/when move to malloc/free */
    return;

  ASSERT(req->done_once && req->using_req);

  vstr_del(req->fname, 1, req->fname->len); /* free mem */
  if (req->f_mmap)
    vstr_del(req->f_mmap, 1, req->f_mmap->len);
  req->content_type_vs1 = NULL;
    
  req->using_req = FALSE;
}

static void http_req_exit(void)
{
  struct Http_req_data *req = http_make_req(NULL);

  if (!req)
    return;
  
  vstr_free_base(req->fname);  req->fname  = NULL;
  vstr_free_base(req->f_mmap); req->f_mmap = NULL;
  vstr_sects_free(req->sects); req->sects  = NULL;
  
  req->done_once = FALSE;
  req->using_req = FALSE;
}

static int http_con_cleanup(struct Con *con, struct Http_req_data *req)
{
  con->ev->io_r->conf->malloc_bad = FALSE;
  con->ev->io_w->conf->malloc_bad = FALSE;
  http_req_free(req);
    
  return (FALSE);
}

static int http_fin_req(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *out = con->ev->io_w;
  
  ASSERT(!out->conf->malloc_bad);
  serv_clear_hdrs(con);

  /* Note: Not resetting con->parsed_method_ver_1_0,
   * if it's non-0.9 now it should continue to be */
  
  if (!con->keep_alive) /* all input is here */
  {
    evnt_wait_cntl_del(con->ev, POLLIN);
    req->len = con->ev->io_r->len; /* delete it all */
  }
  
  vstr_del(con->ev->io_r, 1, req->len);
  
  http_req_free(req);

  evnt_fd_set_cork(con->ev, TRUE);
  return (serv_send(con));
}

static int http_fin_err_req(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *out = con->ev->io_w;

  req->content_encoding_gzip = FALSE;
  
  if ((req->error_code == 400) || (req->error_code == 405) ||
      (req->error_code == 413) ||
      (req->error_code == 500) || (req->error_code == 501))
    con->keep_alive = HTTP_NIL_KEEP_ALIVE;
  
  ASSERT(!con->f_len);
  
  vlg_info(vlg, "ERREQ from[$<sa:%p>] err[%03u %s]",
           con->ev->sa, req->error_code, req->error_line);
  if (req->sects->num >= 2)
    http_vlg_def(con, req);
  else
    vlg_info(vlg, "%s", "\n");

  if (req->malloc_bad)
  { /* delete all input to give more room */
    ASSERT(req->error_code == 500);
    vstr_del(con->ev->io_r, 1, con->ev->io_r->len);
  }
  
  if (!req->ver_0_9)
  {
    vstr_add_fmt(out, out->len, "%s %03u %s" HTTP_EOL,
                 "HTTP/1.1", req->error_code, req->error_line);
    http_app_def_hdrs(con, req, time(NULL), server_beg,
                      "text/html", req->error_len);
    if (req->error_code == 416)
      http_app_header_fmt(out, "Content-Range", "%s */%ju", "bytes",
                          (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);

    if ((req->error_code == 405) || (req->error_code == 501))
      http_app_header_cstr(out, "Allow", "GET, HEAD, TRACE");
    if (req->error_code == 301)
    { /* make sure we haven't screwed up and allowed response splitting */
      Vstr_base *loc = req->fname;
      
      ASSERT(!vstr_srch_cstr_chrs_fwd(loc, 1, loc->len, HTTP_EOL));
      http_app_header_vstr(out, "Location",
                           loc, 1, loc->len, VSTR_TYPE_ADD_ALL_BUF);
    }
    http_app_end_hdrs(out);
  }

  if (!req->head_op)
  {
    Vstr_base *loc = req->fname;
    
    if (req->error_code == 301)
      vstr_add_fmt(out, out->len, CONF_MSG_FMT_301,
                   CONF_MSG__FMT_301_BEG,
                   loc, (size_t)1, loc->len, VSTR_TYPE_ADD_ALL_BUF,
                   CONF_MSG__FMT_301_END);
    else
      vstr_add_ptr(out, out->len, req->error_msg, req->error_len);
  }
  
  vlg_dbg2(vlg, "ERROR REPLY:\n$<vstr:%p%zu%zu>\n", out, (size_t)1, out->len);

  if (out->conf->malloc_bad)
    return (http_con_cleanup(con, req));
  
  return (http_fin_req(con, req));
}

static int http_fin_err_close_req(struct Con *con, struct Http_req_data *req)
{
  serv_fd_close(con);
  return (http_fin_err_req(con, req));
}

static int http_fin_errmem_req(struct Con *con, struct Http_req_data *req)
{ /* try sending a 500 as the last msg */
  Vstr_base *out = con->ev->io_w;
  
  /* remove anything we can to free space */
  vstr_sc_reduce(out, 1, out->len, out->len - req->orig_io_w_len);
  
  con->ev->io_r->conf->malloc_bad = FALSE;
  con->ev->io_w->conf->malloc_bad = FALSE;
  req->malloc_bad = TRUE;
  
  HTTPD_ERR_RET(req, 500, http_fin_err_req(con, req));
}

static int http_fin_errmem_close_req(struct Con *con, struct Http_req_data *req)
{
  serv_fd_close(con);
  return (http_fin_errmem_req(con, req));
}

static int http__req_redirect_dir(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->ev->io_r;
  Vstr_sect_node *h_h = con->http_hdrs->hdr_host;
  Vstr_base *lfn = req->fname;

  vstr_del(lfn, 1, lfn->len);
  
  if (!h_h->len) /* this is valid AFAICS... */
    vstr_add_cstr_buf(lfn, lfn->len, "http:");
  else
  { /* but we'll do it "normally" if we have a host */
    vstr_add_cstr_buf(lfn, lfn->len, "http://");
    vstr_add_vstr(lfn, lfn->len,
                  data, h_h->pos, h_h->len, VSTR_TYPE_ADD_ALL_BUF);
  }
  
  vstr_add_vstr(lfn, lfn->len, con->ev->io_r, req->path_pos, req->path_len,
                VSTR_TYPE_ADD_ALL_BUF);

  if (VSUFFIX(lfn, 1, lfn->len, "/"))
    vstr_add_cstr_buf(lfn, lfn->len, serv->dir_filename);
  
  vstr_add_cstr_buf(lfn, lfn->len, "/");
  
  req->error_code = 301;
  req->error_line = CONF_LINE_RET_301;
  req->error_len  = CONF_MSG_LEN_301(lfn);
  
  return (http_fin_err_req(con, req));
}

/* doing http://www.example.com/foo/bar where bar is a dir is bad
   because all relative links will be relative to foo, not bar.
   Also note that location must be "http://www.example.com/foo/bar/" or maybe
   "http:/foo/bar/"
*/
static int http__req_chk_dir(struct Con *con, struct Http_req_data *req)
{
  ASSERT(req->fname->len && (vstr_export_chr(req->fname, 1) != '/') &&
         (vstr_export_chr(req->fname, req->fname->len) != '/'));
  
  if (VSUFFIX(req->fname, 1, req->fname->len, "/..") ||
      vstr_cmp_cstr_eq(req->fname, 1, req->fname->len, ".."))
    HTTPD_ERR_RET(req, 403, http_fin_err_req(con, req));
  
  return (http__req_redirect_dir(con, req));
}

/* Lookup content type for filename, If this lookup "fails" it still returns
 * the default content-type. So we just have to determine if we want to use
 * it or not. Can also return "content-types" like /404/ which returns a 404
 * error for the request */
static int http__req_content_type(struct Http_req_data *req)
{
  Vstr_base *vs1 = NULL;
  size_t     pos = 0;
  size_t     len = 0;
  int ret = mime_types_match(req->fname, 1, req->fname->len, &vs1, &pos, &len);

  if (!serv->use_default_mime_type && !ret)
    return (TRUE);

  if ((vstr_export_chr(vs1, pos) == '/') && (len > 2) &&
      (vstr_export_chr(vs1, vstr_sc_poslast(pos, len)) == '/'))
  {
    size_t num_len = 1;

    len -= 2;
    ++pos;
    switch (vstr_parse_uint(vs1, pos, len, 0, &num_len, NULL))
    {
      case 400: if (num_len == len) HTTPD_ERR_RET(req, 400, FALSE);
      case 403: if (num_len == len) HTTPD_ERR_RET(req, 403, FALSE);
      case 404: if (num_len == len) HTTPD_ERR_RET(req, 404, FALSE);
      case 410: if (num_len == len) HTTPD_ERR_RET(req, 410, FALSE);
      case 500: if (num_len == len) HTTPD_ERR_RET(req, 500, FALSE);
      case 503: if (num_len == len) HTTPD_ERR_RET(req, 503, FALSE);
        
      default: /* just ignore everything else */
        return (TRUE);
    }
  }
  
  req->content_type_vs1 = vs1;
  req->content_type_pos = pos;
  req->content_type_len = len;

  return (TRUE);
}

static int http_req_op_get(struct Con *con, struct Http_req_data *req)
{ /* GET or HEAD ops */
  Vstr_base *data = con->ev->io_r;
  Vstr_base *out  = con->ev->io_w;
  Vstr_base *fname = req->fname;
  const char *fname_cstr = NULL;
  unsigned int http_ret_code = 200;
  const char * http_ret_line = "OK";
  
  if (fname->conf->malloc_bad)
    VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_get(FN): %m\n"));

  if (!http__req_content_type(req))
    return (http_fin_err_req(con, req));

  /* change "///foo" into "foo", in case no chroot ... this is done now
   * as mime_type is done on full path name */
  vstr_del(fname, 1, vstr_spn_cstr_chrs_fwd(fname, 1, fname->len, "/"));

  fname_cstr = vstr_export_cstr_ptr(fname, 1, fname->len);
  
  if (fname->conf->malloc_bad)
    VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_get(FN): %m\n"));
  
  if ((con->f_fd = io_open_nonblock(fname_cstr)) == -1)
  {
    if (0) { }
    else if (errno == EISDIR)
      return (http__req_chk_dir(con, req));
    else if (errno == EACCES)
      HTTPD_ERR(req, 403);
    else if ((errno == ENOENT) ||
             (errno == ENODEV) ||
             (errno == ENXIO) ||
             (errno == ELOOP) ||
             (errno == ENAMETOOLONG) ||
             FALSE)
      HTTPD_ERR(req, 404);
    else if (errno == ENAMETOOLONG)
      HTTPD_ERR(req, 414);
    else
      HTTPD_ERR(req, 500);
    
    return (http_fin_err_req(con, req));
  }
  if (fstat64(con->f_fd, req->f_stat) == -1)
    HTTPD_ERR_RET(req, 500, http_fin_err_close_req(con, req));
  if (serv->use_public_only && !(req->f_stat->st_mode & S_IROTH))
    HTTPD_ERR_RET(req, 403, http_fin_err_close_req(con, req));
  
  if (S_ISDIR(req->f_stat->st_mode))
  {
    serv_fd_close(con);
    return (http__req_chk_dir(con, req));
  }
  if (!S_ISREG(req->f_stat->st_mode))
    HTTPD_ERR_RET(req, 403, http_fin_err_close_req(con, req));
  con->f_len = req->f_stat->st_size;

  if (req->ver_0_9)
  {
    serv_conf_file(con, req, 0, con->f_len);
    serv_call_mmap(con, req, 0, con->f_len);
    http_ret_line = "OK - HTTP/0.9";
  }
  else if (!serv_http_req_1_x(con, req, &http_ret_code, &http_ret_line))
    return (http_fin_err_close_req(con, req));
  
  if (out->conf->malloc_bad)
    VLG_WARNNOMEM_RET(http_fin_errmem_close_req(con,req), (vlg,"op_get: %m\n"));
  
  vlg_dbg2(vlg, "REPLY:\n$<vstr:%p%zu%zu>\n", out, (size_t)1, out->len);
  
  if (req->use_mmap && !vstr_mov(con->ev->io_w, con->ev->io_w->len,
                                 req->f_mmap, 1, req->f_mmap->len))
    VLG_WARNNOMEM_RET(http_fin_errmem_close_req(con, req), (vlg, "mmap: %m\n"));

  /* req->head_op is set for 304 returns */
  vlg_info(vlg, "REQ $<vstr.sect:%p%p%u> from[$<sa:%p>] ret[%03u %s]"
           " sz[${BKMG.ju:%ju}:%ju]", data, req->sects, 1, con->ev->sa,
           http_ret_code, http_ret_line, con->f_len, con->f_len);
  http_vlg_def(con, req);
  
  if (req->head_op || req->use_mmap)
    serv_fd_close(con);
  
  return (http_fin_req(con, req));
}

static int http_parse_no_req(struct Con *con, struct Http_req_data *req)
{
  if (server_max_header_sz && (con->ev->io_r->len > server_max_header_sz))
    HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

  http_req_free(req);
  
  return (http_parse_wait_io_r(con));
}

/* http spec says to ignore leading blank lines ... *sigh* */
#define HTTP__PARSE_REQ_ALL(x) do {                                     \
                                                                        \
  if (!(req->len = vstr_srch_cstr_buf_fwd(data, 1, data->len, x)))      \
    return (http_parse_no_req(con, req));                               \
                                                                        \
  if (req->len == 1)                                                    \
  {                                                                     \
    while (VPREFIX(data, 1, data->len, HTTP_EOL))                       \
      vstr_del(data, 1, strlen(HTTP_EOL));                              \
                                                                        \
    if (!(req->len = vstr_srch_cstr_buf_fwd(data, 1, data->len, x)))    \
      return (http_parse_no_req(con, req));                             \
                                                                        \
    ASSERT(req->len > 1);                                               \
  }                                                                     \
                                                                        \
  req->len += strlen(x) - 1;                                            \
 } while (FALSE)

static int http_parse_req(struct Con *con)
{
  Vstr_base *data = con->ev->io_r;
  Vstr_base *out  = con->ev->io_w;
  struct Http_req_data *req = NULL;

  ASSERT(!con->f_len);

  if (!(req = http_make_req(con)))
    return (FALSE);

  if (con->parsed_method_ver_1_0) /* wait for all the headers */
    HTTP__PARSE_REQ_ALL(HTTP_END_OF_REQUEST);
  else
    HTTP__PARSE_REQ_ALL(HTTP_EOL);

  con->keep_alive = HTTP_NIL_KEEP_ALIVE;
  http_req_split_method(con, req);
  if (req->sects->malloc_bad)
    VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "split: %m\n"));
  else if (req->sects->num < 2)
    HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));
  else
  {
    size_t op_pos = 0;
    size_t op_len = 0;

    if (!req->ver_0_9)
    { /* need to get all headers */
      if (!con->parsed_method_ver_1_0)
      {
        con->parsed_method_ver_1_0 = TRUE;
        HTTP__PARSE_REQ_ALL(HTTP_END_OF_REQUEST);
      }
      
      http_req_split_hdrs(con, req);
    }
    
    assert(((req->sects->num >= 3) && !req->ver_0_9) || (req->sects->num == 2));
    
    op_pos        = VSTR_SECTS_NUM(req->sects, 1)->pos;
    op_len        = VSTR_SECTS_NUM(req->sects, 1)->len;
    req->path_pos = VSTR_SECTS_NUM(req->sects, 2)->pos;
    req->path_len = VSTR_SECTS_NUM(req->sects, 2)->len;

    if (!req->ver_0_9 && !http_req_parse_1_x(con, req))
      return (http_fin_err_req(con, req));
      
    if (!serv_http_absolute_uri(con, req))
      HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

    if (0) { }
    else if (vstr_cmp_cstr_eq(data, op_pos, op_len, "GET"))
    {
      if (!http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_get(con, req));      
    }
    else if (req->ver_0_9) /* 400 or 501? - apache does 400 */
      HTTPD_ERR_RET(req, 501, http_fin_err_req(con, req));      
    else if (vstr_cmp_cstr_eq(data, op_pos, op_len, "HEAD"))
    {
      req->head_op = TRUE;
      
      if (!http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_get(con, req));      
    }
    else if (vstr_cmp_cstr_eq(data, op_pos, op_len, "TRACE"))
    {
      time_t now = time(NULL);
      VSTR_AUTOCONF_uintmax_t tmp = 0;
      
      vstr_add_fmt(out, out->len, "%s %03d %s" HTTP_EOL, "HTTP/1.1", 200, "OK");
      http_app_def_hdrs(con, req, now, now, "message/http", req->len);
      http_app_end_hdrs(out);
      vstr_add_vstr(out, out->len, data, 1, req->len, VSTR_TYPE_ADD_DEF);
      if (out->conf->malloc_bad)
        VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "TRACE: %m\n"));

      tmp = req->len;
      vlg_info(vlg, "REQ %s from[$<sa:%p>] ret[%03u %s] sz[${BKMG.ju:%ju}:%ju]",
               "TRACE", con->ev->sa, 200, "OK", tmp, tmp);
      http_vlg_def(con, req);
      
      return (http_fin_req(con, req));
    }
    else if (vstr_cmp_cstr_eq(data, op_pos, op_len, "OPTIONS") ||
             vstr_cmp_cstr_eq(data, op_pos, op_len, "POST") ||
             vstr_cmp_cstr_eq(data, op_pos, op_len, "PUT") ||
             vstr_cmp_cstr_eq(data, op_pos, op_len, "DELETE") ||
             vstr_cmp_cstr_eq(data, op_pos, op_len, "CONNECT") ||
             FALSE) /* we know about these ... but don't allow them */
      HTTPD_ERR_RET(req, 405, http_fin_err_req(con, req));
    else
      HTTPD_ERR_RET(req, 501, http_fin_err_req(con, req));
  }
  ASSERT_NOT_REACHED();
}

/* FIXME: create a Ref for large amounts */
static void serv_send_zeropad(struct Con *con)
{
  size_t len = CONF_ZEROPAD_SZ;
    
  if (len > con->f_len)
    len = con->f_len;
  
  vstr_add_ref(con->ev->io_w, con->ev->io_w->len, zeropad_data, 0, len);
  con->f_len -= len;
}

static int serv_q_send(struct Con *con)
{
  vlg_dbg3(vlg, "Q $<sa:%p>\n", con->ev->sa);
  if (!evnt_send_add(con->ev, TRUE, CL_MAX_WAIT_SEND))
  {
    if (errno != EPIPE)
      vlg_warn(vlg, "send q: %m\n");
    return (FALSE);
  }
      
  /* queued */
  return (TRUE);
}

static int serv_fin_send(struct Con *con)
{
  if (con->keep_alive)
  { /* need to try immediately, as we might have already got the next req */
    if (!con->ev->io_r_shutdown)
      evnt_wait_cntl_add(con->ev, POLLIN);
    return (http_parse_req(con));
  }
    
  return (FALSE);
}

/* try sending a bunch of times, bail out if we've done a few ... keep going
 * if we have too much data */
#define SERV__SEND(x) do {                    \
    if (!--num)                               \
      return (serv_q_send(con));              \
                                              \
    if (!evnt_send(con->ev))                  \
    {                                         \
      if (errno != EPIPE)                     \
        vlg_warn(vlg, "send(%s): %m\n", x);   \
      return (FALSE);                         \
    }                                         \
 } while (out->len >= EX_MAX_W_DATA_INCORE)

static int serv_send(struct Con *con)
{
  Vstr_base *out = con->ev->io_w;
  size_t len = 0;
  int ret = IO_OK;
  unsigned int num = CONF_FS_READ_CALL_LIMIT;
  
  ASSERT(!out->conf->malloc_bad);

  if (out->len)
    SERV__SEND("beg");
    
  if (con->f_fd == -1)
    while (con->f_len)
    {
      serv_send_zeropad(con);
      if (out->conf->malloc_bad)
      {
        out->conf->malloc_bad = FALSE;
        VLG_WARNNOMEM_RET((FALSE), (vlg, "zeropad: %m\n"));
      }

      SERV__SEND("zero");
    }
  
  if (!con->f_len)
  {
    while (out->len)
      SERV__SEND("end");
    return (serv_fin_send(con));
  }

  if (con->use_sendfile)
  {
    unsigned int ern = 0;
    
    while (con->f_len)
    {
      if (!--num)
        return (serv_q_send(con));
      
      if (!evnt_sendfile(con->ev, con->f_fd, &con->f_off, &con->f_len, &ern))
      {
        VSTR_AUTOCONF_uintmax_t f_off = con->f_off;

        if (ern == VSTR_TYPE_SC_READ_FD_ERR_EOF)
          goto file_eof;
        
        if (errno == ENOSYS) /* also logs it */
          serv->use_sendfile = FALSE;
        if ((errno == EPIPE) || (errno == ECONNRESET))
          return (FALSE);
        else
          vlg_warn(vlg, "sendfile: %m\n");

        if (lseek64(con->f_fd, f_off, SEEK_SET) == -1)
          VLG_WARN_RET(FALSE, (vlg, "lseek(<sendfile>,off=%ju): %m\n", f_off));

        con->use_sendfile = FALSE;
        return (serv_send(con));
      }
    }
    
    goto file_eof;
  }
  
  ASSERT(con->f_len);
  len = out->len;
  while ((ret = io_get(out, con->f_fd)) != IO_FAIL)
  {
    size_t tmp = out->len - len;

    if (ret == IO_EOF)
      goto file_eof;
    
    if (tmp < con->f_len)
      con->f_len -= tmp;
    else
    { /* we might not be transfering to EOF, so reduce if needed */
      vstr_sc_reduce(out, 1, out->len, tmp - con->f_len);
      ASSERT((out->len - len) == con->f_len);
      con->f_len = 0;
      goto file_eof;
    }

    SERV__SEND("io_get");

    len = out->len;
  }

  vlg_warn(vlg, "read: %m\n");
  out->conf->malloc_bad = FALSE;

 file_eof:
  ASSERT(con->f_fd != -1);
  close(con->f_fd);
  con->f_fd = -1;

  return (serv_send(con)); /* restart and zeropad, or finish */
}

static int serv_cb_func_send(struct Evnt *evnt)
{
  return (serv_send((struct Con *)evnt));
}

static int serv_recv(struct Con *con)
{
  unsigned int ern = 0;
  int ret = 0;
  Vstr_base *data = con->ev->io_r;

  ASSERT(!con->ev->io_r_shutdown);
  
  if (!(ret = evnt_recv(con->ev, &ern)))
  {
    vlg_dbg3(vlg, "RECV ERR from[$<sa:%p>]: %u\n", con->ev->sa, ern);
    
    if (ern != VSTR_TYPE_SC_READ_FD_ERR_EOF)
      goto con_cleanup;
    if (!evnt_shutdown_r(con->ev))
      goto con_cleanup;
  }
    
  if (con->f_len) /* need to stop input, until we can get rid of it */
  {
    ASSERT(con->keep_alive || con->parsed_method_ver_1_0);
    
    if (server_max_header_sz && (data->len > server_max_header_sz))
      evnt_wait_cntl_del(con->ev, POLLIN);

    return (TRUE);
  }
  
  if (http_parse_req(con))
    return (TRUE);
  
 con_cleanup:
  con->ev->io_r->conf->malloc_bad = FALSE;
  con->ev->io_w->conf->malloc_bad = FALSE;
    
  return (FALSE);
}

static int serv_cb_func_recv(struct Evnt *evnt)
{
  return (serv_recv((struct Con *)evnt));
}

static void serv_cb_func_free(struct Evnt *evnt)
{
  struct Con *con = (struct Con *)evnt;

  if (acpt_evnt && server_max_clients && (evnt_num_all() < server_max_clients))
    evnt_wait_cntl_add(acpt_evnt, POLLIN);

  serv_fd_close(con);

  vlg_info(vlg, "FREE from[$<sa:%p>]"
           " recv[${BKMG.ju:%ju}:%ju] send[${BKMG.ju:%ju}:%ju]\n", con->ev->sa,
           con->ev->acct.bytes_r, con->ev->acct.bytes_r,
           con->ev->acct.bytes_w, con->ev->acct.bytes_w);

  free(con);
}

static void serv_cb_func_acpt_free(struct Evnt *evnt)
{
  vlg_info(vlg, "ACCEPT FREE from[$<sa:%p>]\n", evnt->sa);

  ASSERT(acpt_evnt == evnt);

  acpt_evnt = NULL;
  cntl_free_acpt(evnt);
  
  free(evnt);
}

static struct Evnt *serv_cb_func_accept(int fd,
                                        struct sockaddr *sa, socklen_t len)
{
  struct Con *con = NULL;
  struct timeval tv;

  ASSERT(acpt_evnt);
  
  if (server_max_clients && (evnt_num_all() >= server_max_clients))
    evnt_wait_cntl_del(acpt_evnt, POLLIN);
  
  if (sa->sa_family != AF_INET) /* only support IPv4 atm. */
    goto make_acpt_fail;
  
  if (!(con = malloc(sizeof(struct Con))) ||
      !evnt_make_acpt(con->ev, fd, sa, len))
    goto make_acpt_fail;

  tv = con->ev->ctime;
  TIMER_Q_TIMEVAL_ADD_SECS(&tv, 0, client_timeout);

  con->f_fd  = -1;
  con->f_len =  0;
  
  con->parsed_method_ver_1_0 = FALSE;
  con->keep_alive = HTTP_NIL_KEEP_ALIVE;
  con->use_sendfile  = serv->use_sendfile;
  
  serv_clear_hdrs(con);
  
  if (!(con->ev->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                         TIMER_Q_FLAG_NODE_DEFAULT)))
    goto timer_add_fail;

  vlg_info(vlg, "CONNECT from[$<sa:%p>]\n", con->ev->sa);
  
  con->ev->cbs->cb_func_recv       = serv_cb_func_recv;
  con->ev->cbs->cb_func_send       = serv_cb_func_send;
  con->ev->cbs->cb_func_free       = serv_cb_func_free;

  return (con->ev);

 timer_add_fail:
  evnt_free(con->ev);
 make_acpt_fail:
  free(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, "%s: %m\n", "accept"));
}

static void serv_make_bind(const char *acpt_addr, short acpt_port)
{
  if (!(acpt_evnt = malloc(sizeof(struct Evnt))))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", __func__));
  
  if (!evnt_make_bind_ipv4(acpt_evnt, acpt_addr, acpt_port))
    vlg_err(vlg, 2, "%s: %m\n", __func__);
  
  acpt_evnt->cbs->cb_func_accept = serv_cb_func_accept;
  acpt_evnt->cbs->cb_func_free   = serv_cb_func_acpt_free;

  if (serv->defer_accept)
    evnt_fd_set_defer_accept(acpt_evnt, serv->defer_accept);
}

static void serv_filter_attach(int fd, const char *fname)
{
  static int done = FALSE;
  Vstr_base *s1 = NULL;
  unsigned int ern = 0;

  if (!CONF_SERV_USE_SOCKET_FILTERS)
    return;
  
  if (!(s1 = vstr_make_base(NULL)))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "filter_attach: %m\n"));

  vstr_sc_read_len_file(s1, 0, fname, 0, 0, &ern);

  if (ern &&
      ((ern != VSTR_TYPE_SC_READ_FILE_ERR_OPEN_ERRNO) || (errno != ENOENT)))
    vlg_err(vlg, EXIT_FAILURE, "filter_attach(%s): %m\n", fname);

  if ((s1->len / sizeof(struct sock_filter)) > USHRT_MAX)
  {
    errno = E2BIG;
    vlg_err(vlg, EXIT_FAILURE, "filter_attach(%s): %m\n", fname);
  }
  
  if (!s1->len)
  {
    if (!done)
      vlg_warn(vlg, "filter_attach(%s): Empty file\n", fname);
    else
    {
      if (setsockopt(fd, SOL_SOCKET, SO_DETACH_FILTER, NULL, 0) == -1)
        vlg_err(vlg, EXIT_FAILURE,
                "setsockopt(SOCKET, DETACH_FILTER, %s): %m\n", fname);
    }
  }
  else
  {
    struct sock_fprog filter[1];
    socklen_t len = sizeof(filter);

    filter->len    = s1->len / sizeof(struct sock_filter);
    filter->filter = (void *)vstr_export_cstr_ptr(s1, 1, s1->len);
    
    if (!filter->filter)
      VLG_ERRNOMEM((vlg, EXIT_FAILURE, "filter_attach: %m\n"));
  
    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, filter, len) == -1)
      vlg_err(vlg, EXIT_FAILURE,
              "setsockopt(SOCKET, ATTACH_FILTER, %s): %m\n", fname);
  }

  done = TRUE;
  
  vstr_free_base(s1);
}

static void cl_cmd_line(int argc, char *argv[])
{
  char optchar = 0;
  const char *program_name = NULL;
  struct option long_options[] =
  {
   {"help", no_argument, NULL, 'h'},
   {"daemon", optional_argument, NULL, 1},
   {"chroot", required_argument, NULL, 2},
   {"drop-privs", optional_argument, NULL, 3},
   {"priv-uid", required_argument, NULL, 4},
   {"priv-gid", required_argument, NULL, 5},
   {"pid-file", required_argument, NULL, 6},
   {"cntl-file", required_argument, NULL, 7},
   {"acpt-filter-file", required_argument, NULL, 8},
   {"accept-filter-file", required_argument, NULL, 8},
   {"sendfile", optional_argument, NULL, 31},
   {"mmap", optional_argument, NULL, 30},
   {"keep-alive", optional_argument, NULL, 29},
   {"keep-alive-1.0", optional_argument, NULL, 28},
   {"vhosts", optional_argument, NULL, 27},
   {"virtual-hosts", optional_argument, NULL, 27},
   {"range", optional_argument, NULL, 26},
   {"range-1.0", optional_argument, NULL, 25},
   {"public-only", optional_argument, NULL, 24},
   {"dir-filename", required_argument, NULL, 23},
   {"server-name", required_argument, NULL, 22},
   {"gzip-content-replacement", optional_argument, NULL, 21},
   {"send-default-mime-type", optional_argument, NULL, 20},
   {"default-mime-type", required_argument, NULL, 19},
   {"mime-types-main", required_argument, NULL, 18},
   {"mime-types-extra", required_argument, NULL, 17},
   {"mime-types-xtra", required_argument, NULL, 17},
   {"processes", required_argument, NULL, 16},
   {"procs", required_argument, NULL, 16},
   {"defer-accept", required_argument, NULL, 15},
   /* {"404-file", required_argument, NULL, 0}, */
   {"debug", no_argument, NULL, 'd'},
   {"host", required_argument, NULL, 'H'},
   {"port", required_argument, NULL, 'P'},
   {"nagle", optional_argument, NULL, 'n'},
   {"max-clients", required_argument, NULL, 'M'},
   {"max-header-sz", required_argument, NULL, 128},
   {"timeout", required_argument, NULL, 't'},
   {"version", no_argument, NULL, 'V'},
   {NULL, 0, NULL, 0}
  };
  OPT_SC_SERV_DECL_OPTS();

  /* FIXME: TCP_CORK doesn't work if off, but only want it on for http evnt's */
  evnt_opt_nagle = TRUE;
  
  program_name = opt_program_name(argv[0], "jhttpd");
  
  while ((optchar = getopt_long(argc, argv, "dhH:M:nP:t:V",
                                long_options, NULL)) != -1)
  {
    switch (optchar)
    {
      case '?':
        usage(program_name, TRUE,  "");
      case 'h':
        usage(program_name, FALSE, "");
        
      case 'V':
      {
        Vstr_base *out = vstr_make_base(NULL);
        
        if (!out)
          errno = ENOMEM, err(EXIT_FAILURE, "usage");

        vstr_add_fmt(out, 0, " %s version %s.\n",
                     program_name, CONF_SERV_VERSION);

        if (io_put_all(out, STDOUT_FILENO) == IO_FAIL)
          err(EXIT_FAILURE, "write");

        exit (EXIT_SUCCESS);
      }
      
      case 't': client_timeout       = atoi(optarg);  break;
      case 'H': server_ipv4_address  = optarg;        break;
      case 128: server_max_header_sz = atoi(optarg);  break;
      case 'M': server_max_clients   = atoi(optarg);  break;
      case 'P': server_port          = atoi(optarg);  break;
        
      case 'd': vlg_debug(vlg);                       break;
        
      case 'n': OPT_TOGGLE_ARG(evnt_opt_nagle); break;

        OPT_SC_SERV_OPTS();
        
      case 31: OPT_TOGGLE_ARG(serv->use_sendfile);                 break;
      case 30: OPT_TOGGLE_ARG(serv->use_mmap);                     break;
      case 29: OPT_TOGGLE_ARG(serv->use_keep_alive);               break;
      case 28: OPT_TOGGLE_ARG(serv->use_keep_alive_1_0);           break;
      case 27: OPT_TOGGLE_ARG(serv->use_vhosts);                   break;
      case 26: OPT_TOGGLE_ARG(serv->use_range);                    break;
      case 25: OPT_TOGGLE_ARG(serv->use_range_1_0);                break;
      case 24: OPT_TOGGLE_ARG(serv->use_public_only);              break;
      case 23: serv->dir_filename = optarg;                        break;
      case 22: serv->server_name  = optarg;                        break;
      case 21: OPT_TOGGLE_ARG(serv->use_gzip_content_replacement); break;
      case 20: OPT_TOGGLE_ARG(serv->use_default_mime_type);        break;
      case 19: serv->mime_types_default_type = optarg;             break;
      case 18: serv->mime_types_main = optarg;                     break;
      case 17: serv->mime_types_xtra = optarg;                     break;
      case 16:
      {
        unsigned int num = atoi(optarg);
        if (!num || (num > 255))
          usage(program_name, TRUE,
                " The number of processes must be in the range 1 to 255.\n");
        serv->num_procs = num;
      }
      break;
      
      case 15:
      {
        unsigned int num = atoi(optarg);
        if (num > 16384)
          usage(program_name, TRUE,
                " The seconds to defer connections must be in the range 0 to 16384.\n");
        serv->defer_accept = num;
      }
        
      ASSERT_NO_SWITCH_DEF();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 1)
    usage(program_name, EXIT_FAILURE, " Too many arguments.\n");

  if (!mime_types_init(serv->mime_types_default_type))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (!mime_types_load_simple(serv->mime_types_main))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (!mime_types_load_simple(serv->mime_types_xtra))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (become_daemon)
  {
    if (daemon(FALSE, FALSE) == -1)
      err(EXIT_FAILURE, "daemon");
    vlg_daemon(vlg, program_name);
  }

  serv_make_bind(server_ipv4_address, server_port);

  if (pid_file)
    vlg_pid_file(vlg, pid_file);

  if (cntl_file)
    cntl_init(vlg, cntl_file, acpt_evnt);
  
  if (acpt_filter_file)
    serv_filter_attach(evnt_fd(acpt_evnt), acpt_filter_file);
  
  if (chroot_dir)
  { /* preload locale info. so syslog can log in localtime */
    time_t now = time(NULL);
    (void)localtime(&now);
  }
  
  /* after daemon so syslog works */
  if (chroot_dir && ((chroot(chroot_dir) == -1) || (chdir("/") == -1)))
    vlg_err(vlg, EXIT_FAILURE, "chroot(%s): %m\n", chroot_dir);
  
  if (chdir(argv[0]) == -1)
    vlg_err(vlg, EXIT_FAILURE, "chdir(%s): %m\n", argv[0]);

  if (drop_privs)
  {
    if (setgroups(1, &priv_gid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setgroups(%ld): %m\n", (long)priv_gid);
    
    if (setgid(priv_gid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setgid(%ld): %m\n", (long)priv_gid);
    
    if (setuid(priv_uid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setuid(%ld): %m\n", (long)priv_uid);
  }

  if (serv->num_procs > 1)
  {
    unsigned int num = serv->num_procs;
    int pfds[2];

    vlg->log_pid_stdout = TRUE;
    
    if (pipe(pfds) == -1)
      vlg_err(vlg, EXIT_FAILURE, "pipe(): %m\n");
    
    while (--num)
    {
      pid_t cpid = fork();

      if (cpid == -1)
        vlg_err(vlg, EXIT_FAILURE, "fork(): %m\n");

      if (!cpid)
      { /* child */
        cntl_pipe_acpt_fds(pfds[0], -1);
        close(pfds[1]); pfds[1] = -1;
        break;
      }
    }

    /*  We "leak" the writable side of the pipe fd in the parent, which will
     * cause an error on all the children when the parent dies. */
    if (pfds[1] != -1)
      close(pfds[0]);
  }

  if (!vstr_make_spare_nodes(NULL, VSTR_TYPE_NODE_BUF,
                             (CONF_MEM_PREALLOC_MIN / CONF_BUF_SZ)))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  server_beg = time(NULL);
  
  vlg_info(vlg, "READY [$<sa:%p>]: %s%s%s\n", acpt_evnt->sa,
           chroot_dir ? chroot_dir : "",
           chroot_dir ?        "/" : "",
           argv[0]);
}

int main(int argc, char *argv[])
{
  serv_init();

  cl_cmd_line(argc, argv);

  while (evnt_waiting() && !child_exited)
  {
    int ready = evnt_poll();
    struct timeval tv;
    
    if ((ready == -1) && (errno != EINTR))
      vlg_err(vlg, EXIT_FAILURE, "%s: %m\n", "poll");
    if (ready == -1)
      continue;
    
    evnt_out_dbg3("1");
    evnt_scan_fds(ready, CL_MAX_WAIT_SEND);
    evnt_out_dbg3("2");
    evnt_scan_send_fds();
    evnt_out_dbg3("3");
    
    gettimeofday(&tv, NULL);
    timer_q_run_norm(&tv);

    evnt_out_dbg3("4");
    evnt_scan_send_fds();
    evnt_out_dbg3("5");

    serv_cntl_resources();
  }
  evnt_out_dbg3("E");

  timer_q_del_base(cl_timeout_base);

  evnt_close_all();
  
  http_req_exit();

  mime_types_exit();
  
  vlg_free(vlg);
  
  vlg_exit();

  vstr_ref_del(zeropad_data);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
