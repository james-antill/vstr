
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

/* #define NDEBUG 1 -- done via. ./configure */
#include <assert.h>
#define ASSERT assert

#include <err.h>


#include <socket_poll.h>
#include <timer_q.h>

#define TRUE 1
#define FALSE 0


#define CL_PACKET_MAX 0xFFFF
#define CL_MAX_CONNECT 128

/* is the data is less than this, queue instead of hitting write */
#define CL_MAX_WAIT_SEND 16

#include "evnt.h"

struct con
{
 struct Evnt ev[1];
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

static int serv_recv(struct con *con)
{
  int ret = evnt_recv(con->ev);

  if (con->ev->io_r->len)
    vstr_mov(con->ev->io_w, con->ev->io_w->len,
             con->ev->io_r, 1, con->ev->io_r->len);
  
  evnt_send_add(con->ev, FALSE, CL_MAX_WAIT_SEND); /* does write */

  if (con->ev->io_w->len)
    return (TRUE);
  
  if (!ret)
    evnt_close(con->ev);
    
  return (ret);
}

static int cl_cb_func_recv(struct Evnt *evnt)
{
  return (serv_recv((struct con *)evnt));
}

static void cl_cb_func_free(struct Evnt *evnt)
{
  struct con *con = (struct con *)evnt;

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

  /* simple logger */
  printf("CONNECT @[%s] from[%s]\n",
         serv_date_rfc1123(time(NULL)),
         inet_ntoa(((struct sockaddr_in *)sa)->sin_addr));
  fflush(NULL);
  
  con->ev->cbs->cb_func_recv = cl_cb_func_recv;
  con->ev->cbs->cb_func_free = cl_cb_func_free;
  
  ++server_clients_count;
  
  return (con->ev);

 timer_add_fail:
  errno = ENOMEM, warn(__func__);
  evnt_free(con->ev);
 make_acpt_fail:
  free(con);
  return (NULL);
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
          " --debug -d        - Enable/disable debug info.\n"
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
  const char *program_name = "echod";
  struct option long_options[] =
  {
   {"help", no_argument, NULL, 'h'},
   {"debug", required_argument, NULL, 'd'},
   {"host", required_argument, NULL, 'H'},
   {"port", required_argument, NULL, 'P'},
   {"nagle", optional_argument, NULL, 'n'},
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

  while ((optchar = getopt_long(argc, argv, "d:hH:nP:t:V",
                                long_options, NULL)) != EOF)
  {
    switch (optchar)
    {
      case '?':
        fprintf(stderr, " That option is not valid.\n");
      case 'h':
        usage(program_name, '?' == optchar);
        
      case 'V':
        printf(" %s version 0.1.1, compiled on %s.\n",
               program_name,
               __DATE__);
        printf(" %s compiled on %s.\n", program_name, __DATE__);
        exit (EXIT_SUCCESS);

      case 'd': cl_dbg_opt          = atoi(optarg); break;
      case 't': client_timeout      = atoi(optarg); break;
      case 'H': server_ipv4_address = optarg;       break;
      case 'P': server_port         = atoi(optarg); break;

      case 'n':
        if (!optarg)
        { evnt_opt_nagle = !evnt_opt_nagle; }
        else if (!strcasecmp("true", optarg))   evnt_opt_nagle = TRUE;
        else if (!strcasecmp("1", optarg))      evnt_opt_nagle = TRUE;
        else if (!strcasecmp("false", optarg))  evnt_opt_nagle = FALSE;
        else if (!strcasecmp("0", optarg))      evnt_opt_nagle = FALSE;
        break;

      default:
        abort();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 1)
    usage(program_name, TRUE);
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

static void cl_beg(void)
{
  cl_timeout_base = timer_q_add_base(cl_timer_con, TIMER_Q_FLAG_BASE_DEFAULT);

  if (!cl_timeout_base)
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  if (!cl_make_bind(server_ipv4_address, server_port))
    err(EXIT_FAILURE, __func__);
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

int main(int argc, char *argv[])
{
  if (!vstr_init())
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_sc_fmt_add_all(NULL);
  
  if (!(cl_dbg_log = vstr_make_base(NULL)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  
  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  srand(getpid() ^ time(NULL)); /* doesn't need to be secure... just different
                                 * for different runs */
  
  cl_signals();
  
  cl_cmd_line(argc, argv);

  cl_beg();
  
  while (acpt_sock || server_clients_count)
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
  
  vstr_free_base(cl_dbg_log);

  evnt_close_all();
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
         
