
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>

/* #define NDEBUG 1 -- done via. ./configure */
#include <assert.h>
#define ASSERT assert

#include <err.h>

#include <socket_poll.h>
#include <timer_q.h>

#include "date.h"

#include "evnt.h"

#define TRUE 1
#define FALSE 0


#define CL_PACKET_MAX 0xFFFF
#define CL_MAX_CONNECT 128

/* is the data is less than this, queue instead of hitting write */
#define CL_MAX_WAIT_SEND 16

struct con
{
 struct Evnt ev[1];
 struct sockaddr *sa;
};

struct con_listen
{
 struct Evnt ev[1];
};

static Timer_q_base *cl_timeout_base = NULL;

static struct con_listen *acpt_sock = NULL;

static int server_clients_count = 0; /* current number of clients */

static unsigned int client_timeout = (2 * 60); /* 2 minutes */

static const char *server_ipv4_address = NULL;
static short server_port = 7;

static Vlg *vlg = NULL;

static int serv_recv(struct con *con)
{
  unsigned int ern = 0;
  int ret = evnt_recv(con->ev, &ern);

  if (con->ev->io_r->len)
    vstr_mov(con->ev->io_w, con->ev->io_w->len,
             con->ev->io_r, 1, con->ev->io_r->len);
  
  evnt_send_add(con->ev, FALSE, CL_MAX_WAIT_SEND); /* does write */

  if (!ret)
  { /* untested -- getting EOF while still doing an old request */
    if (ern == VSTR_TYPE_SC_READ_FD_ERR_EOF)
    {
      SOCKET_POLL_INDICATOR(con->ev->ind)->events &= ~POLLIN;
      
      if (con->ev->io_w->len)
        return (TRUE);
    }    

    evnt_close(con->ev);
  }
  
  return (ret);
}

static int cl_cb_func_recv(struct Evnt *evnt)
{
  return (serv_recv((struct con *)evnt));
}

static void cl_cb_func_free(struct Evnt *evnt)
{
  struct con *con = (struct con *)evnt;

  vlg_info(vlg, "FREE @[%s] from[%s]\n", date_rfc1123(time(NULL)),
           inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr));
  
  free(con->sa);
  free(con);

  --server_clients_count;
}

static void cl_cb_func_acpt_free(struct Evnt *evnt)
{
  struct con_listen *con = (struct con_listen *)evnt;

  free(con);

  ASSERT(acpt_sock == con);
  
  acpt_sock = NULL;
}

static struct Evnt *cl_cb_func_accept(int fd,
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
  
  vlg_info(vlg, "CONNECT @[%s] from[%s]\n", date_rfc1123(time(NULL)),
           inet_ntoa(((struct sockaddr_in *)con->sa)->sin_addr));
  
  con->ev->cbs->cb_func_recv = cl_cb_func_recv;
  con->ev->cbs->cb_func_free = cl_cb_func_free;
  
  ++server_clients_count;
  
  return (con->ev);

 malloc_sa_fail:
  timer_q_quick_del_node(con->ev->tm_o);
 timer_add_fail:
  evnt_free(con->ev);
 make_acpt_fail:
  free(con);
  VLG_WARNNOMEM_RET(NULL, (vlg, __func__));
}

static int cl_make_bind(const char *acpt_addr, short acpt_port)
{
  struct con_listen *con = malloc(sizeof(struct con_listen));

  if (!con || !evnt_make_bind_ipv4(con->ev, acpt_addr, acpt_port))
  {
    free(con);
    return (FALSE);
  }
                                                                              
  SOCKET_POLL_INDICATOR(con->ev->ind)->events |= POLLIN;

  acpt_sock = con;
  
  con->ev->cbs->cb_func_accept = cl_cb_func_accept;
  con->ev->cbs->cb_func_free   = cl_cb_func_acpt_free;

  return (TRUE);
}

static void usage(const char *program_name, int tst_err)
{
  fprintf(tst_err ? stderr : stdout, "\n Format: %s [-dHhnPtV]\n"
          " --daemon          - Become a daemon.\n"
          " --debug -d        - Enable debug info.\n"
          " --host -H         - IPv4 address to bind (def: \"all\").\n"
          " --help -h         - Print this message.\n"
          " --port -P         - Port to bind to.\n"
          " --nagle -n        - Enable/disable nagle TCP option.\n"
          " --timeout -t      - Timeout (usecs) for connections.\n"
          " --version -V      - Print the version string.\n",
          program_name);
  if (tst_err)
    exit (EXIT_FAILURE);
  else
    exit (EXIT_SUCCESS);
}


static void cl_cmd_line(int argc, char *argv[])
{
  char optchar = 0;
  const char *program_name = "jechod";
  struct option long_options[] =
  {
   {"help", no_argument, NULL, 'h'},
   {"daemon", no_argument, NULL, 1},
   {"debug", required_argument, NULL, 'd'},
   {"host", required_argument, NULL, 'H'},
   {"port", required_argument, NULL, 'P'},
   {"nagle", optional_argument, NULL, 'n'},
   {"timeout", required_argument, NULL, 't'},
   {"version", no_argument, NULL, 'V'},
   {NULL, 0, NULL, 0}
  };
  int become_daemon = FALSE;
  
  if (argv[0])
  {
    if ((program_name = strrchr(argv[0], '/')))
      ++program_name;
    else
      program_name = argv[0];
  }

  while ((optchar = getopt_long(argc, argv, "dhH:nP:t:V",
                                long_options, NULL)) != EOF)
  {
    switch (optchar)
    {
      case '?':
        fprintf(stderr, " That option is not valid.\n");
      case 'h':
        usage(program_name, 'h' != optchar);
        
      case 'V':
        printf(" %s version 0.1.1, compiled on %s.\n", program_name, __DATE__);
        exit (EXIT_SUCCESS);

      case 't': client_timeout      = atoi(optarg); break;
      case 'H': server_ipv4_address = optarg;       break;
      case 'P': server_port         = atoi(optarg); break;

      case 'd': vlg_debug(vlg);                     break;
        
      case 'n':
        if (!optarg)
        { evnt_opt_nagle = !evnt_opt_nagle; }
        else if (!strcasecmp("true", optarg))   evnt_opt_nagle = TRUE;
        else if (!strcasecmp("1", optarg))      evnt_opt_nagle = TRUE;
        else if (!strcasecmp("false", optarg))  evnt_opt_nagle = FALSE;
        else if (!strcasecmp("0", optarg))      evnt_opt_nagle = FALSE;
        break;

      case 1:
        become_daemon = TRUE;
        break;
        
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

static void cl_beg(void)
{
  if (!vstr_init())
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_sc_fmt_add_all(NULL);
  
  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  vlg_init();

  if (!(vlg = vlg_make()))
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  evnt_logger(vlg);

  cl_timeout_base = timer_q_add_base(cl_timer_con, TIMER_Q_FLAG_BASE_DEFAULT);
  if (!cl_timeout_base)
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", __func__));
}

static void cl_signals(void)
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
  cl_beg();
  
  cl_signals();
  
  cl_cmd_line(argc, argv);

  if (!cl_make_bind(server_ipv4_address, server_port))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", __func__));

  cl_beg();
  
  vlg_info(vlg, "READY @[%s]!\n", date_rfc1123(time(NULL)));
  
  while (acpt_sock || server_clients_count)
  {
    int ready = evnt_poll();
    struct timeval tv;
    
    if ((ready == -1) && (errno != EINTR))
      vlg_err(vlg, EXIT_FAILURE, "%s: %m\n", "poll");
    if (ready == -1)
      continue;

    vlg_dbg3(vlg, "1 a=%p r=%p s=%p n=%p\n",
             q_accept, q_recv, q_send_recv, q_none);
    evnt_scan_fds(ready, CL_MAX_WAIT_SEND);
    vlg_dbg3(vlg, "2 a=%p r=%p s=%p n=%p\n",
             q_accept, q_recv, q_send_recv, q_none);
    evnt_scan_send_fds();
    vlg_dbg3(vlg, "3 a=%p r=%p s=%p n=%p\n",
             q_accept, q_recv, q_send_recv, q_none);
    
    gettimeofday(&tv, NULL);
    timer_q_run_norm(&tv);

    vlg_dbg3(vlg, "4 a=%p r=%p s=%p n=%p\n",
             q_accept, q_recv, q_send_recv, q_none);
    evnt_scan_send_fds();
  }
  vlg_dbg3(vlg, "E a=%p r=%p s=%p n=%p\n",
           q_accept, q_recv, q_send_recv, q_none);

  timer_q_del_base(cl_timeout_base);
  
  evnt_close_all();
  
  vlg_free(vlg);

  vlg_exit();

  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
         
