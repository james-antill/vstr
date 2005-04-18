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
/* "simple" echo server */

#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netdb.h>
#include <signal.h>
#include <grp.h>

#ifdef __linux__
# include <sys/prctl.h>
#else
# define prctl(x1, x2, x3, x4, x5) 0
#endif

#include <err.h>

#include <socket_poll.h>
#include <timer_q.h>

#include "opt_serv.h"

#include "cntl.h"
#include "date.h"

#include "evnt.h"

#define EX_UTILS_NO_USE_INIT  1
#define EX_UTILS_NO_USE_EXIT  1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_OPEN  1
#define EX_UTILS_NO_USE_GET   1
#define EX_UTILS_RET_FAIL     1
#define EX_UTILS_NO_USE_IO_FD 1
#include "ex_utils.h"

#include "mk.h"

MALLOC_CHECK_DECL();

#define CONF_SERV_DEF_PORT    7

#define CONF_BUF_SZ (128 - sizeof(Vstr_node_buf))
#define CONF_MEM_PREALLOC_MIN (16  * 1024)
#define CONF_MEM_PREALLOC_MAX (128 * 1024)

/* is the data is less than this, queue instead of hitting write */
#define SERV_CONF_MAX_WAIT_SEND 16

#define CONF_READ_CALL_LIMIT 4

#define CONF_DATA_CONNECTION_MAX (256 * 1024)

struct con
{
 struct Evnt evnt[1];
};

static struct Evnt *acpt_evnt = NULL;

static OPT_SERV_CONF_DECL_OPTS(opts, CONF_SERV_DEF_PORT);

static Vlg *vlg = NULL;

static int serv_recv(struct con *con)
{
  unsigned int num = CONF_READ_CALL_LIMIT;
  int done = FALSE;
  
  while (num--)
    if (evnt_cb_func_recv(con->evnt))
      done = TRUE;
    else if (done)
      break;
    else
      goto malloc_bad;

  vlg_dbg3(vlg, "DATA:\n$<vstr.hexdump:%p%zu%zu>",
           con->evnt->io_r, (size_t)1, con->evnt->io_r->len);
  
  vstr_mov(con->evnt->io_w, con->evnt->io_w->len,
           con->evnt->io_r, 1, con->evnt->io_r->len);
  
  return (evnt_send_add(con->evnt, FALSE, SERV_CONF_MAX_WAIT_SEND));

 malloc_bad:
  con->evnt->io_r->conf->malloc_bad = FALSE;
  con->evnt->io_w->conf->malloc_bad = FALSE;
  return (FALSE);
}

static int serv_cb_func_recv(struct Evnt *evnt)
{
  return (serv_recv((struct con *)evnt));
}

static int serv_send(struct con *con)
{
  if (!evnt_cb_func_send(con->evnt))
    return (FALSE);

  if (con->evnt->io_w->len > CONF_DATA_CONNECTION_MAX)
    evnt_wait_cntl_del(con->evnt, POLLIN);
  else
    evnt_wait_cntl_add(con->evnt, POLLIN);

  return (TRUE);
}

static int serv_cb_func_send(struct Evnt *evnt)
{
  return (serv_send((struct con *)evnt));
}

static void serv_cb_func_free(struct Evnt *evnt)
{
  struct con *con = (struct con *)evnt;

  if (acpt_evnt && opts->max_connections &&
      (evnt_num_all() < opts->max_connections))
    evnt_wait_cntl_add(acpt_evnt, POLLIN);

  evnt_vlg_stats_info(evnt, "FREE");

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

static struct Evnt *serv_cb_func_accept(struct Evnt *from_evnt, int fd,
                                        struct sockaddr *sa, socklen_t len)
{
  struct con *con = malloc(sizeof(struct con));

  (void)from_evnt; /* FIXME: */
  
  if (sa->sa_family != AF_INET) /* only support IPv4 atm. */
    goto sa_fail;
  
  if (!con || !evnt_make_acpt(con->evnt, fd, sa, len))
    goto make_acpt_fail;
  
  if (!evnt_sc_timeout_via_mtime(con->evnt, opts->idle_timeout * 1000))
    goto evnt_fail;

  con->evnt->cbs->cb_func_recv = serv_cb_func_recv;
  con->evnt->cbs->cb_func_send = serv_cb_func_send;
  con->evnt->cbs->cb_func_free = serv_cb_func_free;
  
  vlg_info(vlg, "CONNECT from[$<sa:%p>]\n", con->evnt->sa);
  
  if (opts->max_connections && (evnt_num_all() >= opts->max_connections))
    evnt_wait_cntl_del(acpt_evnt, POLLIN);

  return (con->evnt);

 evnt_fail:
  evnt_free(con->evnt);
  return (NULL);
  
 make_acpt_fail:
  free(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, __func__));
 sa_fail:
  free(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, "%s: HTTPD sa == ipv4 fail\n", "accept"));
}

static void serv_make_bind(const char *acpt_addr, short acpt_port)
{
  if (!(acpt_evnt = malloc(sizeof(struct Evnt))))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", __func__));

  if (!evnt_make_bind_ipv4(acpt_evnt, acpt_addr, acpt_port, opts->q_listen_len))
    vlg_err(vlg, 2, "%s: %m\n", __func__);
  
  acpt_evnt->cbs->cb_func_accept = serv_cb_func_accept;
  acpt_evnt->cbs->cb_func_free   = serv_cb_func_acpt_free;

  if (opts->defer_accept)
    evnt_fd_set_defer_accept(acpt_evnt, opts->defer_accept);
}

static void usage(const char *program_name, int ret, const char *prefix)
{
  Vstr_base *out = vstr_make_base(NULL);

  if (!out)
    errno = ENOMEM, err(EXIT_FAILURE, "usage");

  vstr_add_fmt(out, 0, "%s\n\
 Format: %s [options]\n\
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
",
               prefix, program_name,
               opt_def_toggle(FALSE), opt_def_toggle(FALSE),
               opt_def_toggle(EVNT_CONF_NAGLE));

  if (io_put_all(out, ret ? STDERR_FILENO : STDOUT_FILENO) == IO_FAIL)
    err(EXIT_FAILURE, "write");
  
  exit (ret);
}

static void serv__sig_crash(int s_ig_num)
{
  vlg_abort(vlg, "SIG: %d\n", s_ig_num);
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
  
  if (sigaction(SIGPIPE, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");

  sa.sa_handler = serv__sig_crash;
  
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
  sa.sa_handler = serv__sig_child;
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  
  /*
  sa.sa_handler = ex_http__sig_shutdown;
  if (sigaction(SIGTERM, &sa, NULL) == -1)
    err(EXIT_FAILURE, "signal init");
  */
}


static void serv_cmd_line(int argc, char *argv[])
{
  int optchar = 0;
  const char *program_name = NULL;
  struct option long_options[] =
  {
   OPT_SERV_DECL_GETOPTS(),
   {NULL, 0, NULL, 0}
  };
  
  program_name = opt_program_name(argv[0], "jechod");

  if (!opt_serv_conf_init(opts))
    errno = ENOMEM, err(EXIT_FAILURE, "options");
  
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

        vstr_add_fmt(out, 0, " %s version %s.\n", program_name, "0.9.1");

        if (io_put_all(out, STDOUT_FILENO) == IO_FAIL)
          err(EXIT_FAILURE, "write");

        exit (EXIT_SUCCESS);
      }

      OPT_SERV_GETOPTS(opts);

      case 'C':
        //        if (!opt_serv_conf_parse(opts, optarg))
        //          errx(EXIT_FAILURE, "Failed to parse configuration file: %s", optarg);
        break;
      
      ASSERT_NO_SWITCH_DEF();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 0)
    usage(program_name, EXIT_FAILURE, " Too many arguments.\n");

  if (opts->become_daemon)
  {
    if (daemon(FALSE, FALSE) == -1)
      err(EXIT_FAILURE, "daemon");
    vlg_daemon(vlg, program_name);
  }
  
  {
    const char *ipv4_address = NULL;
    const char *pid_file = NULL;
    const char *cntl_file = NULL;
    const char *acpt_filter_file = NULL;
    const char *chroot_dir = NULL;
    
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
  
  if (pid_file)
    vlg_pid_file(vlg, pid_file);
  
  if (cntl_file)
    cntl_make_file(vlg, acpt_evnt, cntl_file);
  
  if (acpt_filter_file)
    if (!evnt_fd_set_filter(acpt_evnt, acpt_filter_file))
      errx(EXIT_FAILURE, "set_filter");
  
  if (chroot_dir) /* so syslog can log in localtime */
  {
    time_t now = time(NULL);
    (void)localtime(&now);

    vlg_sc_bind_mount(chroot_dir);
  }
  
  /* after daemon so syslog works */
  if (chroot_dir && ((chroot(chroot_dir) == -1) || (chdir("/") == -1)))
    vlg_err(vlg, EXIT_FAILURE, "chroot(%s): %m\n", chroot_dir);
  
  if (opts->drop_privs)
  {
    OPT_SC_RESOLVE_UID(opts);
    OPT_SC_RESOLVE_GID(opts);
    
    if (setgroups(1, &opts->priv_gid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setgroups(%ld): %m\n", (long)opts->priv_gid);
    
    if (setgid(opts->priv_gid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setgid(%ld): %m\n", (long)opts->priv_gid);
    
    if (setuid(opts->priv_uid) == -1)
      vlg_err(vlg, EXIT_FAILURE, "setuid(%ld): %m\n", (long)opts->priv_uid);
  }

  if (opts->num_procs > 1)
    cntl_sc_multiproc(vlg, acpt_evnt, opts->num_procs,
                      !!cntl_file, opts->use_pdeathsig);
  }
  opt_serv_conf_free(opts);
  
  /*  if (make_dumpable && (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1))
   *    vlg_warn(vlg, "prctl(SET_DUMPABLE, TRUE): %m\n"); */

  vlg_info(vlg, "READY [$<sa:%p>]\n", acpt_evnt->sa);
}

static void serv_init(void)
{
  if (!vstr_init())
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_TYPE_GRPALLOC_CACHE,
                      VSTR_TYPE_CNTL_CONF_GRPALLOC_IOVEC))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, CONF_BUF_SZ))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  if (!vstr_make_spare_nodes(NULL, VSTR_TYPE_NODE_BUF,
                             (CONF_MEM_PREALLOC_MIN / CONF_BUF_SZ)))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$') ||
      !vstr_sc_fmt_add_all(NULL) ||
      !vlg_sc_fmt_add_all(NULL))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
  
  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  vlg_init();

  if (!(vlg = vlg_make()))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  evnt_logger(vlg);

  evnt_epoll_init();
  evnt_timeout_init();
  
  serv_signals();
}

static void serv_cntl_resources(void)
{
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BASE, 0, 20);
  
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BUF,
                 0, (CONF_MEM_PREALLOC_MAX / CONF_BUF_SZ));
}

int main(int argc, char *argv[])
{  
  serv_init();
  
  serv_cmd_line(argc, argv);
  
  while (evnt_waiting())
  {
    evnt_sc_main_loop(SERV_CONF_MAX_WAIT_SEND);
    if (child_exited)
    {
      vlg_warn(vlg, "Child exited.\n");
      evnt_close(acpt_evnt);
      evnt_scan_q_close();
      child_exited = FALSE;
    }
    serv_cntl_resources();
  }
  evnt_out_dbg3("E");
  
  evnt_timeout_exit();
  cntl_child_free();  
  evnt_close_all();
  
  vlg_free(vlg);

  vlg_exit();

  vstr_exit();

  MALLOC_CHECK_EMPTY();
  
  exit (EXIT_SUCCESS);
}
         
