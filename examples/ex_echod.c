
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

#include "opt.h"

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

#define TRUE 1
#define FALSE 0

/* only get accept() events when there is readable data, or seconds expire */
#define CONF_SERV_DEF_TCP_DEFER_ACCEPT 16

#define CONF_BUF_SZ (128 - sizeof(Vstr_node_buf))
#define CONF_MEM_PREALLOC_MIN (16  * 1024)
#define CONF_MEM_PREALLOC_MAX (128 * 1024)

/* is the data is less than this, queue instead of hitting write */
#define CL_MAX_WAIT_SEND 16

#define CONF_READ_CALL_LIMIT 4

#define CONF_DATA_CONNECTION_MAX (256 * 1024)

struct con
{
 struct Evnt evnt[1];
};

static struct Evnt *acpt_evnt = NULL;

static unsigned int client_timeout = (2 * 60); /* 2 minutes */

static const char *server_ipv4_address = NULL;
static short server_port = 7;

static unsigned int server_max_clients = 0;

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
  
  vstr_mov(con->evnt->io_w, con->evnt->io_w->len,
           con->evnt->io_r, 1, con->evnt->io_r->len);
  
  return (evnt_send_add(con->evnt, FALSE, CL_MAX_WAIT_SEND));

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

static struct Evnt *serv_cb_func_accept(int fd,
                                        struct sockaddr *sa, socklen_t len)
{
  struct con *con = malloc(sizeof(struct con));

  if (server_max_clients && (evnt_num_all() >= server_max_clients))
    goto make_acpt_fail;

  if (sa->sa_family != AF_INET) /* only support IPv4 atm. */
    goto make_acpt_fail;
  
  if (!con || !evnt_make_acpt(con->evnt, fd, sa, len))
    goto make_acpt_fail;
  
  if (!evnt_sc_timeout_via_mtime(con->evnt, client_timeout))
    goto timer_add_fail;

  vlg_info(vlg, "CONNECT from[$<sa:%p>]\n", con->evnt->sa);
  
  con->evnt->cbs->cb_func_recv = serv_cb_func_recv;
  con->evnt->cbs->cb_func_send = serv_cb_func_send;
  con->evnt->cbs->cb_func_free = serv_cb_func_free;
  
  return (con->evnt);

 timer_add_fail:
  evnt_free(con->evnt);
 make_acpt_fail:
  free(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, __func__));
}

static void serv_make_bind(const char *acpt_addr, short acpt_port)
{
  if (!(acpt_evnt = malloc(sizeof(struct Evnt))))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", __func__));
  
  if (!evnt_make_bind_ipv4(acpt_evnt, acpt_addr, acpt_port))
    vlg_err(vlg, 2, "%s: %m\n", __func__);
  
  acpt_evnt->cbs->cb_func_accept = serv_cb_func_accept;
  acpt_evnt->cbs->cb_func_free   = serv_cb_func_acpt_free;

  evnt_fd_set_defer_accept(acpt_evnt, CONF_SERV_DEF_TCP_DEFER_ACCEPT);
}

static void usage(const char *program_name, int ret, const char *prefix)
{
  Vstr_base *out = vstr_make_base(NULL);

  if (!out)
    errno = ENOMEM, err(EXIT_FAILURE, "usage");

  vstr_add_fmt(out, 0, "%s\n\
 Format: %s [-dHhnPtV]\n\
    --daemon          - Become a daemon.\n\
    --chroot          - Change root.\n\
    --drop-privs      - Drop privilages, after startup.\n\
    --priv-uid        - Drop privilages to this uid.\n\
    --priv-gid        - Drop privilages to this gid.\n\
    --pid-file        - Log pid to file.\n\
    --cntl-file       - Create control file.\n\
    --accept-filter-file\n\
                      - Load Linux Socket Filter code for accept().\n\
    --processes       - Number of processes to use (default: 1).\n\
    --debug -d        - Enable debug info.\n\
    --host -H         - IPv4 address to bind (def: \"all\").\n\
    --help -h         - Print this message.\n\
    --port -P         - Port to bind to.\n\
    --max-clients -M  - Max clients allowed to connect (0 = no limit).\n\
    --nagle -n        - Enable/disable nagle TCP option.\n\
    --timeout -t      - Timeout (usecs) for connections.\n\
    --version -V      - Print the version string.\n\
",
          prefix, program_name);

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
   {NULL, 0, NULL, 0}
  };
  OPT_SC_SERV_DECL_OPTS();
  
  program_name = opt_program_name(argv[0], "jechod");

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

        OPT_SC_SERV_OPTS();
        
      case 't': client_timeout      = atoi(optarg); break;
      case 'H': server_ipv4_address = optarg;       break;
      case 'M': server_max_clients  = atoi(optarg); break;
      case 'P': server_port         = atoi(optarg); break;

      case 'd': vlg_debug(vlg);                     break;
        
      case 'n': OPT_TOGGLE_ARG(evnt_opt_nagle);     break;
        
      ASSERT_NO_SWITCH_DEF();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 0)
    usage(program_name, EXIT_FAILURE, " Too many arguments.\n");

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
  
  if (child_exited)
    vlg_warn(vlg, "Child exited\n");
  
  evnt_timeout_exit();
  cntl_child_free();  
  evnt_close_all();
  
  vlg_free(vlg);

  vlg_exit();

  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
         
