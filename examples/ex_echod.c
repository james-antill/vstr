
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <stdio.h>
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

#include "vlg_assert.h"

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
 struct Evnt ev[1];
};

static Timer_q_base *cl_timeout_base = NULL;

static struct Evnt *acpt_evnt = NULL;

static unsigned int client_timeout = (2 * 60); /* 2 minutes */

static const char *server_ipv4_address = NULL;
static short server_port = 7;

static unsigned int server_max_clients = 0;

static Vlg *vlg = NULL;

static int serv_recv(struct con *con)
{
  unsigned int num = CONF_READ_CALL_LIMIT;

  while (num--)
    if (!evnt_cb_func_recv(con->ev))
      return (FALSE);
  
  vstr_mov(con->ev->io_w, con->ev->io_w->len,
           con->ev->io_r, 1, con->ev->io_r->len);
  
  return (evnt_send_add(con->ev, FALSE, CL_MAX_WAIT_SEND));
}

static int serv_cb_func_recv(struct Evnt *evnt)
{
  return (serv_recv((struct con *)evnt));
}

static int serv_send(struct con *con)
{
  if (!evnt_cb_func_send(con->ev))
    return (FALSE);

  if (con->ev->io_w->len > CONF_DATA_CONNECTION_MAX)
    evnt_wait_cntl_del(con->ev, POLLIN);
  else
    evnt_wait_cntl_add(con->ev, POLLIN);

  return (TRUE);
}

static int serv_cb_func_send(struct Evnt *evnt)
{
  return (serv_send((struct con *)evnt));
}

static void serv_cb_func_free(struct Evnt *evnt)
{
  struct con *con = (struct con *)evnt;

  vlg_info(vlg, "FREE from[$<sa:%p>]"
           " recv[${BKMG.ju:%ju}:%ju] send[${BKMG.ju:%ju}:%ju]\n", con->ev->sa,
           con->ev->acct.bytes_r, con->ev->acct.bytes_r,
           con->ev->acct.bytes_w, con->ev->acct.bytes_w);

  free(con);
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

  if (server_max_clients && (evnt_num_all() >= server_max_clients))
    goto make_acpt_fail;

  if (sa->sa_family != AF_INET) /* only support IPv4 atm. */
    goto make_acpt_fail;
  
  if (!con || !evnt_make_acpt(con->ev, fd, sa, len))
    goto make_acpt_fail;

  tv = con->ev->ctime;
  TIMER_Q_TIMEVAL_ADD_SECS(&tv, 0, client_timeout);
  
  if (!(con->ev->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                         TIMER_Q_FLAG_NODE_DEFAULT)))
    goto timer_add_fail;

  vlg_info(vlg, "CONNECT from[$<sa:%p>]\n", con->ev->sa);
  
  con->ev->cbs->cb_func_recv = serv_cb_func_recv;
  con->ev->cbs->cb_func_send = serv_cb_func_send;
  con->ev->cbs->cb_func_free = serv_cb_func_free;
  
  return (con->ev);

 timer_add_fail:
  evnt_free(con->ev);
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

static void usage(const char *program_name, int tst_err)
{
  fprintf(tst_err ? stderr : stdout, "\n\
 Format: %s [-dHhnPtV]\n\
    --daemon          - Become a daemon.\n\
    --chroot          - Change root.\n\
    --drop-privs      - Drop privilages, after startup.\n\
    --priv-uid        - Drop privilages to this uid.\n\
    --priv-gid        - Drop privilages to this gid.\n\
    --debug -d        - Enable debug info.\n\
    --host -H         - IPv4 address to bind (def: \"all\").\n\
    --help -h         - Print this message.\n\
    --port -P         - Port to bind to.\n\
    --max-clients -M  - Max clients allowed to connect (0 = no limit).\n\
    --nagle -n        - Enable/disable nagle TCP option.\n\
    --timeout -t      - Timeout (usecs) for connections.\n\
    --version -V      - Print the version string.\n\
",
          program_name);
  if (tst_err)
    exit (EXIT_FAILURE);
  else
    exit (EXIT_SUCCESS);
}


static void serv_cmd_line(int argc, char *argv[])
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
   {"debug", required_argument, NULL, 'd'},
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
                                long_options, NULL)) != EOF)
  {
    switch (optchar)
    {
      case '?':
        fprintf(stderr, " That option is not valid.\n");
      case 'h':
        usage(program_name, 'h' != optchar);
        
      case 'V':
        printf(" %s version 0.7.1.\n", program_name);
        exit (EXIT_SUCCESS);

      case 't': client_timeout      = atoi(optarg); break;
      case 'H': server_ipv4_address = optarg;       break;
      case 'M': server_max_clients  = atoi(optarg);  break;
      case 'P': server_port         = atoi(optarg); break;

      case 'd': vlg_debug(vlg);                     break;
        
      case 'n': OPT_TOGGLE_ARG(evnt_opt_nagle); break;
        
        OPT_SC_SERV_OPTS();
        
      default:
        abort();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 0)
    usage(program_name, TRUE);

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
  
  if (chroot_dir) /* so syslog can log in localtime */
  {
    time_t now = time(NULL);
    (void)localtime(&now);
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

  /*  if (make_dumpable && (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1))
   *    vlg_warn(vlg, "prctl(SET_DUMPABLE, TRUE): %m\n"); */
}

static void serv_timer_con(int type, void *data)
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
    errno = ENOMEM, vlg_warn(vlg, "%s: %m\n", __func__);
    evnt_close(con->ev);
  }
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

  cl_timeout_base = timer_q_add_base(serv_timer_con, TIMER_Q_FLAG_BASE_DEFAULT);
  if (!cl_timeout_base)
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", __func__));
}

static void serv_cntl_resources(void)
{
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BASE, 0, 20);
  
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BUF,
                 0, (CONF_MEM_PREALLOC_MAX / CONF_BUF_SZ));
}

static void serv_signals(void)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    vlg_err(vlg, EXIT_FAILURE, "%s: %m\n", __func__);
  
  /* don't use SA_RESTART ... */
  sa.sa_flags = 0;
  /* ignore it... we don't have a use for it */
  sa.sa_handler = SIG_IGN;
  
  if (sigaction(SIGPIPE, &sa, NULL) == -1)
    vlg_err(vlg, EXIT_FAILURE, "%s: %m\n", __func__);
}

int main(int argc, char *argv[])
{  
  serv_init();
  
  serv_signals();
  
  serv_cmd_line(argc, argv);
  
  vlg_info(vlg, "READY [$<sa:%p>]\n", acpt_evnt->sa);
  
  while (evnt_waiting())
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
  
  vlg_free(vlg);

  vlg_exit();

  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
         
