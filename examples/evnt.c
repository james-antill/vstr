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
#include <sys/un.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include <socket_poll.h>
#include <timer_q.h>

#include "vlg.h"

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

#ifndef NDEBUG
# define ASSERT(x) do { if (x) {} else vlg_err(vlg, EXIT_FAILURE, "ASSERT(" #x "), FAILED at %s:%u\n", __FILE__, __LINE__); } while (FALSE)
# define assert(x) do { if (x) {} else vlg_err(vlg, EXIT_FAILURE, "assert(" #x "), FAILED at %s:%u\n", __FILE__, __LINE__); } while (FALSE)
#else
# define ASSERT(x)
# define assert(x)
#endif
#define ASSERT_NOT_REACHED() assert(FALSE)

int evnt_opt_nagle = FALSE;

struct Evnt *q_send_now = NULL;  /* Try a send "now" */

struct Evnt *q_none      = NULL; /* nothing */
struct Evnt *q_accept    = NULL; /* connections - recv */
struct Evnt *q_connect   = NULL; /* connections - send */
struct Evnt *q_recv      = NULL; /* recv */
struct Evnt *q_send_recv = NULL; /* recv + send */

static Vlg *vlg = NULL;

static unsigned int evnt__num = 0;

void evnt_logger(Vlg *passed_vlg)
{
  vlg = passed_vlg;
}

void evnt_fd_set_nonblock(int fd, int val)
{
  int flags = 0;

  assert(val);
  
  if ((flags = fcntl(fd, F_GETFL)) == -1)
    vlg_err(vlg, EXIT_FAILURE, __func__);
  
  if (!(flags & O_NONBLOCK) &&
      (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1))
    vlg_err(vlg, EXIT_FAILURE, __func__);
}


#ifndef NDEBUG
static struct Evnt **evnt__srch(struct Evnt **que, struct Evnt *evnt)
{
  struct Evnt **ret = que;

  while (*ret)
  {
    if (*ret == evnt)
      return (ret);
    
    ret = &(*ret)->next;
  }
  
  return (NULL);
}

static int evnt__valid(struct Evnt *evnt)
{
  int ret = 0;
  
  ASSERT((evnt->flag_q_connect + evnt->flag_q_accept + evnt->flag_q_recv +
          evnt->flag_q_send_recv + evnt->flag_q_none) == 1);

  if (evnt->flag_q_send_now)
  {
    struct Evnt **scan = &q_send_now;
    
    while (*scan && (*scan != evnt))
      scan = &(*scan)->s_next;
    ASSERT(*scan);
  }
  
  if (0) { }
  else   if (evnt->flag_q_accept)
    ret = !!evnt__srch(&q_accept, evnt);
  else   if (evnt->flag_q_connect)
    ret = !!evnt__srch(&q_connect, evnt);
  else   if (evnt->flag_q_recv)
    ret = !!evnt__srch(&q_recv, evnt);
  else   if (evnt->flag_q_send_recv)
    ret = !!evnt__srch(&q_send_recv, evnt);
  else   if (evnt->flag_q_none)
    ret = !!evnt__srch(&q_none, evnt);
  else
    ASSERT_NOT_REACHED();
  
  return (ret);
}

static unsigned int evnt__debug_num_1(struct Evnt *scan)
{
  unsigned int num = 0;
  
  while (scan)
  {
    struct Evnt *scan_next = scan->next;
    
    ++num;
    
    scan = scan_next;
  }

  return (num);
}

static unsigned int evnt__debug_num_all(void)
{
  unsigned int num = 0;

  num += evnt__debug_num_1(q_connect);
  num += evnt__debug_num_1(q_accept);  
  num += evnt__debug_num_1(q_recv);  
  num += evnt__debug_num_1(q_send_recv);  
  num += evnt__debug_num_1(q_none);  

  return (num);
}
#endif


int evnt_cb_func_connect(struct Evnt *EVNT__ATTR_UNUSED(evnt))
{
  return (TRUE);
}

struct Evnt *evnt_cb_func_accept(int fd,
                                 struct sockaddr *EVNT__ATTR_UNUSED(sa),
                                 socklen_t EVNT__ATTR_UNUSED(len))
{
  close(fd);
  return (NULL);
}

int evnt_cb_func_recv(struct Evnt *evnt)
{
  unsigned int ern = 0;
  int ret = evnt_recv(evnt, &ern);

  if (ret)
    return (TRUE);

  if (ern == VSTR_TYPE_SC_READ_FD_ERR_EOF)
  {
    SOCKET_POLL_INDICATOR(evnt->ind)->events &= ~POLLIN;
    
    if (evnt->io_r->len || evnt->io_w->len)
      return (TRUE);
  }
  
  return (FALSE);
}

int evnt_cb_func_send(struct Evnt *evnt)
{
  return (evnt_send(evnt));
}

void evnt_cb_func_free(struct Evnt *EVNT__ATTR_UNUSED(evnt))
{
}

static int evnt_init(struct Evnt *evnt, int fd)
{
  evnt->flag_q_accept    = FALSE;
  evnt->flag_q_connect   = FALSE;
  evnt->flag_q_recv      = FALSE;
  evnt->flag_q_send_recv = FALSE;
  evnt->flag_q_none      = FALSE;
  
  evnt->flag_q_send_now  = FALSE;
  
  evnt->acct.req_put = 0;
  evnt->acct.req_got = 0;
  evnt->acct.bytes_r = 0;
  evnt->acct.bytes_w = 0;

  evnt->cbs->cb_func_accept  = evnt_cb_func_accept;
  evnt->cbs->cb_func_connect = evnt_cb_func_connect;
  evnt->cbs->cb_func_recv    = evnt_cb_func_recv;
  evnt->cbs->cb_func_send    = evnt_cb_func_send;
  evnt->cbs->cb_func_free    = evnt_cb_func_free;
  
  if (!(evnt->io_r = vstr_make_base(NULL)))
    goto make_vstr_fail;
  
  if (!(evnt->io_w = vstr_make_base(NULL)))
    goto make_vstr_fail;

  if (!(evnt->ind = socket_poll_add(fd)))
    goto poll_add_fail;

  evnt->sa   = NULL;
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

  errno = ENOMEM;
  return (FALSE);
}

static int evnt__make_true(struct Evnt **que, struct Evnt *node)
{
  evnt_add(que, node);
  
  ++evnt__num;
  return (TRUE);
}


int evnt_make_con_ipv4(struct Evnt *evnt, const char *ipv4_string, short port)
{
  int fd = -1;
  socklen_t alloc_len = sizeof(struct sockaddr_in);
  
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    return (FALSE);
  
  if (!evnt_init(evnt, fd))
  {
    close(fd);
    return (FALSE);
  }
  
  if (!(evnt->sa = malloc(alloc_len)))
  {
    close(fd);
    evnt_free(evnt);
    return (FALSE);
  }
  memset(evnt->sa, 0, alloc_len);
  
  if (!evnt_opt_nagle)
  {
    int nodelay = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
  }
  
  EVNT_SA_IN(evnt)->sin_family = AF_INET;
  EVNT_SA_IN(evnt)->sin_port = htons(port);
  EVNT_SA_IN(evnt)->sin_addr.s_addr = inet_addr(ipv4_string);

  ASSERT(port && (EVNT_SA_IN(evnt)->sin_addr.s_addr != htonl(INADDR_ANY)));
  
  if (connect(fd, evnt->sa, alloc_len) == -1)
  {
    if (errno == EINPROGRESS)
    { /* The connection needs more time....*/
      SOCKET_POLL_INDICATOR(evnt->ind)->events = POLLOUT;
      evnt->flag_q_connect = TRUE;
      return (evnt__make_true(&q_connect, evnt));
    }
    
    close(fd);
    evnt_free(evnt);
    
    return (FALSE);
  }
  
  evnt->flag_q_none = TRUE;
  return (evnt__make_true(&q_none, evnt));
}

int evnt_make_con_local(struct Evnt *evnt, const char *fname)
{
  int fd = -1;
  size_t len = strlen(fname) + 1;
  socklen_t alloc_len = sizeof(struct sockaddr_un);
  
  if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
    return (FALSE);
  
  if (!evnt_init(evnt, fd))
  {
    close(fd);
    return (FALSE);
  }
  
  if (!(evnt->sa = malloc(alloc_len)))
  {
    close(fd);
    evnt_free(evnt);
    return (FALSE);
  }
  memset(evnt->sa, 0, alloc_len);
  
  EVNT_SA_UN(evnt)->sun_family = AF_LOCAL;
  memcpy(EVNT_SA_UN(evnt)->sun_path, fname, len);
  
  alloc_len = sizeof(struct sockaddr_un); /* Linux barfs */
  if (connect(fd, evnt->sa, alloc_len) == -1)
  {
    if (errno == EINPROGRESS)
    { /* The connection needs more time....*/
      SOCKET_POLL_INDICATOR(evnt->ind)->events = POLLOUT;
      evnt->flag_q_connect = TRUE;
      return (evnt__make_true(&q_connect, evnt));
    }
    
    close(fd);
    evnt_free(evnt);
    
    return (FALSE);
  }

  evnt->flag_q_none = TRUE;
  return (evnt__make_true(&q_none, evnt));
}

int evnt_make_acpt(struct Evnt *evnt, int fd,struct sockaddr *sa, socklen_t len)
{
  if (!evnt_init(evnt, fd))
    return (FALSE);
  
  if (!(evnt->sa = malloc(len)))
  {
    evnt_free(evnt);
    return (FALSE);
  }
  memcpy(evnt->sa, sa, len);
  
  if (!evnt_opt_nagle)
  {
    int nodelay = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
  }

  /* assume there is some data */
  SOCKET_POLL_INDICATOR(evnt->ind)->events  = POLLIN;
  SOCKET_POLL_INDICATOR(evnt->ind)->revents = POLLIN;
  
  evnt->flag_q_recv = TRUE;
  return (evnt__make_true(&q_recv, evnt));
}

int evnt_make_bind_ipv4(struct Evnt *evnt,
                        const char *acpt_addr, short server_port)
{
  int fd = -1;
  int opt_t = TRUE;
  int saved_errno = 0;
  socklen_t alloc_len = sizeof(struct sockaddr_in);
  
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    goto sock_fail;
  
  if (!evnt_init(evnt, fd))
    goto init_fail;

  if (!(evnt->sa = malloc(alloc_len)))
  {
    errno = ENOMEM;
    goto malloc_sockaddr_in_fail;
  }
  memset(evnt->sa, 0, alloc_len);

  EVNT_SA_IN(evnt)->sin_family = AF_INET;
  if (acpt_addr)
    EVNT_SA_IN(evnt)->sin_addr.s_addr = inet_addr(acpt_addr); /* FIXME */
  else
  {
    EVNT_SA_IN(evnt)->sin_addr.s_addr = htonl(INADDR_ANY);
    acpt_addr = "any";
  }
  EVNT_SA_IN(evnt)->sin_port = htons(server_port);

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt_t, sizeof(opt_t)) == -1)
    goto reuse_fail;
  
  if (bind(fd, evnt->sa, alloc_len) == -1)
    goto bind_fail;

  if (!server_port)
    if (getsockname(fd, evnt->sa, &alloc_len) == -1)
      vlg_err(vlg, EXIT_FAILURE, "getsockname: %m\n");
      
  
  if (listen(fd, 512) == -1)
    goto listen_fail;

  vstr_free_base(evnt->io_r); evnt->io_r = NULL;
  vstr_free_base(evnt->io_w); evnt->io_w = NULL;
  
  SOCKET_POLL_INDICATOR(evnt->ind)->events |= POLLIN;

  evnt->flag_q_accept = TRUE;
  return (evnt__make_true(&q_accept, evnt));
  
 bind_fail:
  saved_errno = errno;
  vlg_warn(vlg, "bind(%s:%hd): %m\n", acpt_addr, server_port);
  errno = saved_errno;
 listen_fail:
 reuse_fail:
 malloc_sockaddr_in_fail:
 init_fail:
  saved_errno = errno;
  evnt_free(evnt);
  close(fd);
  errno = saved_errno;
 sock_fail:
  return (FALSE);
}

int evnt_make_bind_local(struct Evnt *evnt, const char *fname)
{
  int fd = -1;
  int saved_errno = 0;
  size_t len = strlen(fname) + 1;
  socklen_t alloc_len = sizeof(struct sockaddr_un) + len;
  
  if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
    goto sock_fail;
  
  if (!evnt_init(evnt, fd))
    goto init_fail;
  
  if (!(evnt->sa = malloc(alloc_len)))
  {
    errno = ENOMEM;
    goto malloc_sockaddr_un_fail;
  }
  memset(evnt->sa, 0, alloc_len);
                                                                              
  EVNT_SA_UN(evnt)->sun_family = AF_LOCAL;
  memcpy(EVNT_SA_UN(evnt)->sun_path, fname, len);

  unlink(fname);
  
  alloc_len = sizeof(struct sockaddr_un); /* Linux barfs */
  if (bind(fd, evnt->sa, alloc_len) == -1)
    goto bind_fail;
  
  if (listen(fd, 512) == -1)
    goto listen_fail;

  vstr_free_base(evnt->io_r); evnt->io_r = NULL;
  vstr_free_base(evnt->io_w); evnt->io_w = NULL;
  
  SOCKET_POLL_INDICATOR(evnt->ind)->events |= POLLIN;

  evnt->flag_q_accept = TRUE;
  return (evnt__make_true(&q_accept, evnt));
  
 bind_fail:
  saved_errno = errno;
  vlg_warn(vlg, "bind(%s): %m\n", fname);
  errno = saved_errno;
 listen_fail:
 malloc_sockaddr_un_fail:
 init_fail:
  saved_errno = errno;
  evnt_free(evnt);
  close(fd);
  errno = saved_errno;
 sock_fail:
  return (FALSE);
}

static void evnt__free(struct Evnt *evnt)
{
  evnt_send_del(evnt);
  
  vstr_free_base(evnt->io_w); evnt->io_w = NULL;
  vstr_free_base(evnt->io_r); evnt->io_r = NULL;
  
  socket_poll_del(evnt->ind);

  ASSERT(evnt__num >= 1);
  --evnt__num;
}

void evnt_free(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));
  
  evnt__free(evnt);
  
  free(evnt->sa);
  
  if (evnt->tm_o)
    timer_q_quick_del_node(evnt->tm_o);
}

static void evnt__close(struct Evnt *evnt)
{
  struct sockaddr *sa = evnt->sa;
  Timer_q_node *tm_o  = evnt->tm_o;
  
  close(SOCKET_POLL_INDICATOR(evnt->ind)->fd);
  evnt__free(evnt);
  evnt->cbs->cb_func_free(evnt);

  free(sa);
  
  if (tm_o)
    timer_q_quick_del_node(tm_o);
}

void evnt_close(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));
  
  if (0) { }
  else if (evnt->flag_q_accept)
    evnt_del(&q_accept, evnt);
  else if (evnt->flag_q_connect)
    evnt_del(&q_connect, evnt);
  else if (evnt->flag_q_recv)
    evnt_del(&q_recv, evnt);
  else if (evnt->flag_q_send_recv)
    evnt_del(&q_send_recv, evnt);
  else if (evnt->flag_q_none)
    evnt_del(&q_none, evnt);
  else
    ASSERT_NOT_REACHED();

  evnt__close(evnt);
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

void evnt_put_pkt(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));

  ASSERT(evnt->acct.req_put >= evnt->acct.req_got);
  ++evnt->acct.req_put;
  
  if (evnt->flag_q_none)
  {
    evnt_del(&q_none, evnt); evnt->flag_q_none = FALSE;
    evnt_add(&q_recv, evnt); evnt->flag_q_recv = TRUE;
  }
  
  ASSERT(evnt__valid(evnt));
}

void evnt_got_pkt(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));
  
  ++evnt->acct.req_got;
  ASSERT(evnt->acct.req_put >= evnt->acct.req_got);
  
  if (!evnt->flag_q_send_recv && (evnt->acct.req_put == evnt->acct.req_got))
  {
    evnt_del(&q_recv, evnt), evnt->flag_q_recv = FALSE;
    evnt_add(&q_none, evnt), evnt->flag_q_none = TRUE;
  }
  
  ASSERT(evnt__valid(evnt));
}

static int evnt__call_send(struct Evnt *evnt, unsigned int *ern)
{
  size_t tmp = evnt->io_w->len;
  int fd = SOCKET_POLL_INDICATOR(evnt->ind)->fd;
  
  if (!vstr_sc_write_fd(evnt->io_w, 1, tmp, fd, ern) && (errno != EAGAIN))
    return (FALSE);

  evnt->acct.bytes_w += (tmp - evnt->io_w->len);
  
  return (TRUE);
}

int evnt_send_add(struct Evnt *evnt, int force_q, size_t max_sz)
{
  ASSERT(evnt__valid(evnt));
  
  vlg_dbg3(vlg, "q now = %u, q send recv = %u, force = %u\n",
           evnt->flag_q_send_now, evnt->flag_q_send_recv, force_q);
  
  if (!evnt->flag_q_send_recv && (evnt->io_w->len > max_sz))
  {
    if (!evnt__call_send(evnt, NULL))
    {
      ASSERT(evnt__valid(evnt));
      return (FALSE);
    }
    if (!evnt->io_w->len && !force_q)
    {
      ASSERT(evnt__valid(evnt));
      return (TRUE);
    }
  }

  /* already on send_q -- or already polling (and not forcing) */
  if (evnt->flag_q_send_now || (evnt->flag_q_send_recv && !force_q))
  {
    ASSERT(evnt__valid(evnt));
    return (TRUE);
  }
  
  evnt->s_next = q_send_now;
  q_send_now = evnt;
  evnt->flag_q_send_now = TRUE;
  
  ASSERT(evnt__valid(evnt));
  
  return (TRUE);
}

/* if a connection is on the send now q, then remove them ... this is only
 * done when the client gets killed, so it doesn't matter it's slow */
void evnt_send_del(struct Evnt *evnt)
{
  struct Evnt **scan = &q_send_now;
  
  if (!evnt->flag_q_send_now)
    return;
  
  while (*scan && (*scan != evnt))
    scan = &(*scan)->s_next;
  
  ASSERT(*scan);
  
  *scan = evnt->s_next;

  evnt->flag_q_send_now = FALSE;
}

int evnt_recv(struct Evnt *evnt, unsigned int *ern)
{
  int ret = TRUE;
  size_t tmp = evnt->io_r->len;
  
  ASSERT(evnt__valid(evnt));
  
  vstr_sc_read_iov_fd(evnt->io_r, evnt->io_r->len,
                      SOCKET_POLL_INDICATOR(evnt->ind)->fd, 4, 32, ern);
  evnt->acct.bytes_r += (evnt->io_r->len - tmp);
  
  switch (*ern)
  {
    case VSTR_TYPE_SC_READ_FD_ERR_NONE:
      gettimeofday(&evnt->mtime, NULL);
      break;
    case VSTR_TYPE_SC_READ_FD_ERR_MEM:
      VLG_WARN_RET(FALSE, (vlg, __func__));

    case VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO:
      if (errno == EAGAIN)
        return (TRUE);
      ret = FALSE;
      break;

    case VSTR_TYPE_SC_READ_FD_ERR_EOF:
      ret = FALSE;
      break;

    default: /* unknown */
      VLG_WARN_RET(FALSE, (vlg, "read_iov() = %d: %m\n", *ern));
  }
  
  return (ret);
}

int evnt_send(struct Evnt *evnt)
{
  unsigned int ern = 0;
  
  ASSERT(evnt__valid(evnt));

  if (!evnt__call_send(evnt, &ern))
    return (FALSE);

  gettimeofday(&evnt->mtime, NULL);
  
  if (0)
  { /* nothing */ }
  else if ( evnt->flag_q_send_recv && !evnt->io_w->len)
  {
    evnt_del(&q_send_recv, evnt);
    ASSERT(evnt->acct.req_put >= evnt->acct.req_got);
    if (evnt->acct.req_put == evnt->acct.req_got)
      evnt_add(&q_none, evnt), evnt->flag_q_none = TRUE;
    else
      evnt_add(&q_recv, evnt), evnt->flag_q_recv = TRUE;
    SOCKET_POLL_INDICATOR(evnt->ind)->events  &= ~POLLOUT;
    SOCKET_POLL_INDICATOR(evnt->ind)->revents &= ~POLLOUT;
    evnt->flag_q_send_recv = FALSE;
  }
  else if (!evnt->flag_q_send_recv &&  evnt->io_w->len)
  {
    if (evnt->flag_q_none)
      evnt_del(&q_none, evnt), evnt->flag_q_none = FALSE;
    else
      evnt_del(&q_recv, evnt), evnt->flag_q_recv = FALSE;
    evnt_add(&q_send_recv, evnt);
    SOCKET_POLL_INDICATOR(evnt->ind)->events |=  POLLOUT;
    evnt->flag_q_send_recv = TRUE;
  }
  
  ASSERT(evnt__valid(evnt));
  
  return (TRUE);
}

int evnt_poll(void)
{
  const struct timeval *tv = timer_q_first_timeval();
  int msecs = -1;

  if (q_send_now)
    msecs = 0;
  else if (tv)
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
        vlg_err(vlg, EXIT_FAILURE, "getsockopt(SO_ERROR): %m\n");
      else if (ern)
      {
        /* server_ipv4_address */
        if (ern == ECONNREFUSED)
          errno = ern, vlg_err(vlg, EXIT_FAILURE, "connect(): %m\n");

        evnt_close(scan);
        goto next_connect;
      }
      else
      {
        SOCKET_POLL_INDICATOR(scan->ind)->events  = 0;
        SOCKET_POLL_INDICATOR(scan->ind)->revents = 0;
        
        evnt_del(&q_connect, scan); scan->flag_q_connect = FALSE;
        evnt_add(&q_none,    scan); scan->flag_q_none    = TRUE;
        
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
      {
        close(fd);
        goto next_accept;
      }
      
      done = FALSE; /* swap the accept() for a read */
      assert(SOCKET_POLL_INDICATOR(tmp->ind)->events  == POLLIN);
      assert(SOCKET_POLL_INDICATOR(tmp->ind)->revents == POLLIN);
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
      {
        evnt_close(scan);
        goto next_send;
      }
    }
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT)
    {
      done = TRUE; /* need groups so we can do direct send here */
      if (!evnt_send_add(scan, TRUE, max_sz))
        evnt_close(scan);
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
      {
        evnt_close(scan);
        goto next_recv;
      }
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
    vlg_err(vlg, EXIT_FAILURE, "ready = %d\n", ready);
}

void evnt_scan_send_fds(void)
{
  struct Evnt *scan = q_send_now;

  q_send_now = NULL;
  while (scan)
  {
    struct Evnt *scan_s_next = scan->s_next;
    
    ASSERT(scan->flag_q_send_now);
    
    scan->flag_q_send_now = FALSE;
    if (!scan->cbs->cb_func_send(scan))
      evnt_close(scan);
      
    scan = scan_s_next;
  }
}

static void evnt__close_1(struct Evnt *scan)
{
  while (scan)
  {
    struct Evnt *scan_next = scan->next;
    
    evnt__close(scan);
    
    scan = scan_next;
  }
}

void evnt_close_all(void)
{
  q_send_now = NULL;
  
  evnt__close_1(q_connect);   q_connect = NULL;
  evnt__close_1(q_accept);    q_accept = NULL;
  evnt__close_1(q_recv);      q_recv = NULL;
  evnt__close_1(q_send_recv); q_send_recv = NULL;
  evnt__close_1(q_none);      q_none = NULL;
}

void evnt_out_dbg3(const char *prefix)
{
  if (vlg->out_dbg < 3)
    return;
  
  vlg_dbg3(vlg, "%s T=%u c=%u a=%u r=%u s=%u n=%u [SN=%u]\n",
           prefix, evnt_num_all(),
           evnt__debug_num_1(q_connect),
           evnt__debug_num_1(q_accept),
           evnt__debug_num_1(q_recv),
           evnt__debug_num_1(q_send_recv),
           evnt__debug_num_1(q_none),
           evnt__debug_num_1(q_send_now));
}

unsigned int evnt_num_all(void)
{
  ASSERT(evnt__num == evnt__debug_num_all());
  return (evnt__num);
}

int evnt_waiting(void)
{
  return (q_connect || q_accept || q_recv || q_send_recv || q_send_now);
}
