/* threaded dns stub resolver, don't use ... just here for comparon to ex_dns.c
 */

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

#include <err.h>


#include <socket_poll.h>
#include <timer_q.h>

#define TRUE 1
#define FALSE 0


#define WAIT_SET_RECV_FLAG 1 /* work around ? */

#define CL_PACKET_MAX 0xFFFF
#define CL_MAX_CONNECT 128

/* made up ... 8bits */
#define CL_MSG_CLIENT_NONE INT_MAX
#define CL_MSG_CLIENT_ZERO (INT_MAX - 1)
#define CL_MAX_WAIT_SEND 16


#include "dns.h"

struct con
{
 struct con *next;
 struct con *prev;
 
 unsigned int ind; /* socket poll */
 
 Vstr_base *io_r;
 Vstr_base *io_w;

 Timer_q_node *tm_o;

 struct con *s_next;
 
 unsigned int flag_q_send_poll : 1;
 unsigned int flag_q_send_now  : 1;
 unsigned int flag_q_none   : 1;
 
 /* info only needed in the client ... */
 struct timeval ctime;
 struct timeval mtime;
 
 unsigned int req_put;
 unsigned int req_got;
};

static int io_r_fd = STDIN_FILENO;
unsigned int io_ind_r = 0; /* socket poll */
static Vstr_base *io_r = NULL;
static int io_w_fd = STDOUT_FILENO;
unsigned int io_ind_w = 0; /* socket poll */
static Vstr_base *io_w = NULL;

static Timer_q_base *cl_timeout_base = NULL;
static Timer_q_base *cl_timer_connect_base = NULL;

static struct con *q_send_now = NULL;

static struct con *q_none      = NULL; /* nothing */
static struct con *q_accept    = NULL; /* connections */
static struct con *q_recv      = NULL; /* recv */
static struct con *q_send_poll = NULL; /* recv + send */

static int server_clients_count = 0; /* current number of clients */

static int server_clients = 1;
static unsigned int server_timeout = (2 * 60); /* 2 minutes */

static const char *server_ipv4_address = "127.0.0.1";
static short server_port = 53;

static int cl_opt_nagle = TRUE;

static void dbg(const char *fmt, ... )
   VSTR__COMPILE_ATTR_FMT(1, 2);
static void dbg(const char *fmt, ... )
{
  Vstr_base *dlog = dns_dbg_log;
  va_list ap;

  if (!dns_dbg_opt)
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

static int app_fmt(Vstr_base *s1, const char *fmt, ... )
   VSTR__COMPILE_ATTR_FMT(2, 3);
static inline int app_fmt(Vstr_base *s1, const char *fmt, ... )
{
  va_list ap;
  int ret = FALSE;
  
  va_start(ap, fmt);
  ret = vstr_add_vfmt(s1, s1->len, fmt, ap);
  va_end(ap);

  return (ret);
}
static inline int app_buf(Vstr_base *s1, const void *buf, size_t len)
{ return (vstr_add_buf(s1, s1->len, buf, len)); }
static inline int app_cstr_buf(Vstr_base *s1, const char *buf)
{ return (vstr_add_cstr_buf(s1, s1->len, buf)); }
static inline int app_i_be4(Vstr_base *s1, unsigned int data)
{
  unsigned char buf[4];

  buf[3] = data & 0xFF; data >>= 8;
  buf[2] = data & 0xFF; data >>= 8;
  buf[1] = data & 0xFF; data >>= 8;
  buf[0] = data & 0xFF;
  
  return (app_buf(s1, buf, 4));
}
static inline int app_i_be2(Vstr_base *s1, unsigned int data)
{
  unsigned char buf[2];

  buf[1] = data & 0xFF; data >>= 8;
  buf[0] = data & 0xFF;
  
  return (app_buf(s1, buf, 2));
}
static inline int app_i_be1(Vstr_base *s1, unsigned int data)
{
  unsigned char buf[1];

  buf[0] = data & 0xFF;
  
  return (app_buf(s1, buf, 1));
}

static inline int sub_i_be2(Vstr_base *s1, size_t pos, unsigned int data)
{
  unsigned char buf[2];

  buf[1] = data & 0xFF; data >>= 8;
  buf[0] = data & 0xFF;
  
  return (vstr_sub_buf(s1, pos, 2, buf, 2));
}
static inline unsigned int get_i_be4(Vstr_base *s1, size_t pos)
{
  unsigned char buf[4];
  unsigned int num = 0;

  vstr_export_buf(s1, pos, 4, buf, sizeof(buf));

  num += buf[0]; num <<= 8;
  num += buf[1]; num <<= 8;
  num += buf[2]; num <<= 8;
  num += buf[3];
  
  return (num);
}
static inline unsigned int get_i_be2(Vstr_base *s1, size_t pos)
{
  unsigned char buf[2];
  unsigned int num = 0;
  
  vstr_export_buf(s1, pos, 2, buf, sizeof(buf));

  num += buf[0]; num <<= 8;
  num += buf[1];
  
  return (num);
}

static void cl_add(struct con **que, struct con *node)
{
  assert(node != *que);
  
  if ((node->next = *que))
    node->next->prev = node;
  
  node->prev = NULL;
  *que = node;
}

static void cl_del(struct con **que, struct con *node)
{
  if (node->prev)
    node->prev->next = node->next;
  else
  {
    assert(*que == node);
    *que = node->next;
  }

  if (node->next)
    node->next->prev = node->prev;
}

static void cl_put_pkt(struct con *con)
{
  assert(con->req_put >= con->req_got);
  ++con->req_put;
  
  if (con->flag_q_none)
  {
    cl_del(&q_none, con), con->flag_q_none = FALSE;
    cl_add(&q_recv, con);
  }  
}

static void cl_got_pkt(struct con *con)
{
  ++con->req_got;
  assert(con->req_put >= con->req_got);
  
  if (!con->flag_q_send_poll && (con->req_put == con->req_got))
  {
    cl_del(&q_recv, con);
    cl_add(&q_none, con), con->flag_q_none = TRUE;
  }
}

static void cl_send_add(struct con *con, int poll_event)
{
  if (!con->flag_q_send_poll && (con->io_w->len > CL_MAX_WAIT_SEND))
  {
    if (!vstr_sc_write_fd(con->io_w, 1, con->io_w->len,
                          SOCKET_POLL_INDICATOR(con->ind)->fd, NULL) &&
        (errno != EAGAIN))
    { /* ignore write errorrs atm. */ ; }
  }
  
  if (con->flag_q_send_now || (con->flag_q_send_poll && !poll_event))
    return; /* already on send_q -- or already polling */
  
  con->s_next = q_send_now;
  q_send_now = con;
  con->flag_q_send_now = TRUE;
}

/* if a connection is on the send now q, then remove them ... this is only
 * done when the client gets killed, so it doesn't matter it's slow */
static void cl_send_del(struct con *con)
{
  struct con **scan = &q_send_now;

  while (*scan && (*scan != con))
    scan = &(*scan)->s_next;

  if (*scan)
    *scan = con->s_next;
}

static void io_fd_set_nonblock(int fd, int val)
{
  int flags = 0;

  assert(val);
  
  if ((flags = fcntl(fd, F_GETFL)) == -1)
    err(EXIT_FAILURE, __func__);
  
  if (!(flags & O_NONBLOCK) &&
      (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1))
    err(EXIT_FAILURE, __func__);
}

static struct con *cl_make(const char *ipv4_string, short port)
{
  int fd = -1;
  struct con *ret = NULL;
  struct sockaddr_in sa;

  ++server_clients_count;
  
  if (!(ret = malloc(sizeof(struct con))))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  ret->tm_o = NULL;
  ret->flag_q_send_poll = FALSE;
  ret->flag_q_send_now  = FALSE;
  ret->flag_q_none   = FALSE;

  ret->req_put = 0;
  ret->req_got = 0;
  
  if (!(ret->io_r = vstr_make_base(NULL)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  if (!(ret->io_w = vstr_make_base(NULL)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    err(EXIT_FAILURE, __func__);
  //  endprotoent(); /* cleanup */
  
  if (fcntl(fd, F_SETFD, TRUE) == -1)
    err(EXIT_FAILURE, __func__);
  
  io_fd_set_nonblock(fd, TRUE);

  if (!cl_opt_nagle)
  {
    int nodelay = 1;
    int retval = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay,
                            sizeof(nodelay));
    if (retval != 0)
      abort();
  }
  
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = inet_addr(ipv4_string);

  if (!(ret->ind = socket_poll_add(fd)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  gettimeofday(&ret->ctime, NULL);
  gettimeofday(&ret->mtime, NULL);
  if (connect(fd, (struct sockaddr *) &sa, sizeof(sa)) == -1)
  {
    if (errno == EINPROGRESS)
    { /*The connection needs more time....*/
      SOCKET_POLL_INDICATOR(ret->ind)->events = POLLOUT;
      cl_add(&q_accept, ret);
      return (ret);
    }

    err(EXIT_FAILURE, "connect()");
  }

  SOCKET_POLL_INDICATOR(ret->ind)->events = POLLIN;
  ret->flag_q_none   = TRUE;
  cl_add(&q_none, ret);
  
  return (ret);
}

static void cl_free(struct con *con)
{
  cl_send_del(con);
  
  vstr_free_base(con->io_w);
  vstr_free_base(con->io_r);
  
  socket_poll_del(con->ind);

  if (con->tm_o)
    timer_q_quick_del_node(con->tm_o);
  
  free(con);

  --server_clients_count;
}

static int cl_send(struct con *con)
{
  if (con->io_w->len &&
      !vstr_sc_write_fd(con->io_w, 1, con->io_w->len,
                        SOCKET_POLL_INDICATOR(con->ind)->fd, NULL) &&
      (errno != EAGAIN))
    return (FALSE);

  gettimeofday(&con->mtime, NULL);
  
  if (0)
  { /* nothing */ }
  else if ( con->flag_q_send_poll && !con->io_w->len)
  {
    cl_del(&q_send_poll, con);
    assert(con->req_put >= con->req_got);
    if (con->req_put == con->req_got)
      cl_add(&q_none, con), con->flag_q_none = TRUE;
    else
      cl_add(&q_recv, con);
    SOCKET_POLL_INDICATOR(con->ind)->events  &= ~POLLOUT;
    SOCKET_POLL_INDICATOR(con->ind)->revents &= ~POLLOUT;
    con->flag_q_send_poll = FALSE;
  }
  else if (!con->flag_q_send_poll &&  con->io_w->len)
  {
    if (con->flag_q_none)
      cl_del(&q_none, con), con->flag_q_none = FALSE;
    else
      cl_del(&q_recv, con);
    cl_add(&q_send_poll, con);
    SOCKET_POLL_INDICATOR(con->ind)->events |=  POLLOUT;
    con->flag_q_send_poll = TRUE;
  }
  
  return (TRUE);
}

static void ui_out(Vstr_base *pkt)
{
  if (!io_ind_w)
    return;

  dns_sc_ui_out(io_w, pkt);
  SOCKET_POLL_INDICATOR(io_ind_w)->events |=  POLLOUT;
}

static void cl_parse(struct con *con, size_t msg_len)
{
  Vstr_base *pkt = vstr_dup_vstr(con->io_r->conf,
                                 con->io_r, 1, msg_len,
                                 VSTR_TYPE_ADD_BUF_PTR);

  if (!pkt)
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  if (dns_dbg_opt > 1)
  {
    dbg("${rep_chr:%c%zu} recv ${BKMG.u:%u} ${rep_chr:%c%zu}\n",
        '=', 33, msg_len, '=', 33);
    dns_dbg_prnt_pkt(pkt);
    dbg("${rep_chr:%c%zu}\n", '-', 79);
  }
  
  ui_out(pkt);
  vstr_free_base(pkt);
  vstr_del(con->io_r, 1, msg_len);

  cl_got_pkt(con);
}

static int cl_recv(struct con *con)
{
  unsigned int ern;
  int ret = TRUE;
  size_t orig_len = con->io_r->len;
  
  vstr_sc_read_iov_fd(con->io_r, con->io_r->len,
                      SOCKET_POLL_INDICATOR(con->ind)->fd, 4, 32, &ern);
  dbg("RECV(%zu)\n", con->io_r->len - orig_len);
  
  switch (ern)
  {
    case VSTR_TYPE_SC_READ_FD_ERR_NONE:
      gettimeofday(&con->mtime, NULL);
      break;
    case VSTR_TYPE_SC_READ_FD_ERR_MEM:
      err(EXIT_FAILURE, __func__);

    case VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO:
      if (errno == EAGAIN)
        return (TRUE);

      dbg("ERROR-RECV-ERRNO(%d:%m): fd=%d\n", errno,
              SOCKET_POLL_INDICATOR(con->ind)->fd);
      ret = FALSE;
      break;

    case VSTR_TYPE_SC_READ_FD_ERR_EOF:
      dbg("ERROR-RECV-EOF: fd=%d len=%zu\n",
              SOCKET_POLL_INDICATOR(con->ind)->fd, con->io_r->len);
      ret = FALSE;
      break;

    default: /* unknown */
      err(EXIT_FAILURE, "read_iov() = %d", ern);
  }

  /* parse data */
  while (con->io_r->len >= 2)
  {
    unsigned int msg_len = get_i_be2(con->io_r, 1);

    if (msg_len > (con->io_r->len - 2))
    {
      if (msg_len > CL_PACKET_MAX)
      {
        dbg("ERROR-RECV-MAX: fd=%d len=%zu\n",
            SOCKET_POLL_INDICATOR(con->ind)->fd, msg_len);
        return (FALSE);
      }
      
      return (TRUE);
    }

    vstr_del(con->io_r, 1, 2);
    
    cl_parse(con, msg_len);
  }

  if (!ret)
  {
    if (con->flag_q_send_poll)
      cl_del(&q_send_poll, con);
    else
      cl_del(&q_recv, con);
    close(SOCKET_POLL_INDICATOR(con->ind)->fd);
    cl_free(con);
  }
  
  return (ret);
}

static void cl_scan_send_fds(void)
{
  struct con *scan = q_send_now;

  q_send_now = NULL;
  while (scan)
  {
    struct con *scan_s_next = scan->s_next;
    
    if (!cl_send(scan))
    {
      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      dbg("ERROR-SEND fd=%d\n", SOCKET_POLL_INDICATOR(scan->ind)->fd);
      if (scan->flag_q_send_poll)
        cl_del(&q_send_poll, scan);
      else if (scan->flag_q_none)
        cl_del(&q_none, scan);
      else
        cl_del(&q_recv, scan);
      cl_free(scan);
    }
    else
      scan->flag_q_send_now = FALSE;
    
    scan = scan_s_next;
  }
}

static void cl_connect(void)
{
  struct con *con = cl_make(server_ipv4_address, server_port);
  struct timeval tv;

  if (server_timeout)
  {
    gettimeofday(&tv, NULL);
    
    TIMER_Q_TIMEVAL_ADD_SECS(&tv, 0, rand() % server_timeout);
  
    if (!(con->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                       TIMER_Q_FLAG_NODE_DEFAULT)))
      errno = ENOMEM, err(EXIT_FAILURE, __func__);
  }
  
  //  dns_app_recq1_pkt(con, "www.and.org", DNS_CLASS_IN, DNS_TYPE_IN_A);
  //  dns_app_recq1_pkt(con, "and.org", DNS_CLASS_IN, DNS_TYPE_IN_TXT);
  //  dns_app_recq1_pkt(con, "version.bind", DNS_CLASS_CH, DNS_TYPE_CH_TXT);
}

static void cl_app_recq1_pkt(struct con *con,
                             const char *name,
                             unsigned int dns_class,
                             unsigned int dns_type)
{
  dns_app_recq_pkt(con->io_w, 1, name, dns_class, dns_type);
  
  cl_put_pkt(con);
  
  if (con != q_accept)
    cl_send_add(con, FALSE); /* does write */
}


#define UI_CMD(x, c, t)                                                 \
    else if (vstr_cmp_case_bod_cstr_eq(io_r, 1, len, x " ")) do         \
    {                                                                   \
      size_t pos = 1;                                                   \
      size_t tmp = strlen(x " ");                                       \
      const char *name = NULL;                                          \
                                                                        \
      pos += tmp; len -= tmp;                                           \
      tmp = vstr_spn_cstr_chrs_fwd(io_r, pos, len, " ");                \
      pos += tmp; len -= tmp;                                           \
      name = vstr_export_cstr_ptr(io_r, pos, len - 1);                  \
                                                                        \
      cl_app_recq1_pkt(con, name, c, t);                                \
    } while (FALSE)

static void ui_parse(void)
{
  size_t len = 0;
  unsigned int count = 64;
  struct con *con = q_none;
  
  if (!con) con = q_recv;
  if (!con) con = q_send_poll;
  if (!con)
  {
    cl_connect();
    return;
  }

  else
  { /* FIXME: not optimal, only want to change after a certain level */
    struct con *con_min = con;

    while (con)
    {
      if (con_min->io_w->len > con->io_w->len)
        con_min = con;
      con = con->next;
    }
    con = con_min;
  }
  
  while ((len = vstr_srch_chr_fwd(io_r, 1, io_r->len, '\n')) && --count)
  {
    size_t line_len = len;

    if (0) { /* not reached */ }
    UI_CMD("*.*",      DNS_CLASS_ALL, DNS_TYPE_ALL);
    UI_CMD("all.all",  DNS_CLASS_ALL, DNS_TYPE_ALL);

    UI_CMD("in.*",     DNS_CLASS_IN, DNS_TYPE_ALL);
    UI_CMD("in.all",   DNS_CLASS_IN, DNS_TYPE_ALL);
    
    UI_CMD("in.a",     DNS_CLASS_IN, DNS_TYPE_IN_A);
    UI_CMD("in.aaaa",  DNS_CLASS_IN, DNS_TYPE_IN_AAAA);
    UI_CMD("in.cname", DNS_CLASS_IN, DNS_TYPE_IN_CNAME);
    UI_CMD("in.hinfo", DNS_CLASS_IN, DNS_TYPE_IN_HINFO);
    UI_CMD("in.mx",    DNS_CLASS_IN, DNS_TYPE_IN_MX);
    UI_CMD("in.ns",    DNS_CLASS_IN, DNS_TYPE_IN_NS);
    UI_CMD("in.ptr",   DNS_CLASS_IN, DNS_TYPE_IN_PTR);
    UI_CMD("in.soa",   DNS_CLASS_IN, DNS_TYPE_IN_SOA);
    UI_CMD("in.txt",   DNS_CLASS_IN, DNS_TYPE_IN_TXT);
    
    UI_CMD("ch.*",     DNS_CLASS_CH, DNS_TYPE_ALL);
    UI_CMD("ch.all",   DNS_CLASS_CH, DNS_TYPE_ALL);
    UI_CMD("ch.txt",   DNS_CLASS_CH, DNS_TYPE_CH_TXT);

    /* ignore everything else */
    
    vstr_del(io_r, 1, line_len);
  }
}
#undef UI_CMD

static void cl_scan_fds(unsigned int ready)
{
  const int bad_poll_flags = (POLLERR | POLLHUP | POLLNVAL);
  struct con *scan = NULL;

  dbg("BEG ready = %u\n", ready);
  if (io_ind_r &&
      SOCKET_POLL_INDICATOR(io_ind_r)->revents & bad_poll_flags)
  {
    --ready;
    
    close(SOCKET_POLL_INDICATOR(io_ind_r)->fd);
    dbg("ERROR-POLL-IO_R(%d):\n",
        SOCKET_POLL_INDICATOR(io_ind_r)->revents);
    socket_poll_del(io_ind_r);
    io_ind_r = 0;
  }
  if (SOCKET_POLL_INDICATOR(io_ind_w)->revents & bad_poll_flags)
  {
    --ready;
    
    close(SOCKET_POLL_INDICATOR(io_ind_w)->fd);
    dbg("ERROR-POLL-IO_W(%d):\n",
        SOCKET_POLL_INDICATOR(io_ind_w)->revents);
    socket_poll_del(io_ind_w);
    io_ind_w = 0;
    return;
  }
  if (io_ind_r && (SOCKET_POLL_INDICATOR(io_ind_r)->revents & POLLIN))
  {
    unsigned int ern;
    
    --ready;
    while (vstr_sc_read_iov_fd(io_r, io_r->len, io_r_fd, 4, 32, &ern))
    { /* do nothing */ }
    switch (ern)
    {
      case VSTR_TYPE_SC_READ_FD_ERR_EOF:
        close(SOCKET_POLL_INDICATOR(io_ind_r)->fd);        
        SOCKET_POLL_INDICATOR(io_ind_r)->fd = -1;        
        socket_poll_del(io_ind_r);
        io_ind_r = 0;
        errno = EAGAIN;
      case VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO:
        if (errno != EAGAIN)
          break;
      case VSTR_TYPE_SC_READ_FD_ERR_NONE:
        ui_parse();
      default:
        break;
    }
    dbg("READ UI\n");
  }
  else if (io_r->len)
    ui_parse();
  
  if (io_ind_w && (SOCKET_POLL_INDICATOR(io_ind_w)->revents & POLLOUT))
  {
    unsigned int ern;
    
    --ready;
    
    while (io_w->len && vstr_sc_write_fd(io_w, 1, io_w->len, io_w_fd, &ern))
    { /* do nothing */ }
    if (!io_w->len)
      SOCKET_POLL_INDICATOR(io_ind_w)->events &= ~POLLOUT;
    dbg("WRITE UI\n");
  }
  
  scan = q_accept;
  while (scan && ready)
  {
    struct con *scan_next = scan->next;
    int done = FALSE;

    /* done as one so we get error code */
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & (POLLOUT|bad_poll_flags))
    {
      int ern = 0;
      socklen_t len = sizeof(int);
      int ret = 0;
      
      done = TRUE;
      
      dbg("ACCEPT %p %d\n", scan, SOCKET_POLL_INDICATOR(scan->ind)->fd);
      ret = getsockopt(SOCKET_POLL_INDICATOR(scan->ind)->fd,
                       SOL_SOCKET, SO_ERROR, &ern, &len);
      if (ret == -1)
        err(EXIT_FAILURE, "getsockopt(SO_ERROR)");
      else if (ern)
      {
        if (ern == ECONNREFUSED)
          errx(EXIT_FAILURE,
               "connection refused to ipv4: %s", server_ipv4_address);
          
        close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
        dbg("ERROR-CONNECT(%d): fd=%2d msg=%s\n", ern,
            SOCKET_POLL_INDICATOR(scan->ind)->fd, strerror(ern));
        cl_del(&q_accept, scan);
        cl_free(scan);
        goto next_accept;
      }
      else
      {
        SOCKET_POLL_INDICATOR(scan->ind)->events = POLLIN;
        SOCKET_POLL_INDICATOR(scan->ind)->revents = 0;
        
        cl_del(&q_accept, scan);
        scan->flag_q_none = TRUE;
        cl_add(&q_none, scan);
        //        cl_send_add(scan, FALSE);
        
        ui_parse();
      }
    }
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
      dbg("ERROR-ACCPT-EVNT(%d): fd=%d\n",
          SOCKET_POLL_INDICATOR(scan->ind)->revents,
          SOCKET_POLL_INDICATOR(scan->ind)->fd);

   next_accept:
    if (done)
      --ready;

    scan = scan_next;
  }
  dbg("POST accept ready = %u\n", ready);

  scan = q_send_poll;
  while (scan && ready)
  {
    struct con *scan_next = scan->next;
    int done = FALSE;
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & bad_poll_flags)
    {
      done = TRUE;
      
      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      dbg("ERROR-POLL-SQ(%d): fd=%d\n",
          SOCKET_POLL_INDICATOR(scan->ind)->revents,
          SOCKET_POLL_INDICATOR(scan->ind)->fd);
      cl_del(&q_send_poll, scan);
      cl_free(scan);
      goto next_send;
    }

    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
    {
      dbg("send/RECV %p %d %u %u\n",
          scan, SOCKET_POLL_INDICATOR(scan->ind)->fd,
          scan->req_put, scan->req_got);
      done = TRUE;
      if (!cl_recv(scan))
        goto next_send;
    }
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT)
    {
      dbg("SEND/recv %p %d %u %u\n",
          scan, SOCKET_POLL_INDICATOR(scan->ind)->fd,
          scan->req_put, scan->req_got);
      done = TRUE;
      cl_send_add(scan, TRUE);
    }

   next_send:
    if (done)
      --ready;

    scan = scan_next;
  }
  dbg("POST send ready = %u\n", ready);

  scan = q_recv;
  while (scan && ready)
  {
    struct con *scan_next = scan->next;
    int done = FALSE;
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & bad_poll_flags)
    {
      done = TRUE;

      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      dbg("ERROR-POLL-RQ(%d): fd=%d\n",
          SOCKET_POLL_INDICATOR(scan->ind)->revents,
          SOCKET_POLL_INDICATOR(scan->ind)->fd);
      cl_del(&q_recv, scan);
      cl_free(scan);
      goto next_recv;
    }
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
    {
      dbg("     RECV %p %d %u %u\n", scan, SOCKET_POLL_INDICATOR(scan->ind)->fd,
          scan->req_put, scan->req_got);
      done = TRUE;
      if (!cl_recv(scan))
        goto next_recv;
    }
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT)
      dbg("ERROR-RECV-EVNT(%d): fd=%d\n",
          SOCKET_POLL_INDICATOR(scan->ind)->revents,
          SOCKET_POLL_INDICATOR(scan->ind)->fd);

   next_recv:
    if (done)
      --ready;

    scan = scan_next;
  }
  dbg("POST recv ready = %u\n", ready);
  
  scan = q_none;
  while (scan && ready)
  {
    struct con *scan_next = scan->next;
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & (bad_poll_flags | POLLIN))
    {
      /* POLLIN == EOF */
      --ready;

      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      dbg("ERROR-POLL-NQ(%d): fd=%d\n",
          SOCKET_POLL_INDICATOR(scan->ind)->revents,
          SOCKET_POLL_INDICATOR(scan->ind)->fd);
      cl_del(&q_none, scan);
      cl_free(scan);
    }
    else if (SOCKET_POLL_INDICATOR(scan->ind)->revents)
      dbg("ERROR-NONE-EVNT(%d): fd=%d\n",
          SOCKET_POLL_INDICATOR(scan->ind)->revents,
          SOCKET_POLL_INDICATOR(scan->ind)->fd);

    scan = scan_next;
  }
  dbg("POST none ready = %u\n", ready);
  
  dbg("END ready = %u\n", ready);
  assert(!ready);
}

static int cl_poll(void)
{
  const struct timeval *tv = timer_q_first_timeval();
  int msecs = -1;
  
  if (tv)
  {
    long diff = 0;
    struct timeval now_timeval;
    
    gettimeofday(&now_timeval, NULL);
    
    diff = timer_q_timeval_diff_msecs(tv, &now_timeval);
    
    if (diff > 0)
    {
      if (diff >= INT_MAX)
        msecs = INT_MAX - 1;
      else
        msecs = diff;
    }
    else
      msecs = 0;
  }

  return (socket_poll_update_all(msecs));
}

static void usage(const char *program_name, int tst_err)
{
  fprintf(tst_err ? stderr : stdout, "\n Format: %s [-chmtwV] <?>\n"
          " --help -h         - Print this message.\n"
          " --debug -d        - Enable/disable debug info.\n"
          " --output -o       - Output debug to filename instead of stderr.\n"
          " --clients -c      - Number of connections to make.\n"
          " --host -H         - IPv4 host address to send DNS queries to.\n"
          " --port -P         - Port to send DNS queries to.\n"
          " --nagle -n        - Enable/disable nagle TCP option.\n"
          " --timeout -t      - Timeout (usecs) between each message.\n"
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
  const char *program_name = "dns";
  struct option long_options[] =
  {
   {"help", no_argument, NULL, 'h'},
   {"clients", required_argument, NULL, 'c'},
   {"debug", required_argument, NULL, 'd'},
   {"execute", required_argument, NULL, 'e'},
   {"host", required_argument, NULL, 'H'},
   {"port", required_argument, NULL, 'P'},
   {"nagle", optional_argument, NULL, 'n'},
   {"output", required_argument, NULL, 'o'},
   {"recursive", optional_argument, NULL, 'R'},
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

  while ((optchar = getopt_long(argc, argv, "c:d:e:hH:m:no:P:Rt:TVw:",
                                long_options, NULL)) != EOF)
  {
    switch (optchar)
    {
      case '?':
        fprintf(stderr, " That option is not valid.\n");
      case 'h':
        usage(program_name, '?' == optchar);
        
      case 'V':
        printf(" %s version 0.0.1, compiled on %s.\n",
               program_name,
               __DATE__);
        printf(" %s compiled on %s.\n", program_name, __DATE__);
        exit (EXIT_SUCCESS);

      case 'c': server_clients      = atoi(optarg); break;
      case 'd': dns_dbg_opt         = atoi(optarg); break;
      case 't': server_timeout      = atoi(optarg); break;
      case 'H': server_ipv4_address = optarg;       break;
      case 'P': server_port         = atoi(optarg); break;

      case 'e':
        /* use cmd line instead of stdin */
        io_r_fd = -1;
        app_cstr_buf(io_r, optarg); app_cstr_buf(io_r, "\n");
        break;
        
      case 'o':
        dns_dbg_fd = open(optarg, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (dns_dbg_fd == -1)
          err(EXIT_FAILURE, "open(%s)", optarg);
        break;

      case 'n':
        if (!optarg)
        { cl_opt_nagle = !cl_opt_nagle; }
        else if (!strcasecmp("true", optarg))   cl_opt_nagle = TRUE;
        else if (!strcasecmp("1", optarg))      cl_opt_nagle = TRUE;
        else if (!strcasecmp("false", optarg))  cl_opt_nagle = FALSE;
        else if (!strcasecmp("0", optarg))      cl_opt_nagle = FALSE;
        break;

      case 'R':
        if (!optarg)
        { dns_opt_recur = !dns_opt_recur; }
        else if (!strcasecmp("true", optarg))   dns_opt_recur = TRUE;
        else if (!strcasecmp("1", optarg))      dns_opt_recur = TRUE;
        else if (!strcasecmp("false", optarg))  dns_opt_recur = FALSE;
        else if (!strcasecmp("0", optarg))      dns_opt_recur = FALSE;
        break;

      default:
        abort();
    }
  }

  argc -= optind;
  argv += optind;

  //  if (argc != 1)
  //    usage(program_name, TRUE);

  //  ip_addr_input_file = argv[0];
}

static void cl_timer_cli(int type, void *data)
{
  struct con *con = NULL;
  struct timeval tv;
  unsigned long diff = 0;
  
  if (type == TIMER_Q_TYPE_CALL_DEL)
    return;

  con = data;

  gettimeofday(&tv, NULL);
  diff = timer_q_timeval_udiff_secs(&tv, &con->mtime);
  if (diff > server_timeout)
  {
    dbg("timeout = %p (%lu, %lu)\n", con, diff, (unsigned long)server_timeout);
    close(SOCKET_POLL_INDICATOR(con->ind)->fd);
    return;
  }
  
  if (type == TIMER_Q_TYPE_CALL_RUN_ALL)
    return;
  
  TIMER_Q_TIMEVAL_ADD_SECS(&tv, (server_timeout - diff) + 1, 0);
  if (!(con->tm_o = timer_q_add_node(cl_timeout_base, con, &tv,
                                     TIMER_Q_FLAG_NODE_DEFAULT)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
}

static void cl_timer_con(int type, void *data)
{
  int count = 0;

  (void)data; /* currently unused */
  
  if (type == TIMER_Q_TYPE_CALL_DEL)
    return;
  
  while ((server_clients_count < server_clients) && (count < CL_MAX_CONNECT))
  {
    cl_connect();
    ++count;
  }
  
  if (type == TIMER_Q_TYPE_CALL_RUN_ALL)
    return;
  
  if (server_clients_count < server_clients)
  {
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    TIMER_Q_TIMEVAL_ADD_SECS(&tv, 1, 0);
    if (!timer_q_add_node(cl_timer_connect_base, NULL, &tv,
                          TIMER_Q_FLAG_NODE_DEFAULT))
      errno = ENOMEM, err(EXIT_FAILURE, __func__);
  }
}

static void cl_beg(void)
{
  int count = 0;
  
  cl_timeout_base       = timer_q_add_base(cl_timer_cli,
                                           TIMER_Q_FLAG_BASE_DEFAULT);
  cl_timer_connect_base = timer_q_add_base(cl_timer_con,
                                           TIMER_Q_FLAG_BASE_DEFAULT);
  if (!cl_timeout_base)
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  if (!cl_timer_connect_base)
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  if (io_r_fd != -1)
  {
    io_fd_set_nonblock(io_r_fd,  TRUE);
    if (!(io_ind_r = socket_poll_add(io_r_fd)))
      errno = ENOMEM, err(EXIT_FAILURE, __func__);
    SOCKET_POLL_INDICATOR(io_ind_r)->events |= POLLIN;
  }
  
  io_fd_set_nonblock(io_w_fd, TRUE);
  if (!(io_ind_w = socket_poll_add(io_w_fd)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  while ((server_clients_count < server_clients) && (count < CL_MAX_CONNECT))
  {
    cl_connect();
    ++count;
  }
  
  if (server_clients_count < server_clients)
  {
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    TIMER_Q_TIMEVAL_ADD_SECS(&tv, 1, 0);
    if (!timer_q_add_node(cl_timer_connect_base, NULL, &tv,
                          TIMER_Q_FLAG_NODE_DEFAULT))
      errno = ENOMEM, err(EXIT_FAILURE, __func__);
  }
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
  
  if (!(dns_dbg_log = vstr_make_base(NULL)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  
  if (!(io_r = vstr_make_base(NULL))) /* used in cmd line */
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  if (!(io_w = vstr_make_base(NULL)))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);
  
  if (!socket_poll_init(0, SOCKET_POLL_TYPE_MAP_DIRECT))
    errno = ENOMEM, err(EXIT_FAILURE, __func__);

  srand(getpid() ^ time(NULL)); /* doesn't need to be secure... just different
                                 * for different runs */
  
  cl_signals();
  
  cl_cmd_line(argc, argv);

  cl_beg();
  
  while (io_ind_w && (io_w->len ||
                      ((q_accept || q_recv || q_send_poll) ||
                       io_ind_r || io_r->len)))
  {
    int ready = cl_poll();
    struct timeval tv;
    
    if ((ready == -1) && (errno != EINTR))
      err(EXIT_FAILURE, __func__);

    dbg("1 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_poll, q_none);
    cl_scan_fds(ready);
    dbg("2 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_poll, q_none);
    cl_scan_send_fds();
    dbg("3 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_poll, q_none);
    
    gettimeofday(&tv, NULL);
    timer_q_run_norm(&tv);
    dbg("4 a=%p r=%p s=%p n=%p\n", q_accept, q_recv, q_send_poll, q_none);
    cl_scan_send_fds();
  }

  timer_q_del_base(cl_timeout_base);
  timer_q_del_base(cl_timer_connect_base);
  
  vstr_free_base(dns_dbg_log);
  vstr_free_base(io_r);
  vstr_free_base(io_w);

  {
    struct con *scan = NULL;

    q_send_now = NULL;
    
    scan = q_accept; q_accept = NULL;
    while (scan)
    {
      struct con *scan_next = scan->next;

      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      cl_free(scan);
      
      scan = scan_next;
    }
    
    scan = q_recv; q_recv = NULL;
    while (scan)
    {
      struct con *scan_next = scan->next;

      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      cl_free(scan);
      
      scan = scan_next;
    }
    
    scan = q_send_poll; q_send_poll = NULL;
    while (scan)
    {
      struct con *scan_next = scan->next;

      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      cl_free(scan);
      
      scan = scan_next;
    }

    scan = q_none; q_none = NULL;
    while (scan)
    {
      struct con *scan_next = scan->next;

      close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
      cl_free(scan);
      
      scan = scan_next;
    }
  }
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
         
