/*
 *  Copyright (C) 2004, 2005  James Antill
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  email: james@and.org
 */
/* conditionally compliant HTTP/1.1 server. */
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

#define CONF_SERV_VERSION "0.9.5"

/* **** compile time conf **** */


/* **** defaults for runtime conf **** */
#define CONF_SERV_DEF_NAME "jhttpd/" CONF_SERV_VERSION " (Vstr)"
#define CONF_SERV_USE_MMAP FALSE
#define CONF_SERV_USE_SENDFILE TRUE
#define CONF_SERV_USE_KEEPA TRUE
#define CONF_SERV_USE_KEEPA_1_0 TRUE
#define CONF_SERV_USE_VHOSTS FALSE
#define CONF_SERV_USE_RANGE TRUE
#define CONF_SERV_USE_RANGE_1_0 TRUE
#define CONF_SERV_USE_PUBLIC_ONLY FALSE
#define CONF_SERV_USE_GZIP_CONTENT_REPLACEMENT TRUE
#define CONF_SERV_DEF_TCP_DEFER_ACCEPT 16 /* HC usage txt */
#define CONF_SERV_DEF_DIR_FILENAME "index.html"
#define CONF_SERV_USE_DEFAULT_MIME_TYPE FALSE
#define CONF_SERV_DEF_MIME_TYPE "application/octet-stream"
#define CONF_SERV_MIME_TYPE_MAIN "/etc/mime.types"
#define CONF_SERV_MIME_TYPE_XTRA NULL
#define CONF_SERV_USE_ERR_406 TRUE
#define CONF_SERV_USE_CANONIZE_HOST FALSE
#define CONF_SERV_USE_NON_HOST_ERR_400 TRUE

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

/* Linear Whitespace, a full stds. check for " " and "\t" */
#define HTTP_LWS " \t"

/* End of Line */
#define HTTP_EOL "\r\n"

/* End of Request - blank line following previous line in request */
#define HTTP_END_OF_REQUEST HTTP_EOL HTTP_EOL

/* HTTP crack -- Implied linear whitespace between tokens, note that it
 * is *LWS == *([CRLF] 1*(SP | HT)) */
#define HTTP_SKIP_LWS(s1, p, l) do {                                    \
      char http__q_tst_lws = 0;                                         \
                                                                        \
      if (!l) break;                                                    \
      http__q_tst_lws = vstr_export_chr(s1, p);                         \
      if ((http__q_tst_lws != '\r') && (http__q_tst_lws != ' ') &&      \
          (http__q_tst_lws != '\t'))                                    \
        break;                                                          \
                                                                        \
      http__skip_lws(s1, &p, &l);                                       \
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

/* for simplicity */
#define VEQ(vstr, p, l, cstr) vstr_cmp_cstr_eq(vstr, p, l, cstr)
#define VIEQ(vstr, p, l, cstr) vstr_cmp_case_cstr_eq(vstr, p, l, cstr)

#define TRUE  1
#define FALSE 0

struct Http_hdrs
{
 Vstr_sect_node hdr_ua[1];
 Vstr_sect_node hdr_referer[1]; /* NOTE: referrer */

 Vstr_sect_node hdr_expect[1];
 Vstr_sect_node hdr_host[1];
 Vstr_sect_node hdr_if_match[1];
 Vstr_sect_node hdr_if_modified_since[1];
 Vstr_sect_node hdr_if_none_match[1];
 Vstr_sect_node hdr_if_range[1];
 Vstr_sect_node hdr_if_unmodified_since[1];

 /* can have multiple headers... */
 struct Http_hdrs__multi {
  Vstr_base *combiner_store;
  Vstr_base *comb;
  Vstr_sect_node hdr_accept[1];
  Vstr_sect_node hdr_accept_encoding[1];
  Vstr_sect_node hdr_accept_language[1];
  Vstr_sect_node hdr_connection[1];
  Vstr_sect_node hdr_range[1];
 } multi[1];
};

struct Http_req_data
{
 struct Http_hdrs http_hdrs[1];
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
 Vstr_base *content_location;
 unsigned int ver_0_9 : 1;
 unsigned int ver_1_1 : 1;
 unsigned int use_mmap : 1;
 unsigned int head_op : 1;

 unsigned int content_encoding_gzip  : 1;
 unsigned int content_encoding_xgzip : 1; /* only valid if gzip is TRUE */
 
 unsigned int content_encoding_identity : 1;

 unsigned int using_req : 1;
 unsigned int done_once : 1;

 unsigned int malloc_bad : 1;
};

struct Con
{
 struct Evnt evnt[1];

 int f_fd;
 VSTR_AUTOCONF_uintmax_t f_off;
 VSTR_AUTOCONF_uintmax_t f_len;

 unsigned int parsed_method_ver_1_0 : 1;
 unsigned int keep_alive : 3;
 
 unsigned int use_sendfile : 1; 
};

#define HTTP_NIL_KEEP_ALIVE 0
#define HTTP_1_0_KEEP_ALIVE 1
#define HTTP_1_1_KEEP_ALIVE 2

#define HTTP__HDR_SET(req, h, p, l) do {               \
      (req)-> http_hdrs -> hdr_ ## h ->pos = (p);          \
      (req)-> http_hdrs -> hdr_ ## h ->len = (l);          \
    } while (FALSE)
#define HTTP__HDR_MULTI_SET(req, h, p, l) do {         \
      (req)-> http_hdrs -> multi -> hdr_ ## h ->pos = (p); \
      (req)-> http_hdrs -> multi -> hdr_ ## h ->len = (l); \
    } while (FALSE)

static struct Evnt *acpt_evnt = NULL;

static unsigned int client_timeout = (2 * 60); /* 2 minutes */

static const char *server_ipv4_address = CONF_SERV_DEF_ADDR;
static short server_port = CONF_SERV_DEF_PORT;

static unsigned int server_max_clients = 0;

static unsigned int server_max_header_sz = CONF_INPUT_MAXSZ;

static time_t server_beg = (time_t)-1;

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
 unsigned int defer_accept : 12; /* 0 => 4095 (1hr8m) (10th-22nd bitfields) */
 unsigned int use_err_406 : 1; /* 23rd bitfield */
 unsigned int canonize_host : 1;
 unsigned int non_host_err_400 : 1; /* 25th bitfield */
 
 const char * dir_filename;
 const char * mime_types_default_type;
 const char * mime_types_main;
 const char * mime_types_xtra;
 Vstr_base *default_hostname;
} serv[1] = {{CONF_SERV_DEF_NAME,
              CONF_SERV_USE_MMAP, CONF_SERV_USE_SENDFILE,
              CONF_SERV_USE_KEEPA, CONF_SERV_USE_KEEPA_1_0,
              CONF_SERV_USE_VHOSTS,
              CONF_SERV_USE_RANGE, CONF_SERV_USE_RANGE_1_0,
              CONF_SERV_USE_PUBLIC_ONLY,
              CONF_SERV_USE_GZIP_CONTENT_REPLACEMENT,
              CONF_SERV_USE_DEFAULT_MIME_TYPE,
              CONF_SERV_DEF_TCP_DEFER_ACCEPT,
              CONF_SERV_USE_ERR_406,
              CONF_SERV_USE_CANONIZE_HOST,
              CONF_SERV_USE_NON_HOST_ERR_400,
              CONF_SERV_DEF_DIR_FILENAME,
              CONF_SERV_DEF_MIME_TYPE,
              CONF_SERV_MIME_TYPE_MAIN,
              CONF_SERV_MIME_TYPE_XTRA,
              NULL}};

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
    --accept-filter-file\n\
                      - Load Linux Socket Filter code for accept().\n\
    --processes       - Number of processes to use (default: 1).\n\
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
    --send-err-406    - Toggle sending 406 responses%s.\n\
    --canonize-host   - Strip leading 'www.', strip trailing '.'%s.\n\
    --err-host-400    - Give an 400 error for a bad host%s.\n\
    --defer-accept    - Time to defer dataless connections (default: 16s).\n\
    --default-hostname\n\
                      - Hostname used when not supplied (default is hostname).\n\
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
               opt_def_toggle(CONF_SERV_USE_VHOSTS),
               opt_def_toggle(CONF_SERV_USE_RANGE),
               opt_def_toggle(CONF_SERV_USE_RANGE_1_0),
               opt_def_toggle(CONF_SERV_USE_PUBLIC_ONLY),
               opt_def_toggle(CONF_SERV_USE_GZIP_CONTENT_REPLACEMENT),
               opt_def_toggle(CONF_SERV_USE_DEFAULT_MIME_TYPE),
               opt_def_toggle(CONF_SERV_USE_ERR_406),
               opt_def_toggle(CONF_SERV_USE_CANONIZE_HOST),
               opt_def_toggle(CONF_SERV_USE_NON_HOST_ERR_400),
               opt_def_toggle(evnt_opt_nagle));

  if (io_put_all(out, ret ? STDERR_FILENO : STDOUT_FILENO) == IO_FAIL)
    err(EXIT_FAILURE, "write");
  
  exit (ret);
}

#define SERV__SIG_OR_ERR(x)                     \
    if (sigaction(x, &sa, NULL) == -1)          \
      err(EXIT_FAILURE, "signal(" #x ")")

static void serv__sig_crash(int s_ig_num)
{ /* strsignal() is _GNU_SOURCE */
  vlg_abort(vlg, "SIG[%d]: %s\n", s_ig_num, "");
}

static void serv__sig_raise_cont(int s_ig_num)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    err(EXIT_FAILURE, "signal init");

  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = SIG_DFL;
  SERV__SIG_OR_ERR(s_ig_num);

  /* strsignal() is _GNU_SOURCE */
  vlg_info(vlg, "SIG[%d]: %s\n", s_ig_num, "");
  raise(s_ig_num);
}

static void serv__sig_cont(int s_ig_num)
{
  if (s_ig_num == SIGCONT)
  {
    struct sigaction sa;
  
    if (sigemptyset(&sa.sa_mask) == -1)
      err(EXIT_FAILURE, "signal init");

    sa.sa_flags   = SA_RESTART;
    sa.sa_handler = serv__sig_raise_cont;
    SERV__SIG_OR_ERR(SIGTSTP);
  }
  
  /* strsignal() is _GNU_SOURCE */
  vlg_info(vlg, "SIG[%d]: %s\n", s_ig_num, "");
}

static volatile sig_atomic_t child_exited = FALSE;
static void serv__sig_child(int s_ig_num)
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
  
  SERV__SIG_OR_ERR(SIGPIPE);

  sa.sa_handler = serv__sig_crash;
  
  SERV__SIG_OR_ERR(SIGSEGV);
  SERV__SIG_OR_ERR(SIGBUS);
  SERV__SIG_OR_ERR(SIGILL);
  SERV__SIG_OR_ERR(SIGFPE);
  SERV__SIG_OR_ERR(SIGXFSZ);

  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = serv__sig_child;
  SERV__SIG_OR_ERR(SIGCHLD);
  
  sa.sa_handler = serv__sig_cont; /* print, and do nothing */
  
  SERV__SIG_OR_ERR(SIGHUP);
  SERV__SIG_OR_ERR(SIGCONT);
  
  sa.sa_handler = serv__sig_raise_cont; /* queue print, and re-raise */
  
  SERV__SIG_OR_ERR(SIGTSTP);
  
  /*
  sa.sa_handler = ex_http__sig_shutdown;
  if (sigaction(SIGTERM, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  */
}
#undef SERV__SIG_OR_ERR


/* HTTP crack -- Implied linear whitespace between tokens, note that it
 * is *LWS == *([CRLF] 1*(SP | HT)) */
static void http__skip_lws(Vstr_base *s1, size_t *pos, size_t *len)
{
  size_t lws__len = 0;
  
  ASSERT(s1 && pos && len);
  
  while (TRUE)
  {
    if (VPREFIX(s1, *pos, *len, HTTP_EOL))
    {
      *len -= strlen(HTTP_EOL); *pos += strlen(HTTP_EOL);
    }
    else if (lws__len)
      break;
    
    if (!(lws__len = vstr_spn_cstr_chrs_fwd(s1, *pos, *len, HTTP_LWS)))
      break;
    *len -= lws__len;
    *pos += lws__len;
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
  Vstr_base *tmp = NULL;
  
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

  if (!(tmp = vstr_make_base(NULL)))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  else
  {
    char buf[256];

    if (gethostname(buf, sizeof(buf)) == -1)
      err(EXIT_FAILURE, "gethostname");
    
    buf[255] = 0;
    vstr_add_cstr_buf(tmp, 0, buf);
    vstr_conv_lowercase(tmp, 1, tmp->len);
  }
  if (tmp->conf->malloc_bad)
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  serv->default_hostname = tmp;
  
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
  evnt_timeout_init();
  
  serv_signals();
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
  Vstr_base *s1 = con->evnt->io_r;
  size_t pos = 1;
  size_t len = req->len;
  size_t el = 0;
  size_t skip_len = 0;
  unsigned int orig_num = req->sects->num;
  
  el = vstr_srch_cstr_buf_fwd(s1, pos, len, HTTP_EOL);
  ASSERT(el >= pos);
  len = el - pos; /* only parse the first line */

  /* split request */
  if (!(el = vstr_srch_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
    return;
  vstr_sects_add(req->sects, pos, el - pos);
  len -= (el - pos); pos += (el - pos);

  /* just skip whitespace on method call... */
  if ((skip_len = vstr_spn_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
  { len -= skip_len; pos += skip_len; }

  if (!len)
  {
    req->sects->num = orig_num;
    return;
  }
  
  if (!(el = vstr_srch_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
  {
    vstr_sects_add(req->sects, pos, len);
    len = 0;
  }
  else
  {
    vstr_sects_add(req->sects, pos, el - pos);
    len -= (el - pos); pos += (el - pos);
    
    /* just skip whitespace on method call... */
    if ((skip_len = vstr_spn_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
    { len -= skip_len; pos += skip_len; }
  }

  if (len)
    vstr_sects_add(req->sects, pos, len);
  else
    req->ver_0_9 = TRUE;
}

static void http_req_split_hdrs(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *s1 = con->evnt->io_r;
  size_t pos = 1;
  size_t len = req->len;
  size_t el = 0;
  size_t hpos = 0;

  ASSERT(req->sects->num >= 3);  
    
  /* skip first line */
  el = (VSTR_SECTS_NUM(req->sects, req->sects->num)->pos +
        VSTR_SECTS_NUM(req->sects, req->sects->num)->len);
  
  assert(VEQ(s1, el, strlen(HTTP_EOL), HTTP_EOL));
  len -= (el - pos) + strlen(HTTP_EOL); pos += (el - pos) + strlen(HTTP_EOL);
  
  if (VPREFIX(s1, pos, len, HTTP_EOL))
    return; /* end of headers */

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
                              unsigned int http_ret_code,
                              const char *http_ret_line,
                              time_t now, time_t mtime,
                              const char *custom_content_type,
                              VSTR_AUTOCONF_uintmax_t content_length)
{
  Vstr_base *out = con->evnt->io_w;

  vstr_add_fmt(out, out->len, "%s %03u %s" HTTP_EOL,
               "HTTP/1.1", http_ret_code, http_ret_line);
  http_app_header_cstr(out, "Date", date_rfc1123(now));
  http_app_header_cstr(out, "Server", serv->server_name);
  
  if (difftime(now, mtime) < 0) /* if mtime in future, chop it #14.29 */
    mtime = now;

  if (mtime)
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
  
  if (serv->use_range)
    http_app_header_cstr(out, "Accept-Ranges", "bytes");
  if (serv->use_gzip_content_replacement)
    http_app_header_cstr(out, "Vary", "Accept-Encoding");
  
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

#if 0
/* see rfc2616 3.3.1 -- full date parser */
static time_t http_parse_date(Vstr_base *s1, size_t pos, size_t len, time_t now)
{
  struct tm *tm = gmtime(&now);
  static const char http__date_days_shrt[4][7] =
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char http__date_days_full[10][7] =
    {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
     "Saturday"};
  static const char http__date_months[4][12] =
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
     "Sep", "Oct", "Nov", "Dec"};
  unsigned int scan = 0;

  if (!tm) return (-1);

  switch (len)
  {
    case 4 + 1 + 9 + 1 + 8 + 1 + 3: /* rfc1123 format - should be most common */
    {
      scan = 0;
      while (scan < 7)
      {
        if (VPREFIX(s1, pos, len, http__date_days_shrt[scan]))
          break;
        ++scan;
      }
      len -= 3; pos += 3;
      
      if (!VPREFIX(s1, pos, len, ", "))
        return (-1);
      len -= strlen(", "); pos += strlen(", ");

      tm->tm_mday = http__date_parse_2d(s1, pos, len, 1, 31);
      
      if (!VPREFIX(s1, pos, len, " "))
        return (-1);
      len -= strlen(" "); pos += strlen(" ");

      scan = 0;
      while (scan < 12)
      {
        if (VPREFIX(s1, pos, len, http__date_months[scan]))
          break;
        ++scan;
      }
      len -= 3; pos += 3;
      
      tm->tm_mon = scan;
      
      if (!VPREFIX(s1, pos, len, " "))
        return (-1);
      len -= strlen(" "); pos += strlen(" ");

      tm->tm_year = http__date_parse_4d(s1, pos, len);

      if (!VPREFIX(s1, pos, len, " "))
        return (-1);
      len -= strlen(" "); pos += strlen(" ");

      tm->tm_hour = http__date_parse2d(s1, pos, len, 0, 23);
      if (!VPREFIX(s1, pos, len, ":"))
        return (-1);
      len -= strlen(":"); pos += strlen(":");
      tm->tm_min  = http__date_parse2d(s1, pos, len, 0, 59);
      if (!VPREFIX(s1, pos, len, ":"))
        return (-1);
      len -= strlen(":"); pos += strlen(":");
      tm->tm_sec  = http__date_parse2d(s1, pos, len, 0, 61);
      
      if (!VPREFIX(s1, pos, len, " GMT"))
        return (-1);
    }
    return (mktime(tm));

    case  7 + 1 + 7 + 1 + 8 + 1 + 3:
    case  8 + 1 + 7 + 1 + 8 + 1 + 3:
    case  9 + 1 + 7 + 1 + 8 + 1 + 3:
    case 10 + 1 + 7 + 1 + 8 + 1 + 3: /* rfc850 format */
    {
      size_t match_len = 0;
      
      scan = 0;
      while (scan < 7)
      {
        match_len = strlen(http__date_days_full[scan]);
        if (VPREFIX(s1, pos, len, http__date_days_full[scan]))
          break;
        ++scan;
      }
      len -= match_len; pos += match_len;

      return (-1);
    }
    return (mktime(tm));

    case  3 + 1 + 6 + 1 + 8 + 1 + 4: /* asctime format */
    {
      scan = 0;
      while (scan < 7)
      {
        if (VPREFIX(s1, pos, len, http__date_days_shrt[scan]))
          break;
        ++scan;
      }
      len -= 3; pos += 3;
      
      return (-1);
    }
    return (mktime(tm));
  }
  
  return (-1);  
}
#endif

/* gets here if the GET/HEAD response is ok, we test for caching etc. using the
 * if-* headers */
/* FALSE = 412 Precondition Failed */
static int http_response_ok(struct Con *con, struct Http_req_data *req,
                            time_t now,
                            unsigned int *http_ret_code,
                            const char ** http_ret_line)
{
  Vstr_base *hdrs = con->evnt->io_r;
  time_t mtime = req->f_stat->st_mtime;
  Vstr_sect_node *h_im   = req->http_hdrs->hdr_if_match;
  Vstr_sect_node *h_ims  = req->http_hdrs->hdr_if_modified_since;
  Vstr_sect_node *h_inm  = req->http_hdrs->hdr_if_none_match;
  Vstr_sect_node *h_ir   = req->http_hdrs->hdr_if_range;
  Vstr_sect_node *h_iums = req->http_hdrs->hdr_if_unmodified_since;
  Vstr_sect_node *h_r    = req->http_hdrs->multi->hdr_range;
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
  if (h_ims->pos && VEQ(hdrs, h_ims->pos,  h_ims->len,  date))
    cached_output = TRUE;
  if (h_iums_tst && VEQ(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst   && VEQ(hdrs, h_ir->pos,   h_ir->len,   date))
    req_if_range = TRUE;
  
  date = date_rfc850(mtime);
  if (h_ims->pos && VEQ(hdrs, h_ims->pos,  h_ims->len,  date))
    cached_output = TRUE;
  if (h_iums_tst && VEQ(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst   && VEQ(hdrs, h_ir->pos,   h_ir->len,   date))
    req_if_range = TRUE;
  
  date = date_asctime(mtime);
  if (h_ims->pos && VEQ(hdrs, h_ims->pos,  h_ims->len,  date))
    cached_output = TRUE;
  if (h_iums_tst && VEQ(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst   && VEQ(hdrs, h_ir->pos,   h_ir->len,   date))
    req_if_range = TRUE;

  if (h_ir_tst && !req_if_range)
    h_r->pos = 0;
  
  if (req->ver_1_1)
  {
    if (h_inm->pos &&  VEQ(hdrs, h_inm->pos, h_inm->len, "*"))
      cached_output = TRUE;

    if (h_im->pos  && !VEQ(hdrs, h_inm->pos, h_inm->len, "*"))
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

static void http__multi_hdr_fixup(Vstr_sect_node *hdr_ignore,
                                  Vstr_sect_node *hdr, size_t pos, size_t len)
{
  if (hdr == hdr_ignore)
    return;
  
  if (hdr->pos <= pos)
    return;

  hdr->pos += len;
}

static int http__multi_hdr_cp(Vstr_base *comb,
                              Vstr_base *data, Vstr_sect_node *hdr)
{
  size_t pos = comb->len + 1;

  if (!hdr->len)
    return (TRUE);
  
  if (!vstr_add_vstr(comb, comb->len,
                     data, hdr->pos, hdr->len, VSTR_TYPE_ADD_BUF_PTR))
    return (FALSE);

  hdr->pos = pos;
  
  return (TRUE);
}

static int http__app_multi_hdr(Vstr_base *data, struct Http_hdrs *hdrs,
                               Vstr_sect_node *hdr, size_t pos, size_t len)
{
  Vstr_base *comb = hdrs->multi->comb;
  
  ASSERT(comb);
  
  ASSERT((hdr == hdrs->multi->hdr_accept) ||
         (hdr == hdrs->multi->hdr_accept_encoding) ||
         (hdr == hdrs->multi->hdr_accept_language) ||
         (hdr == hdrs->multi->hdr_connection) ||
         (hdr == hdrs->multi->hdr_range));

  ASSERT((comb == data) || (comb == hdrs->multi->combiner_store));
  
  if ((data == comb) && !hdr->pos)
  { /* Do the fast thing... */
    hdr->pos = pos;
    hdr->len = len;
    return (TRUE);
  }

  if (data == comb)
  { /* OK, so we have a crap request and need to JOIN multiple headers... */
    comb = hdrs->multi->comb = hdrs->multi->combiner_store;
  
    if (!http__multi_hdr_cp(comb, data, hdrs->multi->hdr_accept) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_accept_encoding) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_accept_language) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_connection) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_range) ||
        FALSE)
      return (FALSE);
  }
  
  if (!hdr->pos)
  {
    hdr->pos = comb->len + 1;
    hdr->len = len;
    return (vstr_add_vstr(comb, comb->len,
                          data, pos, len, VSTR_TYPE_ADD_BUF_PTR));
  }

  /* reverses the order, but that doesn't matter */
  if (!vstr_add_cstr_ptr(comb, hdr->pos - 1, ","))
    return (FALSE);
  if (!vstr_add_vstr(comb, hdr->pos - 1,
                     data, pos, len, VSTR_TYPE_ADD_BUF_PTR))
    return (FALSE);
  hdr->len += ++len;

  /* now need to "move" any hdrs after this one */
  pos = hdr->pos - 1;
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_accept,          pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_accept_encoding, pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_accept_language, pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_connection,      pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_range,           pos, len);
    
  return (TRUE);
}

/* viprefix, with local knowledge */
static int http__hdr_eq(Vstr_base *data, size_t pos, size_t len,
                        const char *hdr, size_t *hdr_val_pos)
{
  if (!VIPREFIX(data, pos, len, hdr))
    return (FALSE);
  len -= strlen(hdr); pos += strlen(hdr);

  HTTP_SKIP_LWS(data, pos, len);
  if (!len)
    return (FALSE);
  
  if (vstr_export_chr(data, pos) != ':')
    return (FALSE);
  --len; ++pos;
  
  *hdr_val_pos = pos;
  
  return (TRUE);
}

/* remove LWS from front and end... what a craptastic std. */
static void http__hdr_fixup(Vstr_base *data, size_t *pos, size_t *len,
                            size_t hdr_val_pos)
{
  size_t tmp = 0;
  
  *len -= hdr_val_pos - *pos; *pos += hdr_val_pos - *pos;
  HTTP_SKIP_LWS(data, *pos, *len);

  /* hand coding for a HTTP_SKIP_LWS() going backwards... */
  while ((tmp = vstr_spn_cstr_chrs_rev(data, *pos, *len, HTTP_LWS)))
  {
    *len -= tmp;
  
    if (VSUFFIX(data, *pos, *len, HTTP_EOL))
      *len -= strlen(HTTP_EOL);
  }  
}

#define HDR__EQ(x) http__hdr_eq(data, pos, len, x, &hdr_val_pos)
#define HDR__SET(h) do {                                                \
      http__hdr_fixup(data, &pos, &len, hdr_val_pos);                   \
      http_hdrs-> hdr_ ## h ->pos = pos;                                \
      http_hdrs-> hdr_ ## h ->len = len;                                \
    } while (FALSE)
#define HDR__MULTI_SET(h) do {                                          \
      http__hdr_fixup(data, &pos, &len, hdr_val_pos);                   \
      if (!http__app_multi_hdr(data, http_hdrs,                         \
                               http_hdrs->multi-> hdr_ ## h, pos, len)) \
        HTTPD_ERR_RET(req, 500, FALSE);                                 \
    } while (FALSE)

static int http__parse_hdrs(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  struct Http_hdrs *http_hdrs = req->http_hdrs;
  unsigned int num = 3; /* skip "method URI version" */
  
  while (++num <= req->sects->num)
  {
    size_t pos = VSTR_SECTS_NUM(req->sects, num)->pos;
    size_t len = VSTR_SECTS_NUM(req->sects, num)->len;
    size_t hdr_val_pos = 0;

    if (0) { /* nothing */ }
    /* single headers, isn't allowed ... we do last one wins */
    /* nothing headers ... use for logging only */
    else if (HDR__EQ("User-Agent"))          HDR__SET(ua);
    else if (HDR__EQ("Referer"))             HDR__SET(referer);

    else if (HDR__EQ("Expect"))              HDR__SET(expect);
    else if (HDR__EQ("Host"))                HDR__SET(host);
    else if (HDR__EQ("If-Match"))            HDR__SET(if_match);
    else if (HDR__EQ("If-Modified-Since"))   HDR__SET(if_modified_since);
    else if (HDR__EQ("If-None-Match"))       HDR__SET(if_none_match);
    else if (HDR__EQ("If-Range"))            HDR__SET(if_range);
    else if (HDR__EQ("If-Unmodified-Since")) HDR__SET(if_unmodified_since);

    /* allow continuations over multiple headers... *sigh* */
    else if (HDR__EQ("Accept"))              HDR__MULTI_SET(accept);
    else if (HDR__EQ("Accept-Encoding"))     HDR__MULTI_SET(accept_encoding);
    else if (HDR__EQ("Accept-Language"))     HDR__MULTI_SET(accept_language);
    else if (HDR__EQ("Connection"))          HDR__MULTI_SET(connection);
    else if (HDR__EQ("Range"))               HDR__MULTI_SET(range);

    /* in theory 0 bytes is ok ... who cares */
    else if (HDR__EQ("Content-Length"))      HTTPD_ERR_RET(req, 413, FALSE);

    /* in theory identity is ok ... who cares */
    else if (HDR__EQ("Transfer-Encoding"))   HTTPD_ERR_RET(req, 413, FALSE);
  }

  return (TRUE);
}
#undef HDR__EQ
#undef HDR__SET
#undef HDR__MUTLI_SET

static void http__clear_hdrs(struct Http_req_data *req)
{
  Vstr_base *tmp = req->http_hdrs->multi->combiner_store;

  ASSERT(tmp);
  
  HTTP__HDR_SET(req, ua,                  0, 0);
  HTTP__HDR_SET(req, referer,             0, 0);

  HTTP__HDR_SET(req, expect,              0, 0);
  HTTP__HDR_SET(req, host,                0, 0);
  HTTP__HDR_SET(req, if_match,            0, 0);
  HTTP__HDR_SET(req, if_modified_since,   0, 0);
  HTTP__HDR_SET(req, if_none_match,       0, 0);
  HTTP__HDR_SET(req, if_range,            0, 0);
  HTTP__HDR_SET(req, if_unmodified_since, 0, 0);

  vstr_del(tmp, 1, tmp->len);
  HTTP__HDR_MULTI_SET(req, accept,          0, 0);
  HTTP__HDR_MULTI_SET(req, accept_encoding, 0, 0);
  HTTP__HDR_MULTI_SET(req, accept_language, 0, 0);
  HTTP__HDR_MULTI_SET(req, connection,      0, 0);
  HTTP__HDR_MULTI_SET(req, range,           0, 0);
}

/* might have been able to do it with string matches, but getting...
 * "HTTP/1.1" = OK
 * "HTTP/1.10" = OK
 * "HTTP/1.10000000000000" = BAD
 * ...seemed not as easy. It also seems like you have to accept...
 * "HTTP / 01 . 01" as "HTTP/1.1"
 */
static int http_req_parse_version(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  size_t op_pos = VSTR_SECTS_NUM(req->sects, 3)->pos;
  size_t op_len = VSTR_SECTS_NUM(req->sects, 3)->len;
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

#define HDR__CON_1_0_FIXUP(name, h)                                     \
    else if (VIEQ(data, pos, tmp, name))                                \
      do {                                                              \
        req -> http_hdrs -> hdr_ ## h ->pos = 0;                        \
        req -> http_hdrs -> hdr_ ## h ->len = 0;                        \
      } while (FALSE)
#define HDR__CON_1_0_MULTI_FIXUP(name, h)                               \
    else if (VIEQ(data, pos, tmp, name))                                \
      do {                                                              \
        req -> http_hdrs -> multi -> hdr_ ## h ->pos = 0;               \
        req -> http_hdrs -> multi -> hdr_ ## h ->len = 0;               \
      } while (FALSE)
static void http__parse_connection(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = req->http_hdrs->multi->comb;
  size_t pos = 0;
  size_t len = 0;

  pos = req->http_hdrs->multi->hdr_connection->pos;
  len = req->http_hdrs->multi->hdr_connection->len;

  if (req->ver_1_1)
    con->keep_alive = HTTP_1_1_KEEP_ALIVE;

  if (!len)
    return;

  while (len)
  {
    size_t tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len,
                                         HTTP_EOL HTTP_LWS ",");

    if (req->ver_1_1)
    { /* this is all we have to do for HTTP/1.1 ... proxies understnad it */
      if (VIEQ(data, pos, tmp, "close"))
        con->keep_alive = HTTP_NIL_KEEP_ALIVE;
    }
    else if (VIEQ(data, pos, tmp, "keep-alive"))
    {
      if (serv->use_keep_alive_1_0)
        con->keep_alive = HTTP_1_0_KEEP_ALIVE;
    }
    /* now fixup connection headers for HTTP/1.0 proxies */
    HDR__CON_1_0_FIXUP("User-Agent",          ua);
    HDR__CON_1_0_FIXUP("Referer",             referer);
    HDR__CON_1_0_FIXUP("Expect",              expect);
    HDR__CON_1_0_FIXUP("Host",                host);
    HDR__CON_1_0_FIXUP("If-Match",            if_match);
    HDR__CON_1_0_FIXUP("If-Modified-Since",   if_modified_since);
    HDR__CON_1_0_FIXUP("If-None-Match",       if_none_match);
    HDR__CON_1_0_FIXUP("If-Range",            if_range);
    HDR__CON_1_0_FIXUP("If-Unmodified-Since", if_unmodified_since);
    
    HDR__CON_1_0_MULTI_FIXUP("Accept",          accept);
    HDR__CON_1_0_MULTI_FIXUP("Accept-Encoding", accept_encoding);
    HDR__CON_1_0_MULTI_FIXUP("Accept-Language", accept_language);
    HDR__CON_1_0_MULTI_FIXUP("Range",           range);

    /* skip to end, or after next ',' */
    tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len, ",");    
    len -= tmp; pos += tmp;
    if (!len)
      break;
    
    assert(VPREFIX(data, pos, len, ","));
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
  }
}
#undef HDR__CON_1_0_FIXUP
#undef HDR__CON_1_0_MULTI_FIXUP
                                  
/* parse >= 1.0 things like, version and headers */
static int http__parse_1_x(struct Con *con, struct Http_req_data *req)
{
  ASSERT(!req->ver_0_9);
  
  if (!http_req_parse_version(con, req))
    return (FALSE);
  
  if (!http__parse_hdrs(con, req))
    return (FALSE);

  if (!serv->use_keep_alive)
    return (TRUE);

  http__parse_connection(con, req);
  
  return (TRUE);
}

/* because we only parse for a combined CRLF, and some proxies/clients parse for
 * either ... make sure we don't have enbedded ones to remove possability
 * of request splitting */
static int http__chk_single_crlf(Vstr_base *data, size_t pos, size_t len)
{
  if (vstr_srch_chr_fwd(data, pos, len, '\r') ||
      vstr_srch_chr_fwd(data, pos, len, '\n'))
    return (TRUE);

  return (FALSE);
}

/* convert a http://abcd/foo into /foo with host=abcd ...
 * also do sanity checking on the URI and host for valid characters */
static int http_absolute_uri(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
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
      HTTP__HDR_SET(req, host, op_pos, op_len);
      op_len = 1;
      --op_pos;
    }
    else
    { /* found end of host ... */
      size_t host_len = tmp - op_pos;
      
      HTTP__HDR_SET(req, host, op_pos, host_len);
      op_len -= host_len; op_pos += host_len;
    }
    assert(VPREFIX(data, op_pos, op_len, "/"));
  }

  /* HTTP/1.1 requires a host -- allow blank hostnames */
  if (req->ver_1_1 && !req->http_hdrs->hdr_host->pos)
    return (FALSE);
  
  if (req->http_hdrs->hdr_host->len)
  { /* check host looks valid ... header must exist, but can be empty */
    size_t pos = req->http_hdrs->hdr_host->pos;
    size_t len = req->http_hdrs->hdr_host->len;
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

    if (http__chk_single_crlf(data, pos, len))
      return (FALSE);

    if ((tmp = vstr_srch_chr_fwd(data, pos, len, ':')))
    { /* NOTE: not sure if we have to 400 if the port doesn't match
       * or if it's an "invalid" port number (Ie. == 0 || > 65535) */
      len -= tmp - pos; pos = tmp;

      /* if it's port 80, pretend it's not there */
      if (VEQ(data, pos, len, ":80") || VEQ(data, pos, len, ":"))
        req->http_hdrs->hdr_host->len -= len;
      else
      {
        len -= 1; pos += 1; /* skip the ':' */
        if (vstr_spn_cstr_chrs_fwd(data, pos, len, "0123456789") != len)
          return (FALSE);
      }
    }
  }

  if (http__chk_single_crlf(data, op_pos, op_len))
    return (FALSE);
  
  /* uri#fragment ... craptastic clients pass this and assume it is ignored */
  /* ignore query, and opaque data too? */
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
static int http_parse_range(struct Http_req_data *req,
                            VSTR_AUTOCONF_uintmax_t *range_beg,
                            VSTR_AUTOCONF_uintmax_t *range_end)
{
  Vstr_base *data     = req->http_hdrs->multi->comb;
  Vstr_sect_node *h_r = req->http_hdrs->multi->hdr_range;
  size_t pos = h_r->pos;
  size_t len = h_r->len;
  VSTR_AUTOCONF_uintmax_t fsize = req->f_stat->st_size;
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
  Vstr_base *data = con->evnt->io_r;
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
    /* serv->use_range - turn off? */
    req->http_hdrs->multi->hdr_range->pos = 0;
    *http_ret_code = 200;
    *http_ret_line = "OK - Range Failed";
    return (FALSE);
  }
  
  con->f_len = f_len;

  return (TRUE);
}

#define HTTP__PARSE_CHK_RET_OK() do {                   \
      HTTP_SKIP_LWS(data, pos, len);                    \
                                                        \
      if (!len ||                                       \
          VPREFIX(data, pos, len, ",") ||               \
          (allow_more && VPREFIX(data, pos, len, ";"))) \
      {                                                 \
        *passed_pos = pos;                              \
        *passed_len = len;                              \
                                                        \
        return (TRUE);                                  \
      }                                                 \
    } while (FALSE)

/* What is the quality parameter, value between 0 and 1000 inclusive.
 * returns TRUE on success, FALSE on failure. */
static int http_parse_quality(Vstr_base *data,
                              size_t *passed_pos, size_t *passed_len,
                              int allow_more, unsigned int *val)
{
  size_t pos = *passed_pos;
  size_t len = *passed_len;
  
  ASSERT(val);
  
  *val = 1000;
  
  HTTP_SKIP_LWS(data, pos, len);

  *passed_pos = pos;
  *passed_len = len;
  
  if (!len || VPREFIX(data, pos, len, ","))
    return (TRUE);
  else if (VPREFIX(data, pos, len, ";"))
  {
    int lead_zero = FALSE;
    unsigned int num_len = 0;
    unsigned int parse_flags = VSTR_FLAG02(PARSE_NUM, NO_BEG_PM, NO_NEGATIVE);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "q"))
      return (!!allow_more);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "="))
      return (!!allow_more);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);

    /* if it's 0[.00?0?] TRUE, 0[.\d\d?\d?] or 1[.00?0?] is FALSE */
    if (!(lead_zero = VPREFIX(data, pos, len, "0")) &&
        !VPREFIX(data, pos, len, "1"))
      return (FALSE);
    *val = (!lead_zero) * 1000;
    
    len -= 1; pos += 1;
    
    HTTP__PARSE_CHK_RET_OK();

    if (!VPREFIX(data, pos, len, "."))
      return (FALSE);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);

    *val += vstr_parse_uint(data, pos, len, 10 | parse_flags, &num_len, NULL);
    if (!num_len || (num_len > 3) || (*val > 1000))
      return (FALSE);
    len -= num_len; pos += num_len;
    
    HTTP__PARSE_CHK_RET_OK();
  }
  
  return (FALSE);
}
#undef HTTP__PARSE_CHK_RET_OK

static int http_parse_accept_encoding_gzip(struct Http_req_data *req)
{
  Vstr_base *data = req->http_hdrs->multi->comb;
  size_t pos = 0;
  size_t len = 0;
  unsigned int gzip_val     = 1001;
  unsigned int identity_val = 1001;
  unsigned int star_val     = 1001;
  
  pos = req->http_hdrs->multi->hdr_accept_encoding->pos;
  len = req->http_hdrs->multi->hdr_accept_encoding->len;

  if (!serv->use_err_406 && !serv->use_gzip_content_replacement)
    return (FALSE);
  
  if (!len)
    return (FALSE);
  
  req->content_encoding_xgzip = FALSE;
  
  while (len)
  {
    size_t tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len,
                                         HTTP_EOL HTTP_LWS ";,");
    
    if (0) { }
    else if (VEQ(data, pos, tmp, "identity"))
    {
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, FALSE, &identity_val))
        return (FALSE);
    }
    else if (serv->use_gzip_content_replacement && VEQ(data, pos, tmp, "gzip"))
    {
      len -= tmp; pos += tmp;
      req->content_encoding_xgzip = FALSE;
      if (!http_parse_quality(data, &pos, &len, FALSE, &gzip_val))
        return (FALSE);
    }
    else if (serv->use_gzip_content_replacement &&
             VEQ(data, pos, tmp, "x-gzip"))
    {
      len -= tmp; pos += tmp;
      req->content_encoding_xgzip = TRUE;
      if (!http_parse_quality(data, &pos, &len, FALSE, &gzip_val))
        return (FALSE);
      gzip_val = 1000; /* ignore quality on x-gzip - just parse for errors */
    }
    else if (VEQ(data, pos, tmp, "*"))
    { /* "*;q=0,gzip" means TRUE ... and "*;q=1.0,gzip;q=0" means FALSE */
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, FALSE, &star_val))
        return (FALSE);
    }
    else
    {
      len -= tmp; pos += tmp;
    }
    
    /* skip to end, or after next ',' */
    tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len, ",");    
    len -= tmp; pos += tmp;
    if (!len)
      break;
    assert(VPREFIX(data, pos, len, ","));
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
  }

  if (!serv->use_gzip_content_replacement)
    gzip_val = 0;
  
  if (gzip_val     == 1001) gzip_val     = star_val;
  if (identity_val == 1001) identity_val = star_val;

  if (!identity_val)
    req->content_encoding_identity = FALSE;
  
  if (gzip_val == 1001)
    return (FALSE);

  if (identity_val != 1001)
    if (identity_val > gzip_val)
      return (FALSE);
  
  return (!!gzip_val);
}

/* try to use gzip content-encoding on entity */
static void http__try_gzip_content(struct Con *con, struct Http_req_data *req)
{
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
      /* _only_ copy the new size over, mtime etc. is from the original file */
      con->f_len = req->f_stat->st_size = f_stat->st_size;
      req->content_encoding_gzip = TRUE;
    }
    close(fd);
  }
}  

/* skip a quoted string, or fail on syntax error */
static int http__skip_quoted_string(Vstr_base *data, size_t *pos, size_t *len)
{
  size_t tmp = 0;

  assert(VPREFIX(data, *pos, *len, "\""));
  *len -= 1; *pos += 1;

  while (TRUE)
  {
    tmp = vstr_cspn_cstr_chrs_fwd(data, *pos, *len, "\"\\");
    *len -= tmp; *pos += tmp;
    if (!*len)
      return (FALSE);
    
    if (vstr_export_chr(data, *pos) == '"')
      goto found;
    assert(VPREFIX(data, *pos, *len, "\\"));
    if (*len <= 2) /* must be at least <\X"> */
      return (FALSE);
    *len -= 2; *pos += 2;
  }

 found:
  *len -= 1; *pos += 1;
  HTTP_SKIP_LWS(data, *pos, *len);
  return (TRUE);
}

/* skip, or fail on syntax error */
static int http__skip_parameters(Vstr_base *data, size_t *pos, size_t *len)
{
  while (*len && (vstr_export_chr(data, *pos) != ','))
  { /* skip parameters */
    size_t tmp = 0;
    
    if (vstr_export_chr(data, *pos) != ';')
      return (FALSE); /* syntax error */
    
    *len -= 1; *pos += 1;
    HTTP_SKIP_LWS(data, *pos, *len);
    tmp = vstr_cspn_cstr_chrs_fwd(data, *pos, *len, ";,=");
    *len -= tmp; *pos += tmp;
    if (!*len)
      break;
    
    switch (vstr_export_chr(data, *pos))
    {
      case ';': break;
      case ',': break;
        
      case '=': /* skip parameter value */
        *len -= 1; *pos += 1;
        HTTP_SKIP_LWS(data, *pos, *len);
        if (!*len)
          return (FALSE); /* syntax error */
        if (vstr_export_chr(data, *pos) == '"')
        {
          if (!http__skip_quoted_string(data, pos, len))
            return (FALSE); /* syntax error */
        }
        else
        {
          tmp = vstr_cspn_cstr_chrs_fwd(data, *pos, *len, ";,");
          *len -= tmp; *pos += tmp;
        }
        break;
    }
  }

  return (TRUE);
}

/* returns TRUE if we can use the content-type,
* this does mean that if we fail to parse the "Accept:" line, we return TRUE */
static int http_parse_accept(struct Http_req_data *req)
{
  Vstr_base *data = req->http_hdrs->multi->comb;
  size_t pos = 0;
  size_t len = 0;
  unsigned int val = 0;
  int done_sub_type = FALSE;
  const Vstr_base *ct_vs1 = req->content_type_vs1;
  size_t ct_pos = req->content_type_pos;
  size_t ct_len = req->content_type_len;
  size_t ct_sub_len = 0;
  
  pos = req->http_hdrs->multi->hdr_accept->pos;
  len = req->http_hdrs->multi->hdr_accept->len;
  
  if (!serv->use_err_406 || !len)
    return (TRUE);

  if (!(ct_sub_len = vstr_srch_chr_fwd(ct_vs1, ct_pos, ct_len, '/')) ||
      vstr_srch_chr_fwd(ct_vs1, ct_pos, ct_len, '*'))
  { /* it looks weird, blank it */
    req->content_type_vs1 = NULL;
    return (TRUE);
  }
  ct_sub_len = vstr_sc_posdiff(ct_pos, ct_sub_len);
  
  while (len)
  {
    size_t tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len,
                                         HTTP_EOL HTTP_LWS ";,");
    
    if (0) { }
    else if (vstr_cmp_eq(data, pos, tmp, ct_vs1, ct_pos, ct_len))
    { /* full match */
      len -= tmp; pos += tmp;
      return (!http_parse_quality(data, &pos, &len, TRUE, &val) || val);
    }
    else if ((tmp == (ct_sub_len + 1)) &&
             vstr_cmp_eq(data, pos, ct_sub_len, ct_vs1, ct_pos, ct_sub_len) &&
             (vstr_export_chr(data, vstr_sc_poslast(pos, tmp)) == '*'))
    { /* sub match */
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, TRUE, &val))
        return (TRUE);
      done_sub_type = TRUE;
    }
    else if (!done_sub_type && VEQ(data, pos, tmp, "*/*"))
    { /* "*;q=0,gzip" means TRUE ... and "*;q=1,gzip;q=0" means FALSE */
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, TRUE, &val))
        return (TRUE);
    }
    else
    {
      len -= tmp; pos += tmp;
      HTTP_SKIP_LWS(data, pos, len);
    }

    if (!http__skip_parameters(data, &pos, &len))
      return (TRUE);
    if (!len)
      break;
    
    assert(VPREFIX(data, pos, len, ","));
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
  }

  return (val);
}

static int http_req_1_x(struct Con *con, struct Http_req_data *req,
                        unsigned int *http_ret_code,
                        const char **http_ret_line)
{
  Vstr_base *out = con->evnt->io_w;
  VSTR_AUTOCONF_uintmax_t range_beg = 0;
  VSTR_AUTOCONF_uintmax_t range_end = 0;
  VSTR_AUTOCONF_uintmax_t f_len     = 0;
  time_t now = time(NULL);
  Vstr_sect_node *h_r = req->http_hdrs->multi->hdr_range;
  
  if (req->ver_1_1 && req->http_hdrs->hdr_expect->len)
    /* I'm pretty sure we can ignore 100-continue, as no request will
     * have a body */
    HTTPD_ERR_RET(req, 417, FALSE);
          
  /* Might normally add "!req->head_op && ..." but
   * http://www.w3.org/TR/chips/#gl6 says that's bad */
  if (http_parse_accept_encoding_gzip(req))
    http__try_gzip_content(con, req);
  if (serv->use_err_406 &&
      !req->content_encoding_identity && !req->content_encoding_gzip)
    HTTPD_ERR_RET(req, 406, FALSE);
  
  if (h_r->pos)
  {
    int ret_code = 0;

    if (!(serv->use_range && (req->ver_1_1 || serv->use_range_1_0)))
      h_r->pos = 0;
    else if (!(ret_code = http_parse_range(req, &range_beg, &range_end)))
      h_r->pos = 0;
    ASSERT(!ret_code || (ret_code == 200) || (ret_code == 416));
    if (ret_code == 416)
    {
      if (!req->http_hdrs->hdr_if_range->pos)
        HTTPD_ERR_RET(req, 416, FALSE);
      h_r->pos = 0;
    }
  }

  if (!http_response_ok(con, req, now, http_ret_code, http_ret_line))
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

  http_app_def_hdrs(con, req, *http_ret_code, *http_ret_line,
                    now, req->f_stat->st_mtime, NULL, con->f_len);
  if (h_r->pos)
    http_app_header_fmt(out, "Content-Range",
                        "%s %ju-%ju/%ju", "bytes", range_beg, range_end,
                        (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);
  
  http_app_end_hdrs(out);
  
  return (TRUE);
}

static void http_vlg_def(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_sect_node *h_h  = req->http_hdrs->hdr_host;
  Vstr_sect_node *h_ua = req->http_hdrs->hdr_ua;
  Vstr_sect_node *h_r  = req->http_hdrs->hdr_referer;

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

  vlg_info(vlg, ": $<http-esc.vstr:%p%zu%zu>\n",
           data, req->path_pos, req->path_len);
}

static int serv__init_default_hostname(const char *def_hostname)
{
  Vstr_base *lfn = serv->default_hostname;
  
  if (def_hostname)
    vstr_sub_cstr_ptr(lfn, 1, lfn->len, def_hostname);
  else if (server_port != 80)
    vstr_add_fmt(lfn, lfn->len, ":%hd", server_port);

  return (!lfn->conf->malloc_bad);
}

static int serv_add_default_hostname(Vstr_base *lfn, size_t pos)
{
  Vstr_base *d_h = serv->default_hostname;
  
  return (vstr_add_vstr(lfn, pos, d_h, 1, d_h->len, VSTR_TYPE_ADD_DEF));
}

static int serv_chk_vhost(Vstr_base *lfn, size_t pos, size_t len)
{
  const char *vhost = vstr_export_cstr_ptr(lfn, pos, len);
  struct stat64 v_stat[1];
  
  if (!vhost)
    return (TRUE); /* dealt with as errmem_req() later */
  
  if (stat64(vhost, v_stat) == -1)
    return (FALSE);

  if (!S_ISDIR(v_stat->st_mode))
    return (FALSE);

  return (TRUE);
}

static int serv_add_vhost(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *fname = req->fname;
  Vstr_sect_node *h_h = req->http_hdrs->hdr_host;
  size_t h_h_pos = h_h->pos;
  size_t h_h_len = h_h->len;
    
  if (h_h_len && serv->canonize_host)
  {
    size_t dots = 0;
    
    if (VIPREFIX(data, h_h_pos, h_h_len, "www."))
    { h_h_len -= strlen("www."); h_h_pos += strlen("www."); }
    
    dots = vstr_spn_cstr_chrs_rev(data, h_h_pos, h_h_len, ".");
    h_h_len -= dots;
  }
  
  if (!h_h_len)
    serv_add_default_hostname(fname, 0);
  else if (vstr_add_vstr(fname, 0, data, /* add as buf's, for lowercase op */
                         h_h_pos, h_h_len, VSTR_TYPE_ADD_DEF))
  {
    vstr_conv_lowercase(fname, 1, h_h_len);
    
    if (!serv_chk_vhost(fname, 1, h_h_len))
    {
      if (serv->non_host_err_400)
        HTTPD_ERR_RET(req, 400, FALSE); /* rfc 5.2 */
      else
      { /* what everything else does ... *sigh* */
        if (fname->conf->malloc_bad)
          return (TRUE);
      
        vstr_del(fname, 1, h_h_len);
        serv_add_default_hostname(fname, 0);
      }  
    }
  }

  vstr_add_cstr_ptr(fname, 0, "/");
  return (TRUE);
}

static int http_req_make_path(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *fname = req->fname;
  
  ASSERT(!fname->len);

  assert(VPREFIX(data, req->path_pos, req->path_len, "/") ||
         VEQ(data, req->path_pos, req->path_len, "*"));

  /* don't allow encoded slashes */
  if (vstr_srch_case_cstr_buf_fwd(data, req->path_pos, req->path_len, "%2f"))
    HTTPD_ERR_RET(req, 403, FALSE);
  
  vstr_add_vstr(fname, 0,
                data, req->path_pos, req->path_len, VSTR_TYPE_ADD_BUF_PTR);
  vstr_conv_decode_uri(fname, 1, fname->len);

  if (fname->conf->malloc_bad) /* dealt with as errmem_req() later */
    return (TRUE);
  
  if (serv->use_vhosts)
    if (!serv_add_vhost(con, req))
      return (FALSE);
  
  /* check path ... including hostname */
  if (vstr_srch_chr_fwd(fname, 1, fname->len, 0))
    HTTPD_ERR_RET(req, 403, FALSE);
      
  if (vstr_srch_cstr_buf_fwd(fname, 1, fname->len, "/../") ||
      VSUFFIX(req->fname, 1, req->fname->len, "/.."))
    HTTPD_ERR_RET(req, 403, FALSE);

  ASSERT(fname->len);
  assert(VPREFIX(fname, 1, fname->len, "/") ||
         VEQ(fname, 1, fname->len, "*") ||
         fname->conf->malloc_bad);

  if (fname->conf->malloc_bad)
    return (TRUE);
  
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
  if (con->evnt->io_r_shutdown)
    return (FALSE);

  evnt_fd_set_cork(con->evnt, FALSE);
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
      conf = con->evnt->io_w->conf;
      
    if (!(req->fname = vstr_make_base(conf)) ||
        !(req->http_hdrs->multi->combiner_store = vstr_make_base(conf)) ||
        !(req->sects = vstr_sects_make(8)))
      return (NULL);
  }
  
  req->http_hdrs->multi->comb = con ? con->evnt->io_r : NULL;

  http__clear_hdrs(req);
  
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
    req->orig_io_w_len = con->evnt->io_w->len;

  if (req->f_mmap)
    vstr_del(req->f_mmap, 1, req->f_mmap->len);

  req->content_type_vs1 = NULL;
  req->content_type_pos = 0;
  req->content_type_len = 0;

  req->sects->malloc_bad = FALSE;

  req->content_encoding_gzip     = FALSE;
  req->content_encoding_identity = TRUE;
    
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

  /* we do vstr deletes here to return the nodes back to the pool */
  vstr_del(req->fname, 1, req->fname->len);
  ASSERT(!req->http_hdrs->multi->combiner_store->len);
  if (req->f_mmap)
    vstr_del(req->f_mmap, 1, req->f_mmap->len);
  
  req->content_type_vs1 = NULL;
    
  req->http_hdrs->multi->comb = NULL;

  req->using_req = FALSE;
}

static void http_req_exit(void)
{
  struct Http_req_data *req = http_make_req(NULL);

  if (!req)
    return;
  
  vstr_free_base(req->fname);  req->fname  = NULL;
  vstr_free_base(req->http_hdrs->multi->combiner_store);
                 req->http_hdrs->multi->combiner_store = NULL;
  vstr_free_base(req->f_mmap); req->f_mmap = NULL;
  vstr_sects_free(req->sects); req->sects  = NULL;
  
  req->done_once = FALSE;
  req->using_req = FALSE;
}

static int http_con_cleanup(struct Con *con, struct Http_req_data *req)
{
  con->evnt->io_r->conf->malloc_bad = FALSE;
  con->evnt->io_w->conf->malloc_bad = FALSE;
  
  http__clear_hdrs(req);
  http_req_free(req);
    
  return (FALSE);
}

static int http_fin_req(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *out = con->evnt->io_w;
  
  ASSERT(!out->conf->malloc_bad);
  http__clear_hdrs(req);

  /* Note: Not resetting con->parsed_method_ver_1_0,
   * if it's non-0.9 now it should continue to be */
  
  if (!con->keep_alive) /* all input is here */
  {
    evnt_wait_cntl_del(con->evnt, POLLIN);
    req->len = con->evnt->io_r->len; /* delete it all */
  }
  
  vstr_del(con->evnt->io_r, 1, req->len);
  
  http_req_free(req);

  evnt_put_pkt(con->evnt);
  
  evnt_fd_set_cork(con->evnt, TRUE);
  return (serv_send(con));
}

static int http_fin_err_req(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *out = con->evnt->io_w;

  req->content_encoding_gzip = FALSE;
  
  if ((req->error_code == 400) || (req->error_code == 405) ||
      (req->error_code == 413) ||
      (req->error_code == 500) || (req->error_code == 501))
    con->keep_alive = HTTP_NIL_KEEP_ALIVE;
  
  ASSERT(!con->f_len);
  
  vlg_info(vlg, "ERREQ from[$<sa:%p>] err[%03u %s]",
           con->evnt->sa, req->error_code, req->error_line);
  if (req->sects->num >= 2)
    http_vlg_def(con, req);
  else
    vlg_info(vlg, "%s", "\n");

  if (req->malloc_bad)
  { /* delete all input to give more room */
    ASSERT(req->error_code == 500);
    vstr_del(con->evnt->io_r, 1, con->evnt->io_r->len);
  }
  
  if (!req->ver_0_9)
  {
    http_app_def_hdrs(con, req, req->error_code, req->error_line,
                      time(NULL), server_beg, "text/html", req->error_len);
    if (req->error_code == 416)
      http_app_header_fmt(out, "Content-Range", "%s */%ju", "bytes",
                          (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);

    if ((req->error_code == 405) || (req->error_code == 501))
      http_app_header_cstr(out, "Allow", "GET, HEAD, OPTIONS, TRACE");
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
  Vstr_base *out = con->evnt->io_w;
  
  /* remove anything we can to free space */
  vstr_sc_reduce(out, 1, out->len, out->len - req->orig_io_w_len);
  
  con->evnt->io_r->conf->malloc_bad = FALSE;
  con->evnt->io_w->conf->malloc_bad = FALSE;
  req->malloc_bad = TRUE;
  
  HTTPD_ERR_RET(req, 500, http_fin_err_req(con, req));
}

static void http__req_absolute_uri(struct Con *con, struct Http_req_data *req,
                                   Vstr_base *lfn)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_sect_node *h_h = req->http_hdrs->hdr_host;

  vstr_del(lfn, 1, lfn->len);
  
  vstr_add_cstr_buf(lfn, lfn->len, "http://");
  
  if (!h_h->len)
    serv_add_default_hostname(lfn, lfn->len);
  else
    vstr_add_vstr(lfn, lfn->len,
                  data, h_h->pos, h_h->len, VSTR_TYPE_ADD_ALL_BUF);

  vstr_add_vstr(lfn, lfn->len, con->evnt->io_r, req->path_pos, req->path_len,
                VSTR_TYPE_ADD_ALL_BUF);

  /* we got:       http://foo/bar/
   * and so tried: http://foo/bar/index.html
   *
   * but foo/bar/index.html is a directory (fun), so redirect to:
   *               http://foo/bar/index.html/
   */
  if (VSUFFIX(lfn, 1, lfn->len, "/"))
    vstr_add_cstr_ptr(lfn, lfn->len, serv->dir_filename);
  
  vstr_add_cstr_buf(lfn, lfn->len, "/");
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
  
  assert(!VSUFFIX(req->fname, 1, req->fname->len, "/..") &&
         !VEQ(req->fname, 1, req->fname->len, ".."));
  
  http__req_absolute_uri(con, req, req->fname);
  
  req->error_code = 301;
  req->error_line = CONF_LINE_RET_301;
  req->error_len  = CONF_MSG_LEN_301(req->fname);
  
  return (http_fin_err_req(con, req));
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
        
      default: /* just ignore any other content */
        return (TRUE);
    }
  }

  req->content_type_vs1 = vs1;
  req->content_type_pos = pos;
  req->content_type_len = len;

  if (!http_parse_accept(req))
    HTTPD_ERR_RET(req, 406, FALSE);
  
  return (TRUE);
}

static int http_req_op_get(struct Con *con, struct Http_req_data *req)
{ /* GET or HEAD ops */
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *out  = con->evnt->io_w;
  Vstr_base *fname = req->fname;
  const char *fname_cstr = NULL;
  unsigned int http_ret_code = 200;
  const char * http_ret_line = "OK";
  
  if (fname->conf->malloc_bad)
    goto mem_err;

  assert(VPREFIX(fname, 1, fname->len, "/"));
  
  if (!http__req_content_type(req))
    return (http_fin_err_req(con, req));

  /* change "///foo" into "foo" ... this is done now
   * as req_content_type is done on a full path name */
  vstr_del(fname, 1, vstr_spn_cstr_chrs_fwd(fname, 1, fname->len, "/"));

  fname_cstr = vstr_export_cstr_ptr(fname, 1, fname->len);
  
  if (fname->conf->malloc_bad)
    goto mem_err;

  ASSERT(con->f_fd == -1);
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
             (errno == ENAMETOOLONG) || /* 414 ? */
             FALSE)
      HTTPD_ERR(req, 404);
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
  else if (!http_req_1_x(con, req, &http_ret_code, &http_ret_line))
    return (http_fin_err_close_req(con, req));
  
  if (out->conf->malloc_bad)
    goto mem_close_err;
  
  vlg_dbg2(vlg, "REPLY:\n$<vstr:%p%zu%zu>\n", out, (size_t)1, out->len);
  
  if (req->use_mmap && !vstr_mov(con->evnt->io_w, con->evnt->io_w->len,
                                 req->f_mmap, 1, req->f_mmap->len))
    goto mem_close_err;

  /* req->head_op is set for 304 returns */
  vlg_info(vlg, "REQ $<vstr.sect:%p%p%u> from[$<sa:%p>] ret[%03u %s]"
           " sz[${BKMG.ju:%ju}:%ju]", data, req->sects, 1, con->evnt->sa,
           http_ret_code, http_ret_line, con->f_len, con->f_len);
  http_vlg_def(con, req);
  
  if (req->head_op || req->use_mmap || !con->f_len)
    serv_fd_close(con);
  
  return (http_fin_req(con, req));

 mem_close_err:
  serv_fd_close(con);
 mem_err:
  VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_get(): %m\n"));
}

static int http_req_op_opts(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *out = con->evnt->io_w;
  Vstr_base *fname = req->fname;
  time_t now = time(NULL);
  VSTR_AUTOCONF_uintmax_t tmp = 0;

  if (fname->conf->malloc_bad)
    goto mem_err;
  
  assert(VPREFIX(fname, 1, fname->len, "/") ||
         !serv->use_vhosts || !serv->non_host_err_400 ||
         VEQ(con->evnt->io_r, req->path_pos, req->path_len, "*"));
  
  /* apache doesn't test for 404's here ... which seems weird */
  
  http_app_def_hdrs(con, req, 200, "OK", now, 0, NULL, 0);
  http_app_header_cstr(out, "Allow", "GET, HEAD, OPTIONS, TRACE");
  http_app_end_hdrs(out);
  if (out->conf->malloc_bad)
    goto mem_err;
  
  vlg_info(vlg, "REQ %s from[$<sa:%p>] ret[%03u %s] sz[${BKMG.ju:%ju}:%ju]",
           "OPTIONS", con->evnt->sa, 200, "OK", tmp, tmp);
  http_vlg_def(con, req);
  
  return (http_fin_req(con, req));
  
 mem_err:
  VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_opts(): %m\n"));
}

static int http_req_op_trace(struct Con *con, struct Http_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *out  = con->evnt->io_w;
  time_t now = time(NULL);
  VSTR_AUTOCONF_uintmax_t tmp = 0;
      
  http_app_def_hdrs(con, req, 200, "OK", now, now, "message/http", req->len);
  http_app_end_hdrs(out);
  vstr_add_vstr(out, out->len, data, 1, req->len, VSTR_TYPE_ADD_DEF);
  if (out->conf->malloc_bad)
    VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_trace(): %m\n"));

  tmp = req->len;
  vlg_info(vlg, "REQ %s from[$<sa:%p>] ret[%03u %s] sz[${BKMG.ju:%ju}:%ju]",
           "TRACE", con->evnt->sa, 200, "OK", tmp, tmp);
  http_vlg_def(con, req);
      
  return (http_fin_req(con, req));
}

static int http__parse_no_req(struct Con *con, struct Http_req_data *req)
{
  if (server_max_header_sz && (con->evnt->io_r->len > server_max_header_sz))
    HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

  http_req_free(req);
  
  return (http_parse_wait_io_r(con));
}

/* http spec says ignore leading LWS ... *sigh* */
static int http__parse_req_all(struct Con *con, struct Http_req_data *req,
                               const char *eol, int *ern)
{
  Vstr_base *data = con->evnt->io_r;
  
  ASSERT(eol && ern);
  
  *ern = FALSE;
  
  if (!(req->len = vstr_srch_cstr_buf_fwd(data, 1, data->len, eol)))
      goto no_req;
  
  if (req->len == 1)
  { /* should use vstr_del(data, 1, vstr_spn_cstr_buf_fwd(..., HTTP_EOL)); */
    while (VPREFIX(data, 1, data->len, HTTP_EOL))
      vstr_del(data, 1, strlen(HTTP_EOL));
    
    if (!(req->len = vstr_srch_cstr_buf_fwd(data, 1, data->len, eol)))
      goto no_req;
    
    ASSERT(req->len > 1);
  }
  
  req->len += strlen(eol) - 1; /* add rest of EOL */

  return (TRUE);

 no_req:
  *ern = http__parse_no_req(con, req);
  return (FALSE);
}

static int http_parse_req(struct Con *con)
{
  Vstr_base *data = con->evnt->io_r;
  struct Http_req_data *req = NULL;
  int ern_req_all = 0;

  ASSERT(!con->f_len);

  if (!(req = http_make_req(con)))
    return (FALSE);

  if (con->parsed_method_ver_1_0) /* wait for all the headers */
  {
    if (!http__parse_req_all(con, req, HTTP_END_OF_REQUEST, &ern_req_all))
      return (ern_req_all);
  }
  else
  {
    if (!http__parse_req_all(con, req, HTTP_EOL,            &ern_req_all))
      return (ern_req_all);
  }

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

    if (req->ver_0_9)
      vlg_dbg1(vlg, "Method(0.9):"
               " $<http-esc.vstr.sect:%p%p%u> $<http-esc.vstr.sect:%p%p%u>\n",
               con->evnt->io_r, req->sects, 1, con->evnt->io_r, req->sects, 2);
    else
    { /* need to get all headers */
      if (!con->parsed_method_ver_1_0)
      {
        con->parsed_method_ver_1_0 = TRUE;
        if (!http__parse_req_all(con, req, HTTP_END_OF_REQUEST, &ern_req_all))
          return (ern_req_all);
      }
      
      vlg_dbg1(vlg, "Method(1.x):"
               " $<http-esc.vstr.sect:%p%p%u> $<http-esc.vstr.sect:%p%p%u>"
               " $<http-esc.vstr.sect:%p%p%u>\n",
               data, req->sects, 1, data, req->sects, 2, data, req->sects, 3);
      http_req_split_hdrs(con, req);
    }
    evnt_got_pkt(con->evnt);

    vlg_dbg3(vlg, "REQ:\n$<vstr.hexdump:%p%zu%zu>",
             data, (size_t)1, data->len);
    
    assert(((req->sects->num >= 3) && !req->ver_0_9) || (req->sects->num == 2));
    
    op_pos        = VSTR_SECTS_NUM(req->sects, 1)->pos;
    op_len        = VSTR_SECTS_NUM(req->sects, 1)->len;
    req->path_pos = VSTR_SECTS_NUM(req->sects, 2)->pos;
    req->path_len = VSTR_SECTS_NUM(req->sects, 2)->len;

    if (!req->ver_0_9 && !http__parse_1_x(con, req))
    {
      if (req->error_code == 500)
        return (http_fin_errmem_req(con, req));
      return (http_fin_err_req(con, req));
    }
    
    if (!http_absolute_uri(con, req))
      HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

    if (0) { }
    else if (VEQ(data, op_pos, op_len, "GET"))
    {
      if (!VPREFIX(data, req->path_pos, req->path_len, "/"))
        HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));
      
      if (!http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_get(con, req));
    }
    else if (req->ver_0_9) /* 400 or 501? - apache does 400 */
      HTTPD_ERR_RET(req, 501, http_fin_err_req(con, req));      
    else if (VEQ(data, op_pos, op_len, "HEAD"))
    {
      req->head_op = TRUE; /* not sure where this should go here */
      
      if (!VPREFIX(data, req->path_pos, req->path_len, "/"))
        HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));
      
      if (!http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_get(con, req));
    }
    else if (VEQ(data, op_pos, op_len, "OPTIONS"))
    {
      if (!VPREFIX(data, req->path_pos, req->path_len, "/") &&
          !VEQ(data, req->path_pos, req->path_len, "*"))
        HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

      /* Speed hack: Don't even call make_path if it's "OPTIONS * ..."
       * and we don't need to check the Host header */
      if (serv->use_vhosts && serv->non_host_err_400 &&
          !VEQ(data, req->path_pos, req->path_len, "*") &&
          !http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_opts(con, req));
    }
    else if (VEQ(data, op_pos, op_len, "TRACE"))
    {
      return (http_req_op_trace(con, req));
    }
    else if (VEQ(data, op_pos, op_len, "POST") ||
             VEQ(data, op_pos, op_len, "PUT") ||
             VEQ(data, op_pos, op_len, "DELETE") ||
             VEQ(data, op_pos, op_len, "CONNECT") ||
             FALSE) /* we know about these ... but don't allow them */
      HTTPD_ERR_RET(req, 405, http_fin_err_req(con, req));
    else
      HTTPD_ERR_RET(req, 501, http_fin_err_req(con, req));
  }
  ASSERT_NOT_REACHED();
}

static int serv_q_send(struct Con *con)
{
  vlg_dbg3(vlg, "Q $<sa:%p>\n", con->evnt->sa);
  if (!evnt_send_add(con->evnt, TRUE, CL_MAX_WAIT_SEND))
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
    if (!con->evnt->io_r_shutdown)
      evnt_wait_cntl_add(con->evnt, POLLIN);
    return (http_parse_req(con));
  }

  return (evnt_shutdown_w(con->evnt));
}

/* try sending a bunch of times, bail out if we've done a few ... keep going
 * if we have too much data */
#define SERV__SEND(x) do {                    \
    if (!--num)                               \
      return (serv_q_send(con));              \
                                              \
    if (!evnt_send(con->evnt))                \
    {                                         \
      if (errno != EPIPE)                     \
        vlg_warn(vlg, "send(%s): %m\n", x);   \
      return (FALSE);                         \
    }                                         \
 } while (out->len >= EX_MAX_W_DATA_INCORE)

static int serv_send(struct Con *con)
{
  Vstr_base *out = con->evnt->io_w;
  size_t len = 0;
  int ret = IO_OK;
  unsigned int num = CONF_FS_READ_CALL_LIMIT;
  
  ASSERT(!out->conf->malloc_bad);
    
  if ((con->f_fd == -1) && con->f_len)
    return (FALSE);

  ASSERT((con->f_fd == -1) == !con->f_len);
  
  if (out->len)
    SERV__SEND("beg");

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
      
      if (!evnt_sendfile(con->evnt, con->f_fd, &con->f_off, &con->f_len, &ern))
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

  return (serv_send(con)); /* restart with a read, or finish */
}

static int serv_cb_func_send(struct Evnt *evnt)
{
  return (serv_send((struct Con *)evnt));
}

static int serv_recv(struct Con *con)
{
  unsigned int ern = 0;
  int ret = 0;
  Vstr_base *data = con->evnt->io_r;

  ASSERT(!con->evnt->io_r_shutdown);
  
  if (!(ret = evnt_recv(con->evnt, &ern)))
  {
    vlg_dbg3(vlg, "RECV ERR from[$<sa:%p>]: %u\n", con->evnt->sa, ern);
    
    if (ern != VSTR_TYPE_SC_READ_FD_ERR_EOF)
      goto con_cleanup;
    if (!evnt_shutdown_r(con->evnt, TRUE))
      goto con_cleanup;
  }
    
  if (con->f_len) /* need to stop input, until we can get rid of it */
  {
    ASSERT(con->keep_alive || con->parsed_method_ver_1_0);
    
    if (server_max_header_sz && (data->len > server_max_header_sz))
      evnt_wait_cntl_del(con->evnt, POLLIN);

    return (TRUE);
  }
  
  if (http_parse_req(con))
    return (TRUE);
  
 con_cleanup:
  con->evnt->io_r->conf->malloc_bad = FALSE;
  con->evnt->io_w->conf->malloc_bad = FALSE;
    
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

  evnt_vlg_stats_info(evnt, "FREE");

  if (acpt_evnt)
    evnt_stats_add(acpt_evnt, con->evnt);
    
  free(con);
}

static void serv_cb_func_acpt_free(struct Evnt *evnt)
{
  evnt_vlg_stats_info(evnt, "ACCEPT FREE");

  ASSERT(acpt_evnt == evnt);

  acpt_evnt = NULL;
  cntl_free_acpt(evnt);
  
  free(evnt);
}

static struct Evnt *serv_cb_func_accept(int fd,
                                        struct sockaddr *sa, socklen_t len)
{
  struct Con *con = NULL;

  ASSERT(acpt_evnt);
  
  if (server_max_clients && (evnt_num_all() >= server_max_clients))
    evnt_wait_cntl_del(acpt_evnt, POLLIN);
  
  if (sa->sa_family != AF_INET) /* only support IPv4 atm. */
    goto make_acpt_fail;
  
  if (!(con = malloc(sizeof(struct Con))) ||
      !evnt_make_acpt(con->evnt, fd, sa, len))
    goto make_acpt_fail;

  con->f_fd  = -1;
  con->f_len =  0;
  
  con->parsed_method_ver_1_0 = FALSE;
  con->keep_alive = HTTP_NIL_KEEP_ALIVE;
  con->use_sendfile  = serv->use_sendfile;
  
  if (!evnt_sc_timeout_via_mtime(con->evnt, client_timeout * 1000))
    goto timer_add_fail;

  vlg_info(vlg, "CONNECT from[$<sa:%p>]\n", con->evnt->sa);
  
  con->evnt->cbs->cb_func_recv       = serv_cb_func_recv;
  con->evnt->cbs->cb_func_send       = serv_cb_func_send;
  con->evnt->cbs->cb_func_free       = serv_cb_func_free;

  return (con->evnt);

 timer_add_fail:
  evnt_free(con->evnt);
 make_acpt_fail:
  free(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, "%s: %m\n", "accept"));
}

static void serv_make_bind(const char *acpt_addr, short acpt_port)
{
  struct sockaddr_in *sinv4 = NULL;
  
  if (!(acpt_evnt = malloc(sizeof(struct Evnt))))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", __func__));
  
  if (!evnt_make_bind_ipv4(acpt_evnt, acpt_addr, acpt_port))
    vlg_err(vlg, 2, "%s: %m\n", __func__);
  
  acpt_evnt->cbs->cb_func_accept = serv_cb_func_accept;
  acpt_evnt->cbs->cb_func_free   = serv_cb_func_acpt_free;

  if (serv->defer_accept)
    evnt_fd_set_defer_accept(acpt_evnt, serv->defer_accept);

  sinv4 = EVNT_SA_IN(acpt_evnt);
  ASSERT(!server_port || (server_port == ntohs(sinv4->sin_port)));
  server_port = ntohs(sinv4->sin_port);
}

static void cl_cmd_line(int argc, char *argv[])
{
  int optchar = 0;
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
   {"processes", required_argument, NULL, 9},
   {"procs", required_argument, NULL, 9},
   {"debug", no_argument, NULL, 'd'},
   {"host", required_argument, NULL, 'H'},
   {"port", required_argument, NULL, 'P'},
   {"nagle", optional_argument, NULL, 'n'},
   {"max-clients", required_argument, NULL, 'M'},
   {"timeout", required_argument, NULL, 't'},
   {"version", no_argument, NULL, 'V'},
   
   {"sendfile", optional_argument, NULL, 31},
   {"mmap", optional_argument, NULL, 30},
   
   {"max-header-sz", required_argument, NULL, 128},
   {"keep-alive", optional_argument, NULL, 129},
   {"keep-alive-1.0", optional_argument, NULL, 130},
   {"vhosts", optional_argument, NULL, 131},
   {"virtual-hosts", optional_argument, NULL, 131},
   {"range", optional_argument, NULL, 132},
   {"range-1.0", optional_argument, NULL, 133},
   {"public-only", optional_argument, NULL, 134}, /* FIXME: rm ? */
   {"dir-filename", required_argument, NULL, 135},
   {"server-name", required_argument, NULL, 136},
   {"gzip-content-replacement", optional_argument, NULL, 137},
   {"send-default-mime-type", optional_argument, NULL, 138},
   {"send-err-406", optional_argument, NULL, 139},
   {"default-mime-type", required_argument, NULL, 140},
   {"mime-types-main", required_argument, NULL, 141},
   {"mime-types-extra", required_argument, NULL, 142},
   {"mime-types-xtra", required_argument, NULL, 142},
   /* 143 */
   {"defer-accept", required_argument, NULL, 144},
   {"default-hostname", required_argument, NULL, 145},
   {"canonize-host", optional_argument, NULL, 146},
   {"err-host-400", optional_argument, NULL, 147},
   /* {"404-file", required_argument, NULL, 0}, */
   {NULL, 0, NULL, 0}
  };
  const char *default_hostname = NULL;
  OPT_SC_SERV_DECL_OPTS();

  evnt_opt_nagle = TRUE;
  
  program_name = opt_program_name(argv[0], "jhttpd");
  
  while ((optchar = getopt_long(argc, argv, "dhH:M:nP:t:V",
                                long_options, NULL)) != -1)
  {
    switch (optchar)
    {
      case '?': usage(program_name, EXIT_FAILURE, "");
      case 'h': usage(program_name, EXIT_SUCCESS, "");
        
      case 'V':
      {
        Vstr_base *out = vstr_make_base(NULL);
        
        if (!out)
          errno = ENOMEM, err(EXIT_FAILURE, "version");

        vstr_add_fmt(out, 0, " %s version %s.\n",
                     program_name, CONF_SERV_VERSION);

        if (io_put_all(out, STDOUT_FILENO) == IO_FAIL)
          err(EXIT_FAILURE, "write");

        exit (EXIT_SUCCESS);
      }
      
        OPT_SC_SERV_OPTS();
        
      case 't': client_timeout       = atoi(optarg);  break;
      case 'H': server_ipv4_address  = optarg;        break;
      case 'M': server_max_clients   = atoi(optarg);  break;
      case 'P': server_port          = atoi(optarg);  break;
      case 128: server_max_header_sz = atoi(optarg);  break;
        
      case 'd': vlg_debug(vlg);                       break;
        
      case 'n': OPT_TOGGLE_ARG(evnt_opt_nagle);       break;
        
      case 31: OPT_TOGGLE_ARG(serv->use_sendfile);    break;
      case 30: OPT_TOGGLE_ARG(serv->use_mmap);        break;
        
      case 129: OPT_TOGGLE_ARG(serv->use_keep_alive);              break;
      case 130: OPT_TOGGLE_ARG(serv->use_keep_alive_1_0);          break;
      case 131: OPT_TOGGLE_ARG(serv->use_vhosts);                   break;
      case 132: OPT_TOGGLE_ARG(serv->use_range);                    break;
      case 133: OPT_TOGGLE_ARG(serv->use_range_1_0);                break;
      case 134: OPT_TOGGLE_ARG(serv->use_public_only);              break;
      case 135: serv->dir_filename = optarg;                        break;
      case 136: serv->server_name  = optarg;                        break;
      case 137: OPT_TOGGLE_ARG(serv->use_gzip_content_replacement); break;
      case 138: OPT_TOGGLE_ARG(serv->use_default_mime_type);        break;
      case 139: OPT_TOGGLE_ARG(serv->use_err_406);                  break;
      case 140: serv->mime_types_default_type = optarg;             break;
      case 141: serv->mime_types_main = optarg;                     break;
      case 142: serv->mime_types_xtra = optarg;                     break;
        /* case 143: */
      case 144: OPT_NUM_ARG(serv->defer_accept, "seconds to defer connections",
                            0, 4906, " (1 hour 8 minutes)");        break;
      case 145: default_hostname = optarg;                          break;
      case 146: OPT_TOGGLE_ARG(serv->canonize_host);                break;
      case 147: OPT_TOGGLE_ARG(serv->non_host_err_400);
        
      
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
    err(EXIT_FAILURE, "load_mime(%s)", serv->mime_types_main);
  
  if (!mime_types_load_simple(serv->mime_types_xtra))
    err(EXIT_FAILURE, "load_mime(%s)", serv->mime_types_main);
  
  if (become_daemon)
  {
    if (daemon(FALSE, FALSE) == -1)
      err(EXIT_FAILURE, "daemon");
    vlg_daemon(vlg, program_name);
  }

  serv_make_bind(server_ipv4_address, server_port);

  if (!serv__init_default_hostname(default_hostname))
    errno = ENOMEM, err(EXIT_FAILURE, "default_hostname");
  
  if (pid_file)
    vlg_pid_file(vlg, pid_file);

  if (cntl_file)
    cntl_make_file(vlg, acpt_evnt, cntl_file);

  if (acpt_filter_file)
    if (!evnt_fd_set_filter(acpt_evnt, acpt_filter_file))
      errx(EXIT_FAILURE, "set_filter");
  
  if (chroot_dir)
  { /* preload locale info. so syslog can log in localtime */
    time_t now = time(NULL);
    (void)localtime(&now);

    vlg_sc_bind_mount(chroot_dir);  
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

  if (num_procs > 1)
    cntl_sc_multiproc(vlg, acpt_evnt, num_procs, !!cntl_file);

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
    evnt_sc_main_loop(CL_MAX_WAIT_SEND);
    serv_cntl_resources();
  }
  evnt_out_dbg3("E");

  if (child_exited)
    vlg_warn(vlg, "Child exited\n");
  
  evnt_timeout_exit();
  cntl_child_free();
  evnt_close_all();

  http_req_exit();
  mime_types_exit();
  
  vlg_free(vlg);
  vlg_exit();

  vstr_free_base(serv->default_hostname);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
