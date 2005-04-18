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
#define _GNU_SOURCE 1 /* strsignal() / posix_fadvise64 */
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

#include <signal.h>
#include <grp.h>

/* need better way to test for this */
#ifndef __GLIBC__
# define strsignal(x) ""
#endif

#define EX_UTILS_NO_USE_INIT  1
#define EX_UTILS_NO_USE_EXIT  1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_OPEN  1
#define EX_UTILS_NO_USE_GET   1
#define EX_UTILS_NO_USE_IO_FD 1
#define EX_UTILS_RET_FAIL     1
#include "ex_utils.h"

#include "mk.h"

MALLOC_CHECK_DECL();

#include "cntl.h"
#include "date.h"

#define HTTPD_HAVE_GLOBAL_OPTS 1

#include "httpd.h"
#include "httpd_policy.h"

#define CONF_BUF_SZ (128 - sizeof(Vstr_node_buf))
#define CONF_MEM_PREALLOC_MIN (16  * 1024)
#define CONF_MEM_PREALLOC_MAX (128 * 1024)

#define CLEN VSTR__AT_COMPILE_STRLEN

/* is the cstr a prefix of the vstr */
#define VPREFIX(vstr, p, l, cstr)                                       \
    (((l) >= CLEN(cstr)) &&                                             \
     vstr_cmp_buf_eq(vstr, p, CLEN(cstr), cstr, CLEN(cstr)))
/* is the cstr a suffix of the vstr */
#define VSUFFIX(vstr, p, l, cstr)                                       \
    (((l) >= CLEN(cstr)) &&                                             \
     vstr_cmp_eod_buf_eq(vstr, p, l, cstr, CLEN(cstr)))

/* is the cstr a prefix of the vstr, no case */
#define VIPREFIX(vstr, p, l, cstr)                                      \
    (((l) >= CLEN(cstr)) &&                                             \
     vstr_cmp_case_buf_eq(vstr, p, CLEN(cstr), cstr, CLEN(cstr)))

/* for simplicity */
#define VEQ(vstr, p, l, cstr) vstr_cmp_cstr_eq(vstr, p, l, cstr)

static struct Evnt *httpd_acpt_evnt = NULL;

static Vlg *vlg = NULL;

static void usage(const char *program_name, int ret, const char *prefix)
{
  Vstr_base *out = vstr_make_base(NULL);

  if (!out)
    errno = ENOMEM, err(EXIT_FAILURE, "usage");

  vstr_add_fmt(out, 0, "%s\n\
 Format: %s [options] <dir>\n\
  Daemon options\n\
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
    --debug -d        - Raise debug level (can be used upto 3 times).\n\
    --host -H         - IPv4 address to bind (default: \"all\").\n\
    --help -h         - Print this message.\n\
    --max-connections\n\
                  -M  - Max connections allowed (0 = no limit).\n\
    --nagle -n        - Toggle usage of nagle TCP option%s.\n\
    --port -P         - Port to bind to.\n\
    --idle-timeout -t - Timeout (usecs) for connections that are idle.\n\
    --defer-accept    - Time to defer dataless connections (default: 8s)\n\
    --version -V      - Print the version string.\n\
\n\
  HTTPD options\n\
    --mmap            - Toggle use of mmap() to load files%s.\n\
    --sendfile        - Toggle use of sendfile() to load files%s.\n\
    --keep-alive      - Toggle use of Keep-Alive handling%s.\n\
    --keep-alive-1.0  - Toggle use of Keep-Alive handling for HTTP/1.0%s.\n\
    --virtual-hosts\n\
    --vhosts          - Toggle use of Host header%s.\n\
    --range           - Toggle use of partial responces%s.\n\
    --range-1.0       - Toggle use of partial responces for HTTP/1.0%s.\n\
    --public-only     - Toggle use of public only privilages%s.\n\
    --directory-filename\n\
    --dir-filename    - Filename to use when requesting directories.\n\
    --server-name     - Contents of server header used in replies.\n\
    --gzip-content-replacement\n\
                      - Toggle use of gzip content replacement%s.\n\
    --mime-types-main - Main mime types filename (default: /etc/mime.types).\n\
    --mime-types-xtra - Additional mime types filename.\n\
    --error-406       - Toggle sending 406 responses%s.\n\
    --canonize-host   - Strip leading 'www.', strip trailing '.'%s.\n\
    --error-host-400  - Give an 400 error for a bad host%s.\n\
    --check-host      - Whether we check host headers at all%s.\n\
    --unspecified-hostname\n\
                      - Used for req with no Host header (default is hostname).\n\
    --max-header-sz   - Max size of http header (0 = no limit).\n\
",
               prefix, program_name,
               opt_def_toggle(FALSE), opt_def_toggle(FALSE),
               opt_def_toggle(EVNT_CONF_NAGLE),
               opt_def_toggle(HTTPD_CONF_USE_MMAP),
               opt_def_toggle(HTTPD_CONF_USE_SENDFILE),
               opt_def_toggle(HTTPD_CONF_USE_KEEPA),
               opt_def_toggle(HTTPD_CONF_USE_KEEPA_1_0),
               opt_def_toggle(HTTPD_CONF_USE_VHOSTS),
               opt_def_toggle(HTTPD_CONF_USE_RANGE),
               opt_def_toggle(HTTPD_CONF_USE_RANGE_1_0),
               opt_def_toggle(HTTPD_CONF_USE_PUBLIC_ONLY),
               opt_def_toggle(HTTPD_CONF_USE_GZIP_CONTENT_REPLACEMENT),
               opt_def_toggle(HTTPD_CONF_USE_ERR_406),
               opt_def_toggle(HTTPD_CONF_USE_CANONIZE_HOST),
               opt_def_toggle(HTTPD_CONF_USE_HOST_ERR_400),
               opt_def_toggle(HTTPD_CONF_USE_CHK_HOST_ERR));

  if (io_put_all(out, ret ? STDERR_FILENO : STDOUT_FILENO) == IO_FAIL)
    err(EXIT_FAILURE, "write");
  
  exit (ret);
}

#define SERV__SIG_OR_ERR(x)                     \
    if (sigaction(x, &sa, NULL) == -1)          \
      err(EXIT_FAILURE, "signal(" #x ")")

static void serv__sig_crash(int s_ig_num)
{
  vlg_sig_abort(vlg, "SIG[%d]: %s\n", s_ig_num, strsignal(s_ig_num));
}

static void serv__sig_raise_cont(int s_ig_num)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    err(EXIT_FAILURE, "signal init");

  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = SIG_DFL;
  SERV__SIG_OR_ERR(s_ig_num);

  vlg_sig_info(vlg, "SIG[%d]: %s\n", s_ig_num, strsignal(s_ig_num));
  raise(s_ig_num);
}

static void serv__sig_cont(int s_ig_num)
{
  if (0) /* s_ig_num == SIGCONT) */
  {
    struct sigaction sa;
  
    if (sigemptyset(&sa.sa_mask) == -1)
      err(EXIT_FAILURE, "signal init");

    sa.sa_flags   = SA_RESTART;
    sa.sa_handler = serv__sig_raise_cont;
    SERV__SIG_OR_ERR(SIGTSTP);
  }
  
  vlg_sig_info(vlg, "SIG[%d]: %s\n", s_ig_num, strsignal(s_ig_num));
}

static void serv__sig_child(int s_ig_num)
{
  ASSERT(s_ig_num == SIGCHLD);
  evnt_child_exited = TRUE;
}

static void serv_signals(void)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    err(EXIT_FAILURE, "signal init %s", "sigemptyset");
  
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

  sa.sa_flags   = SA_RESTART; /* FIXME: probably needs to move to evnt_sc_* */
  sa.sa_handler = serv__sig_child;
  SERV__SIG_OR_ERR(SIGCHLD);
  
  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = serv__sig_cont; /* print, and do nothing */
  
  SERV__SIG_OR_ERR(SIGUSR1);
  SERV__SIG_OR_ERR(SIGUSR2);
  SERV__SIG_OR_ERR(SIGHUP);
  SERV__SIG_OR_ERR(SIGCONT);
  
  sa.sa_handler = serv__sig_raise_cont; /* queue print, and re-raise */
  
  SERV__SIG_OR_ERR(SIGTSTP);
  SERV__SIG_OR_ERR(SIGTERM);
  
  /*
  sa.sa_handler = ex_http__sig_shutdown;
  if (sigaction(SIGTERM, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  */
}
#undef SERV__SIG_OR_ERR

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
      !VSTR_SC_FMT_ADD(NULL, http_fmt_add_vstr_add_vstr,
                       "<http-esc.vstr", "p%zu%zu", ">") ||
      !VSTR_SC_FMT_ADD(NULL, http_fmt_add_vstr_add_sect_vstr,
                       "<http-esc.vstr.sect", "p%p%u", ">"))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!(vlg = vlg_make()))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!VSTR_SC_FMT_ADD(vlg->out_vstr->conf, http_fmt_add_vstr_add_vstr,
                       "<http-esc.vstr", "p%zu%zu", ">") ||
      !VSTR_SC_FMT_ADD(vlg->out_vstr->conf, http_fmt_add_vstr_add_sect_vstr,
                       "<http-esc.vstr.sect", "p%p%u", ">"))

  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  evnt_logger(vlg);
  evnt_epoll_init();
  evnt_timeout_init();

  httpd_init(vlg);
  
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

static int serv_cb_func_send(struct Evnt *evnt)
{
  return (httpd_serv_send((struct Con *)evnt));
}

static int serv_cb_func_recv(struct Evnt *evnt)
{
  return (httpd_serv_recv((struct Con *)evnt));
}

static void serv_cb_func_free(struct Evnt *evnt)
{
  struct Con *con = (struct Con *)evnt;
  Opt_serv_opts *opts = httpd_opts->s;
  struct Acpt_data *acpt_data = con->acpt_sa_ref->ptr;
  
  if (acpt_data->evnt &&
      opts->max_connections && (evnt_num_all() < opts->max_connections))
    evnt_wait_cntl_add(acpt_data->evnt, POLLIN);

  httpd_fin_fd_close(con);

  evnt_vlg_stats_info(evnt, "FREE");

  acpt_data = con->acpt_sa_ref->ptr;
  if (acpt_data->evnt)
    evnt_stats_add(acpt_data->evnt, con->evnt);

  vstr_ref_del(con->acpt_sa_ref);
  
  F(con);
}

static void serv_cb_func_acpt_free(struct Evnt *evnt)
{
  struct Acpt_listener *acpt_listener = (struct Acpt_listener *)evnt;
  struct Acpt_data *acpt_data = acpt_listener->ref->ptr;

  evnt_vlg_stats_info(acpt_listener->evnt, "ACCEPT FREE");

  ASSERT(httpd_acpt_evnt == acpt_listener->evnt);

  httpd_acpt_evnt = NULL;
  acpt_data->evnt = NULL;
  cntl_free_acpt(acpt_listener->evnt);

  vstr_ref_del(acpt_listener->ref);
  
  F(acpt_listener);
}

static struct Evnt *serv_cb_func_accept(struct Evnt *from_evnt, int fd,
                                        struct sockaddr *sa, socklen_t len)
{
  struct Acpt_listener *acpt_listener = (struct Acpt_listener *)from_evnt;
  struct Con *con = MK(sizeof(struct Con));
  Opt_serv_opts *opts = httpd_opts->s;

  ASSERT(httpd_acpt_evnt == from_evnt);
  
  if (sa->sa_family != AF_INET) /* only support IPv4 atm. */
    goto sa_fail;
  
  if (!con || !evnt_make_acpt(con->evnt, fd, sa, len))
    goto make_acpt_fail;

  /* FIXME: allow policy changes for idle timeout */
  if (!evnt_sc_timeout_via_mtime(con->evnt, httpd_opts->s->idle_timeout * 1000))
  {
    errno = ENOMEM;
    vlg_info(vlg, "ERRCON from[$<sa:%p>]: %m\n", con->evnt->sa);
    goto evnt_fail;
  }

  if (!httpd_con_init(con, acpt_listener))
    goto evnt_fail;

  if (con->evnt->flag_q_closed)
    return (con->evnt);
  
  /* if it's free'd before this point, use generic free cb */
  con->evnt->cbs->cb_func_recv       = serv_cb_func_recv;
  con->evnt->cbs->cb_func_send       = serv_cb_func_send;
  con->evnt->cbs->cb_func_free       = serv_cb_func_free;

  vlg_info(vlg, "CONNECT from[$<sa:%p>]\n", con->evnt->sa);
  
  if (opts->max_connections && (evnt_num_all() >= opts->max_connections))
    evnt_wait_cntl_del(from_evnt, POLLIN);
  
  return (con->evnt);
  
 evnt_fail:
  evnt_free(con->evnt);
  return (NULL);
  
 make_acpt_fail:
  F(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, "%s: %m\n", "accept"));
  
 sa_fail:
  F(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, "%s: HTTPD sa == ipv4 fail\n", "accept"));
}

static void serv_make_bind(const char *acpt_addr, unsigned short acpt_port)
{
  struct sockaddr_in *sinv4 = NULL;
  struct Acpt_listener *acpt_listener = NULL;
  struct Acpt_data *acpt_data = NULL;
  Vstr_ref *ref = NULL;
  
  if (!(acpt_listener = MK(sizeof(struct Acpt_listener))))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "make_bind(%s, %hu): %m\n",
                  acpt_addr, acpt_port));
  httpd_acpt_evnt = acpt_listener->evnt;

  if (!(ref = vstr_ref_make_malloc(sizeof(struct Acpt_data))))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "make_bind(%s, %hu): %m\n",
                  acpt_addr, acpt_port));
  acpt_listener->ref = ref;
  
  if (!evnt_make_bind_ipv4(acpt_listener->evnt, acpt_addr, acpt_port,
                           httpd_opts->s->q_listen_len))
    vlg_err(vlg, 2, "make_bind(%s, %hu): %m\n", acpt_addr, acpt_port);
  
  acpt_listener->evnt->cbs->cb_func_accept = serv_cb_func_accept;
  acpt_listener->evnt->cbs->cb_func_free   = serv_cb_func_acpt_free;

  if (httpd_opts->s->defer_accept)
    evnt_fd_set_defer_accept(acpt_listener->evnt, httpd_opts->s->defer_accept);

  sinv4 = EVNT_SA_IN(acpt_listener->evnt);
  ASSERT(!httpd_opts->s->tcp_port ||
	 (httpd_opts->s->tcp_port == ntohs(sinv4->sin_port)));
  httpd_opts->s->tcp_port = ntohs(sinv4->sin_port);
  
  acpt_data = ref->ptr;
  memcpy(acpt_data->sa, sinv4, sizeof(struct sockaddr_in));
  acpt_data->evnt = acpt_listener->evnt;
}

#define VCMP_MT_EQ_ALL(x, y, z)               \
    vstr_cmp_eq(x -> mime_types_ ## z, 1, x -> mime_types_ ## z ->len,  \
                y -> mime_types_ ## z, 1, y -> mime_types_ ## z ->len)
static Httpd_policy_opts *serv__mime_types_eq(Httpd_policy_opts *node)
{
  Httpd_policy_opts *scan = httpd_opts->def_policy;

  ASSERT(node);
  
  while (scan != node)
  {
    if (VCMP_MT_EQ_ALL(scan, node, main) && VCMP_MT_EQ_ALL(scan, node, xtra))
      return (scan);
    scan = scan->next;
  }
  
  return (NULL);
}
#undef VCMP_MT_EQ_ALL

static void serv_mime_types(const char *program_name)
{
  Httpd_policy_opts *scan = httpd_opts->def_policy;
    
  while (scan)
  {
    Httpd_policy_opts *prev = NULL;
    
    if (!mime_types_init(scan->mime_types,
                         scan->mime_types_default_type, 1,
                         scan->mime_types_default_type->len))
      errno = ENOMEM, err(EXIT_FAILURE, "init");

    if ((prev = serv__mime_types_eq(scan)))
      mime_types_combine_filedata(scan->mime_types, prev->mime_types);
    else
    {
      const char *mime_types_main = NULL;
      const char *mime_types_xtra = NULL;
  
      OPT_SC_EXPORT_CSTR(mime_types_main, scan->mime_types_main, FALSE,
                         "MIME types main file");
      OPT_SC_EXPORT_CSTR(mime_types_xtra, scan->mime_types_xtra, FALSE,
                         "MIME types extra file");

      if (!mime_types_load_simple(scan->mime_types, mime_types_main))
        err(EXIT_FAILURE, "load_mime(%s)", mime_types_main);
  
      if (!mime_types_load_simple(scan->mime_types, mime_types_xtra))
        err(EXIT_FAILURE, "load_mime(%s)", mime_types_main);
    }
    
    scan = scan->next;
  }
}

static void serv_canon_policies(void)
{
  Httpd_policy_opts *scan = httpd_opts->def_policy;
  
  while (scan)
  { /* check variables for things which will screw us too much */
    Vstr_base *s1 = NULL;
    Vstr_base *s2 = NULL;

    s1 = scan->document_root;
    if (!httpd_canon_dir_path(s1))
      VLG_ERRNOMEM((vlg, EXIT_FAILURE, "canon_dir_path($<vstr:%p%zu%zu>): %m\n",
                    s1, (size_t)1, s1->len));
  
    s1 = scan->req_configuration_dir;
    if (!httpd_canon_dir_path(s1))
      VLG_ERRNOMEM((vlg, EXIT_FAILURE, "canon_dir_path($<vstr:%p%zu%zu>): %m\n",
                    s1, (size_t)1, s1->len));

    s1 = scan->default_hostname;
    if (!httpd_valid_url_filename(s1, 1, s1->len))
      vstr_del(s1, 1, s1->len);
    s2 = httpd_opts->def_policy->default_hostname;
    if (!s1->len && !vstr_add_vstr(s1, s1->len,
                                   s2, 1, s2->len, VSTR_TYPE_ADD_BUF_REF))
      VLG_ERRNOMEM((vlg, EXIT_FAILURE, "hostname(): %m\n"));
    
    s1 = scan->dir_filename;
    if (!httpd_valid_url_filename(s1, 1, s1->len) &&
        !vstr_sub_cstr_ptr(s1, 1, s1->len, HTTPD_CONF_DEF_DIR_FILENAME))
      VLG_ERRNOMEM((vlg, EXIT_FAILURE, "dir_filename(): %m\n"));
    scan = scan->next;
  }
}

static void serv_cmd_line(int argc, char *argv[])
{
  int optchar = 0;
  const char *program_name = NULL;
  struct option long_options[] =
  {
   OPT_SERV_DECL_GETOPTS(),
   
   {"configuration-file", required_argument, NULL, 'C'},
   {"config-file",        required_argument, NULL, 'C'},
   {"configuration-data-daemon", required_argument, NULL, 143},
   {"config-data-daemon",        required_argument, NULL, 143},
   {"configuration-data-jhttpd", required_argument, NULL, 144},
   {"config-data-jhttpd",        required_argument, NULL, 144},
   
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
   /* 138 */
   {"error-406", optional_argument, NULL, 139},
   /* 140 */
   {"mime-types-main", required_argument, NULL, 141},
   {"mime-types-extra", required_argument, NULL, 142},
   {"mime-types-xtra", required_argument, NULL, 142},
   /* 143 -- config data above */
   /* 144 -- config data above */
   {"unspecified-hostname", required_argument, NULL, 145},
   {"canonize-host", optional_argument, NULL, 146},
   {"error-host-400", optional_argument, NULL, 147},
   {"check-host", optional_argument, NULL, 148},
   /* {"404-file", required_argument, NULL, 0}, */
   {NULL, 0, NULL, 0}
  };
  const char *chroot_dir = NULL;
  const char *document_root = NULL;
  Vstr_base *out = vstr_make_base(NULL);
  Httpd_policy_opts *popts = httpd_opts->def_policy;

  if (!out)
    errno = ENOMEM, err(EXIT_FAILURE, "command line");

  evnt_opt_nagle = TRUE;
  
  program_name = opt_program_name(argv[0], "jhttpd");

  if (!opt_serv_conf_init(httpd_opts->s) ||
      !httpd_conf_main_init(httpd_opts))
    errno = ENOMEM, err(EXIT_FAILURE, "options");
  
  while ((optchar = getopt_long(argc, argv, "C:dhH:M:nP:t:V",
                                long_options, NULL)) != -1)
  {
    switch (optchar)
    {
      case '?': usage(program_name, EXIT_FAILURE, "");
      case 'h': usage(program_name, EXIT_SUCCESS, "");
        
      case 'V':
        vstr_add_fmt(out, 0, " %s version %s.\n",
                     program_name, HTTPD_CONF_VERSION);
        
        if (io_put_all(out, STDOUT_FILENO) == IO_FAIL)
          err(EXIT_FAILURE, "write");
        
        exit (EXIT_SUCCESS);
      
      OPT_SERV_GETOPTS(httpd_opts->s);

      case 'C':
        if (!httpd_conf_main_parse_file(out, httpd_opts, optarg))
          goto out_err_conf_msg;
        break;
      case 143: /* FIXME: ... need to integrate */
        if (!opt_serv_conf_parse_cstr(out, httpd_opts->s, optarg))
          goto out_err_conf_msg;
      break;
      case 144:
        if (!httpd_conf_main_parse_cstr(out, httpd_opts, optarg))
          goto out_err_conf_msg;
        break;
        
      case 128: OPT_NUM_NR_ARG(popts->max_header_sz, "max header size"); break;
        
      case  31: OPT_TOGGLE_ARG(popts->use_sendfile);                 break;
      case  30: OPT_TOGGLE_ARG(popts->use_mmap);                     break;
        
      case 129: OPT_TOGGLE_ARG(popts->use_keep_alive);               break;
      case 130: OPT_TOGGLE_ARG(popts->use_keep_alive_1_0);           break;
      case 131: OPT_TOGGLE_ARG(popts->use_vhosts);                   break;
      case 132: OPT_TOGGLE_ARG(popts->use_range);                    break;
      case 133: OPT_TOGGLE_ARG(popts->use_range_1_0);                break;
      case 134: OPT_TOGGLE_ARG(popts->use_public_only);              break;
      case 135: OPT_VSTR_ARG(popts->dir_filename);                   break;
      case 136: OPT_VSTR_ARG(popts->server_name);                    break;
      case 137: OPT_TOGGLE_ARG(popts->use_gzip_content_replacement); break;
      case 139: OPT_TOGGLE_ARG(popts->use_err_406);                  break;
        /* case 140: */
      case 141: OPT_VSTR_ARG(popts->mime_types_main);                break;
      case 142: OPT_VSTR_ARG(popts->mime_types_xtra);                break;
        /* case 143: */
        /* case 144: */
      case 145: OPT_VSTR_ARG(popts->default_hostname);               break;
      case 146: OPT_TOGGLE_ARG(popts->use_canonize_host);            break;
      case 147: OPT_TOGGLE_ARG(popts->use_host_err_400);             break;
      case 148: OPT_TOGGLE_ARG(popts->use_chk_host_err);
        
      
      ASSERT_NO_SWITCH_DEF();
    }
  }
  vstr_free_base(out); out = NULL;
  
  argc -= optind;
  argv += optind;

  if (argc > 1)
    usage(program_name, EXIT_FAILURE, " Too many arguments.\n");

  if (argc == 1)
  {
    Vstr_base *tmp = popts->document_root;
    vstr_sub_cstr_ptr(tmp, 1, tmp->len, argv[0]);
  }

  if (!popts->document_root->len)
    usage(program_name, EXIT_FAILURE, " Not specified a document root.\n");

  if (httpd_opts->s->become_daemon)
  {
    if (daemon(FALSE, FALSE) == -1)
      err(EXIT_FAILURE, "daemon");
    vlg_daemon(vlg, program_name);
  }

  if (httpd_opts->s->rlim_file_num)
  {
    struct rlimit rlim[1];
    if (getrlimit(RLIMIT_NOFILE, rlim) == -1)
      vlg_err(vlg, EXIT_FAILURE, "getrlimit: %m\n");

    if ((httpd_opts->s->rlim_file_num > rlim->rlim_max) && !getuid())
      rlim->rlim_max = httpd_opts->s->rlim_file_num; /* if we are privilaged */
    if (httpd_opts->s->rlim_file_num < rlim->rlim_max)
      rlim->rlim_max = httpd_opts->s->rlim_file_num;
    rlim->rlim_cur = rlim->rlim_max;
	
    if (setrlimit(RLIMIT_NOFILE, rlim) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setrlimit: %m\n");
  }

  /* do for all policies */
  
  {
    const char *ipv4_address = NULL;
    const char *pid_file = NULL;
    const char *cntl_file = NULL;
    const char *acpt_filter_file = NULL;
    Opt_serv_opts *opts = httpd_opts->s;
    
    OPT_SC_EXPORT_CSTR(ipv4_address,     opts->ipv4_address,     FALSE,
                       "ipv4 address");
    OPT_SC_EXPORT_CSTR(pid_file,         opts->pid_file,         FALSE,
                       "pid file");
    OPT_SC_EXPORT_CSTR(cntl_file,        opts->cntl_file,        FALSE,
                       "control file");
    OPT_SC_EXPORT_CSTR(acpt_filter_file, opts->acpt_filter_file, FALSE,
                       "accept filter file");
    OPT_SC_EXPORT_CSTR(chroot_dir,       opts->chroot_dir,       FALSE,
                       "chroot directory");
    
    serv_make_bind(ipv4_address, opts->tcp_port);

    if (!httpd_init_default_hostname(httpd_opts->def_policy))
      errno = ENOMEM, err(EXIT_FAILURE, "default_hostname");
  
    if (pid_file)
      vlg_pid_file(vlg, pid_file);

    if (cntl_file)
      cntl_make_file(vlg, httpd_acpt_evnt, cntl_file);

    if (acpt_filter_file)
      if (!evnt_fd_set_filter(httpd_acpt_evnt, acpt_filter_file))
        errx(EXIT_FAILURE, "set_filter");
  
    serv_canon_policies();
  
    OPT_SC_EXPORT_CSTR(document_root, popts->document_root, TRUE,
                       "document root");
    
    serv_mime_types(program_name);
  
  if (opts->drop_privs)
  { /* FIXME: called from all daemons */
    OPT_SC_RESOLVE_UID(opts);
    OPT_SC_RESOLVE_GID(opts);
  }
  
  if (chroot_dir)
  { /* preload locale info. so syslog can log in localtime */
    time_t now = time(NULL);
    (void)localtime(&now);

    vlg_sc_bind_mount(chroot_dir);  
  }
  
  /* after daemon so syslog works */
  if (chroot_dir && ((chroot(chroot_dir) == -1) || (chdir("/") == -1)))
    vlg_err(vlg, EXIT_FAILURE, "chroot(%s): %m\n", chroot_dir);
    
  if (opts->drop_privs)
  { /* FIXME: called from all daemons */
    if (setgroups(1, &opts->priv_gid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setgroups(%ld): %m\n", (long)opts->priv_gid);
    
    if (setgid(opts->priv_gid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setgid(%ld): %m\n", (long)opts->priv_gid);
    
    if (setuid(opts->priv_uid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setuid(%ld): %m\n", (long)opts->priv_uid);
  }

  if (opts->num_procs > 1)
    cntl_sc_multiproc(vlg, httpd_acpt_evnt, opts->num_procs,
                      !!cntl_file, opts->use_pdeathsig);
  }

  if (!vstr_make_spare_nodes(NULL, VSTR_TYPE_NODE_BUF,
                             (CONF_MEM_PREALLOC_MIN / CONF_BUF_SZ)))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  httpd_opts->beg_time = time(NULL);
  
  vlg_info(vlg, "READY [$<sa:%p>]: %s%s%s\n", httpd_acpt_evnt->sa,
           chroot_dir ? chroot_dir : "",
           chroot_dir ?        "/" : "",
           document_root);
  
  opt_serv_conf_free(httpd_opts->s);
  return;

 out_err_conf_msg:
  vstr_add_cstr_ptr(out, out->len, "\n");
  if (io_put_all(out, STDERR_FILENO) == IO_FAIL)
    err(EXIT_FAILURE, "write");
  exit (EXIT_FAILURE);  
}

int main(int argc, char *argv[])
{
  serv_init();

  serv_cmd_line(argc, argv);

  while (evnt_waiting())
  {
    evnt_sc_main_loop(HTTPD_CONF_MAX_WAIT_SEND);
    if (evnt_child_exited)
    {
      vlg_warn(vlg, "Child exited.\n");
      evnt_close(httpd_acpt_evnt); evnt_scan_q_close();
      evnt_child_exited = FALSE;
    }
    serv_cntl_resources();
  }
  evnt_out_dbg3("E");
  
  evnt_timeout_exit();
  cntl_child_free();
  evnt_close_all();

  httpd_exit();
  
  http_req_exit();
  
  vlg_free(vlg);
  vlg_exit();

  httpd_conf_main_free(httpd_opts);
  
  vstr_exit();

  MALLOC_CHECK_EMPTY();
  
  exit (EXIT_SUCCESS);
}
