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

#include "evnt.h"

#ifdef __GNUC__
# define EVNT__ATTR_UNUSED(x) vstr__UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define EVNT__ATTR_UNUSED(x) /*@unused@*/ vstr__UNUSED_ ## x
#else
# define EVNT__ATTR_UNUSED(x) vstr__UNUSED_ ## x
#endif

int evnt_opt_nagle = TRUE;

struct Evnt *q_send_now = NULL;  /* Try a send "now" */

struct Evnt *q_none      = NULL; /* nothing */
struct Evnt *q_accept    = NULL; /* connections - recv */
struct Evnt *q_connect   = NULL; /* connections - send */
struct Evnt *q_recv      = NULL; /* recv */
struct Evnt *q_send_recv = NULL; /* recv + send */

void evnt_fd_set_nonblock(int fd, int val)
{
  int flags = 0;

  assert(val);
  
  if ((flags = fcntl(fd, F_GETFL)) == -1)
    err(EXIT_FAILURE, __func__);
  
  if (!(flags & O_NONBLOCK) &&
      (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1))
    err(EXIT_FAILURE, __func__);
}

static  int evnt__cb_func_connect(struct Evnt *EVNT__ATTR_UNUSED(evnt))
{
  return (TRUE);
}

static struct Evnt *evnt__cb_func_accept(int fd,
                                         struct sockaddr *EVNT__ATTR_UNUSED(sa),
                                         socklen_t EVNT__ATTR_UNUSED(len))
{
  close(fd);
  return (NULL);
}

static  int evnt__cb_func_recv(struct Evnt *EVNT__ATTR_UNUSED(evnt))
{
  return (TRUE);
}

static  int evnt__cb_func_send(struct Evnt *evnt)
{
  return (evnt_send(evnt));
}

static void evnt__cb_func_free(struct Evnt *EVNT__ATTR_UNUSED(evnt))
{
}

static int evnt_init(struct Evnt *evnt, int fd)
{
  evnt->flag_q_send_recv = FALSE;
  evnt->flag_q_send_now  = FALSE;
  evnt->flag_q_none      = FALSE;
  
  evnt->req_put = 0;
  evnt->req_got = 0;

  evnt->cbs->cb_func_accept  = evnt__cb_func_accept;
  evnt->cbs->cb_func_connect = evnt__cb_func_connect;
  evnt->cbs->cb_func_recv    = evnt__cb_func_recv;
  evnt->cbs->cb_func_send    = evnt__cb_func_send;
  evnt->cbs->cb_func_free    = evnt__cb_func_free;
  
  if (!(evnt->io_r = vstr_make_base(NULL)))
    goto make_vstr_fail;
  
  if (!(evnt->io_w = vstr_make_base(NULL)))
    goto make_vstr_fail;

  if (!(evnt->ind = socket_poll_add(fd)))
    goto poll_add_fail;

  evnt->tm_o = NULL;
  
  gettimeofday(&evnt->ctime, NULL);
  gettimeofday(&evnt->mtime, NULL);
  
  if (fcntl(fd, F_SETFD, TRUE) == -1)
    goto fcntl_fail;
  
  evnt_fd_set_nonblock(fd, TRUE);

  return (TRUE);

 fcntl_fail:
  socket_poll_del(evnt->ind);
 poll_add_fail:
  vstr_free_base(evnt->io_w);
 make_vstr_fail:
  vstr_free_base(evnt->io_r);
  
  return (FALSE);
}

int evnt_make_con_ipv4(struct Evnt *evnt, const char *ipv4_string, short port)
{
  int fd = -1;
  struct sockaddr_in sa;

  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    return (FALSE);
  
  if (!evnt_init(evnt, fd))
    return (FALSE);
  
  if (!evnt_opt_nagle)
  {
    int nodelay = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
  }
  
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = inet_addr(ipv4_string);

  if (connect(fd, (struct sockaddr *) &sa, sizeof(sa)) == -1)
  {
    if (errno == EINPROGRESS)
    { /*The connection needs more time....*/
      SOCKET_POLL_INDICATOR(evnt->ind)->events = POLLOUT;
      evnt_add(&q_connect, evnt);
      return (TRUE);
    }
    
    close(fd);
    
    return (FALSE);
  }

  SOCKET_POLL_INDICATOR(evnt->ind)->events = POLLIN;
  evnt->flag_q_none   = TRUE;
  evnt_add(&q_none, evnt);
  
  return (TRUE);
}

int evnt_make_acpt(struct Evnt *evnt, int fd,
                   struct sockaddr *EVNT__ATTR_UNUSED(sa),
                   socklen_t EVNT__ATTR_UNUSED(len))
{
  int ret = evnt_init(evnt, fd);
  
  if (ret && !evnt_opt_nagle)
  {
    int nodelay = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
  }

  evnt_add(&q_recv, evnt);

  return (ret);
}

int evnt_make_bind_ipv4(struct Evnt *evnt,
                        const char *acpt_addr, short server_port)
{
  int fd = -1;
  struct sockaddr_in socket_bind_in;
  int sock_opt_true = TRUE;

  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    goto sock_fail;
  
  if (!evnt_init(evnt, fd))
    goto init_fail;
  
  memset(&socket_bind_in, 0, sizeof(socket_bind_in));
                                                                              
  socket_bind_in.sin_family = AF_INET;
  if (!acpt_addr)
    socket_bind_in.sin_addr.s_addr = htonl(INADDR_ANY);
  else
    socket_bind_in.sin_addr.s_addr = inet_addr(acpt_addr);
  socket_bind_in.sin_port = htons(server_port);

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                 &sock_opt_true, sizeof(sock_opt_true)) == -1)
    goto reuse_fail;
  
  if (bind(fd, (struct sockaddr *) &socket_bind_in,
           sizeof(struct sockaddr_in)) == -1)
    goto bind_fail;
                                                                              
  if (listen(fd, 512) == -1)
    goto listen_fail;

  evnt_add(&q_accept, evnt);
    
  return (TRUE);

 bind_fail:
  err(EXIT_FAILURE, "bind(%s:%d): %s\n", acpt_addr, server_port,
      strerror(errno));
 listen_fail:
 reuse_fail:
 init_fail:
  evnt_free(evnt);
 sock_fail:
  return (FALSE);
}

void evnt_free(struct Evnt *con)
{
  evnt_send_del(con);
  
  vstr_free_base(con->io_w);
  vstr_free_base(con->io_r);
  
  socket_poll_del(con->ind);

  if (con->tm_o)
    timer_q_quick_del_node(con->tm_o);
}

static void evnt__close(struct Evnt *con)
{
  close(SOCKET_POLL_INDICATOR(con->ind)->fd);
  evnt_free(con);
  con->cbs->cb_func_free(con);
}

void evnt_close(struct Evnt *con)
{
  if (con->flag_q_send_recv)
    evnt_del(&q_send_recv, con);
  else if (con->flag_q_none)
    evnt_del(&q_none, con);
  else
    evnt_del(&q_recv, con);

  evnt__close(con);
}

void evnt_add(struct Evnt **que, struct Evnt *node)
{
  assert(node != *que);
  
  if ((node->next = *que))
    node->next->prev = node;
  
  node->prev = NULL;
  *que = node;
}

void evnt_del(struct Evnt **que, struct Evnt *node)
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

void evnt_put_pkt(struct Evnt *con)
{
  assert(con->req_put >= con->req_got);
  ++con->req_put;
  
  if (con->flag_q_none)
  {
    evnt_del(&q_none, con), con->flag_q_none = FALSE;
    evnt_add(&q_recv, con);
  }  
}

void evnt_got_pkt(struct Evnt *con)
{
  ++con->req_got;
  assert(con->req_put >= con->req_got);
  
  if (!con->flag_q_send_recv && (con->req_put == con->req_got))
  {
    evnt_del(&q_recv, con);
    evnt_add(&q_none, con), con->flag_q_none = TRUE;
  }
}

int evnt_send_add(struct Evnt *con, int force_poll_event, size_t max_sz)
{
  if (!con->flag_q_send_recv && (con->io_w->len > max_sz))
  {
    unsigned int ern = 0;
    
    if (!vstr_sc_write_fd(con->io_w, 1, con->io_w->len,
                          SOCKET_POLL_INDICATOR(con->ind)->fd, &ern) &&
        (errno != EAGAIN))
      return (FALSE);
  }

  /* already on send_q -- or already polling (and not forcing) */
  if (con->flag_q_send_now || (con->flag_q_send_recv && !force_poll_event))
    return (TRUE);
  
  con->s_next = q_send_now;
  q_send_now = con;
  con->flag_q_send_now = TRUE;
  
  return (TRUE);
}

/* if a connection is on the send now q, then remove them ... this is only
 * done when the client gets killed, so it doesn't matter it's slow */
void evnt_send_del(struct Evnt *con)
{
  struct Evnt **scan = &q_send_now;

  if (!con->flag_q_send_now)
    return;
  
  while (*scan && (*scan != con))
  {
    scan = &(*scan)->s_next;

    ASSERT(*scan);
  }
  
  *scan = con->s_next;
}

int evnt_recv(struct Evnt *con)
{
  unsigned int ern;
  int ret = TRUE;
  
  vstr_sc_read_iov_fd(con->io_r, con->io_r->len,
                      SOCKET_POLL_INDICATOR(con->ind)->fd, 4, 32, &ern);
  
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
      ret = FALSE;
      break;

    case VSTR_TYPE_SC_READ_FD_ERR_EOF:
      ret = FALSE;
      break;

    default: /* unknown */
      err(EXIT_FAILURE, "read_iov() = %d", ern);
  }

  return (ret);
}

int evnt_send(struct Evnt *con)
{
  unsigned int ern = 0;
  
  con->flag_q_send_now = FALSE;

  if (con->io_w->len &&
      !vstr_sc_write_fd(con->io_w, 1, con->io_w->len,
                        SOCKET_POLL_INDICATOR(con->ind)->fd, &ern) &&
      (errno != EAGAIN))
    return (FALSE);

  gettimeofday(&con->mtime, NULL);
  
  if (0)
  { /* nothing */ }
  else if ( con->flag_q_send_recv && !con->io_w->len)
  {
    evnt_del(&q_send_recv, con);
    assert(con->req_put >= con->req_got);
    if (con->req_put == con->req_got)
      evnt_add(&q_none, con), con->flag_q_none = TRUE;
    else
      evnt_add(&q_recv, con);
    SOCKET_POLL_INDICATOR(con->ind)->events  &= ~POLLOUT;
    SOCKET_POLL_INDICATOR(con->ind)->revents &= ~POLLOUT;
    con->flag_q_send_recv = FALSE;
  }
  else if (!con->flag_q_send_recv &&  con->io_w->len)
  {
    if (con->flag_q_none)
      evnt_del(&q_none, con), con->flag_q_none = FALSE;
    else
      evnt_del(&q_recv, con);
    evnt_add(&q_send_recv, con);
    SOCKET_POLL_INDICATOR(con->ind)->events |=  POLLOUT;
    con->flag_q_send_recv = TRUE;
  }
  
  return (TRUE);
}

int evnt_poll(void)
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

void evnt_scan_fds(unsigned int ready, size_t max_sz)
{
  const int bad_poll_flags = (POLLERR | POLLHUP | POLLNVAL);
  struct Evnt *scan = NULL;
  
  scan = q_connect;
  while (scan && ready)
  {
    struct Evnt *scan_next = scan->next;
    int done = FALSE;

    /* done as one so we get error code */
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & (POLLOUT|bad_poll_flags))
    {
      int ern = 0;
      socklen_t len = sizeof(int);
      int ret = 0;
      
      done = TRUE;
      
      ret = getsockopt(SOCKET_POLL_INDICATOR(scan->ind)->fd,
                       SOL_SOCKET, SO_ERROR, &ern, &len);
      if (ret == -1)
        err(EXIT_FAILURE, "getsockopt(SO_ERROR)");
      else if (ern)
      {
        /* server_ipv4_address */
        if (ern == ECONNREFUSED)
          errno = ern, err(EXIT_FAILURE, "connect");

        /* FIXME: move del */
        evnt_del(&q_connect, scan);
        evnt_add(&q_none, scan); scan->flag_q_none = TRUE;

        evnt_close(scan);
        goto next_connect;
      }
      else
      {
        SOCKET_POLL_INDICATOR(scan->ind)->events = POLLIN;
        SOCKET_POLL_INDICATOR(scan->ind)->revents = 0;
        
        evnt_del(&q_connect, scan);
        scan->flag_q_none = TRUE;
        evnt_add(&q_none, scan);
        
        if (!scan->cbs->cb_func_connect(scan))
          goto next_connect;
      }
    }
    assert(!(SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN));

   next_connect:
    if (done)
      --ready;

    scan = scan_next;
  }

  scan = q_accept;
  while (scan && ready)
  { /* evidence is that preferring read over accept gives better results
     * -- edge triggering needs to requeue on non failure */
    struct Evnt *scan_next = scan->next;
    int done = FALSE;

    /* done as one so we get error code */
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & (POLLIN|bad_poll_flags))
    {
      struct sockaddr_in sa;
      socklen_t len = sizeof(struct sockaddr_in);
      int fd = 0;
      int acpt_fd = SOCKET_POLL_INDICATOR(scan->ind)->fd;
      struct Evnt *tmp = NULL;
      
      done = TRUE;

      fd = accept(acpt_fd, (struct sockaddr *) &sa, &len);
      if (fd == -1)
        goto next_accept; /* ignore */

      if (!(tmp = scan->cbs->cb_func_accept(fd, (struct sockaddr *) &sa, len)))
        goto next_accept;

      done = FALSE; /* swap the accept() for a read */
      SOCKET_POLL_INDICATOR(tmp->ind)->events  = POLLIN;
      SOCKET_POLL_INDICATOR(tmp->ind)->revents = POLLIN;
    }
    assert(!(SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT));

   next_accept:
    if (done)
      --ready;

    scan = scan_next;
  }
  
  scan = q_send_recv;
  while (scan && ready)
  {
    struct Evnt *scan_next = scan->next;
    int done = FALSE;
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & bad_poll_flags)
    {
      done = TRUE;
      
      evnt_close(scan);
      goto next_send;
    }

    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
    {
      done = TRUE;
      if (!scan->cbs->cb_func_recv(scan))
        goto next_send;
    }
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT)
    {
      done = TRUE;
      evnt_send_add(scan, TRUE, max_sz);
    }

   next_send:
    if (done)
      --ready;

    scan = scan_next;
  }

  scan = q_recv;
  while (scan && ready)
  {
    struct Evnt *scan_next = scan->next;
    int done = FALSE;
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & bad_poll_flags)
    {
      done = TRUE;

      evnt_close(scan);
      goto next_recv;
    }
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
    {
      done = TRUE;
      if (!scan->cbs->cb_func_recv(scan))
        goto next_recv;
    }
    assert(!(SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT));

   next_recv:
    if (done)
      --ready;

    scan = scan_next;
  }
  
  scan = q_none;
  while (scan && ready)
  {
    struct Evnt *scan_next = scan->next;
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & (bad_poll_flags | POLLIN))
    {
      /* POLLIN == EOF */
      --ready;

      evnt_close(scan);
    }
    else
      assert(!SOCKET_POLL_INDICATOR(scan->ind)->revents);

    scan = scan_next;
  }

  if (ready)
    warnx("ready = %d", ready);
  ASSERT(!ready);
}

void evnt_scan_send_fds(void)
{
  struct Evnt *scan = q_send_now;

  q_send_now = NULL;
  while (scan)
  {
    struct Evnt *scan_s_next = scan->s_next;
    
    if (scan->flag_q_send_now && !scan->cbs->cb_func_send(scan))
      evnt_close(scan);
      
    scan = scan_s_next;
  }
}

void evnt_close_all(void)
{
  struct Evnt *scan = NULL;
  
  q_send_now = NULL;
  
  scan = q_accept; q_accept = NULL;
  while (scan)
  {
    struct Evnt *scan_next = scan->next;
    
    evnt__close(scan);
    
    scan = scan_next;
  }
  
  scan = q_recv; q_recv = NULL;
  while (scan)
  {
    struct Evnt *scan_next = scan->next;
    
    evnt__close(scan);
    
    scan = scan_next;
  }
  
  scan = q_send_recv; q_send_recv = NULL;
  while (scan)
  {
    struct Evnt *scan_next = scan->next;
    
    evnt__close(scan);
    
    scan = scan_next;
  }
  
  scan = q_none; q_none = NULL;
  while (scan)
  {
    struct Evnt *scan_next = scan->next;
    
    evnt__close(scan);
    
    scan = scan_next;
  }
}
