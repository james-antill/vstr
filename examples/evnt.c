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
#include <sys/stat.h>

#include <socket_poll.h>
#include <timer_q.h>

#include <sys/sendfile.h>

#define CONF_EVNT_NO_EPOLL FALSE

#if !defined(SO_DETACH_FILTER) || !defined(SO_ATTACH_FILTER)
# define CONF_USE_SOCKET_FILTERS FALSE
struct sock_fprog { int dummy; };
# define SO_DETACH_FILTER 0
# define SO_ATTACH_FILTER 0
#else
# define CONF_USE_SOCKET_FILTERS TRUE

/* not in glibc... hope it's not in *BSD etc. */
struct sock_filter
{
 uint16_t   code;   /* Actual filter code */
 uint8_t    jt;     /* Jump true */
 uint8_t    jf;     /* Jump false */
 uint32_t   k;      /* Generic multiuse field */
};

struct sock_fprog
{
 unsigned short len;    /* Number of filter blocks */
 struct sock_filter *filter;
};
#endif

#include "vlg.h"
#include "vlg_assert.h"

#define TRUE 1
#define FALSE 0

#include "evnt.h"

#define CSTREQ(x,y) (!strcmp(x,y))

#ifdef __GNUC__
# define EVNT__ATTR_UNUSED(x) vstr__UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define EVNT__ATTR_UNUSED(x) /*@unused@*/ vstr__UNUSED_ ## x
#else
# define EVNT__ATTR_UNUSED(x) vstr__UNUSED_ ## x
#endif

#ifdef __GNUC__
# define EVNT__ATTR_USED() __attribute__((used))
#else
# define EVNT__ATTR_USED() 
#endif

int evnt_opt_nagle = FALSE;

static struct Evnt *q_send_now = NULL;  /* Try a send "now" */
static struct Evnt *q_closed   = NULL;  /* Close when fin. */

static struct Evnt *q_none      = NULL; /* nothing */
static struct Evnt *q_accept    = NULL; /* connections - recv */
static struct Evnt *q_connect   = NULL; /* connections - send */
static struct Evnt *q_recv      = NULL; /* recv */
static struct Evnt *q_send_recv = NULL; /* recv + send */

static Vlg *vlg = NULL;

static unsigned int evnt__num = 0;

/* this should be more configurable... */
static unsigned int evnt__accept_limit = 4;

void evnt_logger(Vlg *passed_vlg)
{
  vlg = passed_vlg;
}

void evnt_fd__set_nonblock(int fd, int val)
{
  int flags = 0;

  assert(val);
  
  if ((flags = fcntl(fd, F_GETFL)) == -1)
    vlg_err(vlg, EXIT_FAILURE, __func__);
  
  if (!(flags & O_NONBLOCK) &&
      (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1))
    vlg_err(vlg, EXIT_FAILURE, __func__);
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

#ifndef VSTR_AUTOCONF_NDEBUG
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

  ASSERT(evnt_num_all());
  
  ASSERT((evnt->flag_q_connect + evnt->flag_q_accept + evnt->flag_q_recv +
          evnt->flag_q_send_recv + evnt->flag_q_none) == 1);

  if (evnt->flag_q_send_now)
  {
    struct Evnt **scan = &q_send_now;
    
    while (*scan && (*scan != evnt))
      scan = &(*scan)->s_next;
    ASSERT(*scan);
  }
  
  if (evnt->flag_q_closed)
  {
    struct Evnt **scan = &q_closed;
    
    while (*scan && (*scan != evnt))
      scan = &(*scan)->c_next;
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

int evnt_fd(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));
  return (SOCKET_POLL_INDICATOR(evnt->ind)->fd);
}

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

  if ((ern == VSTR_TYPE_SC_READ_FD_ERR_EOF) && evnt->io_w->len)
    return (evnt_shutdown_r(evnt, TRUE));
  
  return (FALSE);
}

int evnt_cb_func_send(struct Evnt *evnt)
{
  int ret = -1;

  evnt_fd_set_cork(evnt, TRUE);
  ret = evnt_send(evnt);
  if (!evnt->io_w->len)
    evnt_fd_set_cork(evnt, FALSE);
  
  return (ret);
}

void evnt_cb_func_free(struct Evnt *EVNT__ATTR_UNUSED(evnt))
{
}

int evnt_cb_func_shutdown_r(struct Evnt *evnt)
{
  vlg_dbg3(vlg, "SHUTDOWN from[$<sa:%p>]\n", evnt->sa);
  
  if (!evnt_shutdown_r(evnt, FALSE))
    return (FALSE);

  /* called from outside read, and read'll never get called again ...
   * so quit if we have nothing to send */
  return (!!evnt->io_w->len);
}

static int evnt_init(struct Evnt *evnt, int fd, socklen_t sa_len)
{
  evnt->flag_q_accept    = FALSE;
  evnt->flag_q_connect   = FALSE;
  evnt->flag_q_recv      = FALSE;
  evnt->flag_q_send_recv = FALSE;
  evnt->flag_q_none      = FALSE;
  
  evnt->flag_q_send_now  = FALSE;
  evnt->flag_q_closed    = FALSE;

  evnt->flag_q_pkt_move  = FALSE;

  /* FIXME: need group settings, default no nagle but cork */
  evnt->flag_io_nagle    = evnt_opt_nagle;
  evnt->flag_io_cork     = FALSE;

  evnt->flag_io_filter   = FALSE;
  
  evnt->io_r_shutdown    = FALSE;
  evnt->io_w_shutdown    = FALSE;
  
  evnt->prev_bytes_r = 0;
  
  evnt->acct.req_put = 0;
  evnt->acct.req_got = 0;
  evnt->acct.bytes_r = 0;
  evnt->acct.bytes_w = 0;

  evnt->cbs->cb_func_accept     = evnt_cb_func_accept;
  evnt->cbs->cb_func_connect    = evnt_cb_func_connect;
  evnt->cbs->cb_func_recv       = evnt_cb_func_recv;
  evnt->cbs->cb_func_send       = evnt_cb_func_send;
  evnt->cbs->cb_func_free       = evnt_cb_func_free;
  evnt->cbs->cb_func_shutdown_r = evnt_cb_func_shutdown_r;
  
  if (!(evnt->io_r = vstr_make_base(NULL)))
    goto make_vstr_fail;
  
  if (!(evnt->io_w = vstr_make_base(NULL)))
    goto make_vstr_fail;
  
  evnt->tm_o = NULL;
  
  if (!(evnt->ind = evnt_poll_add(evnt, fd)))
    goto poll_add_fail;

  gettimeofday(&evnt->ctime, NULL);
  gettimeofday(&evnt->mtime, NULL);

  evnt->msecs_tm_mtime = 0;
  
  if (fcntl(fd, F_SETFD, TRUE) == -1)
    goto fcntl_fail;

  evnt->sa = NULL;
  if (sa_len && !(evnt->sa = calloc(1, sa_len)))
    goto malloc_sockaddr_fail;
  
  evnt_fd__set_nonblock(fd, TRUE);

  return (TRUE);

 malloc_sockaddr_fail:
  errno = ENOMEM;
 fcntl_fail:
  evnt_poll_del(evnt);
 poll_add_fail:
  vstr_free_base(evnt->io_w);
 make_vstr_fail:
  vstr_free_base(evnt->io_r);

  errno = ENOMEM;
  return (FALSE);
}

static void evnt__free1(struct Evnt *evnt)
{
  evnt_send_del(evnt);
  
  vstr_free_base(evnt->io_w); evnt->io_w = NULL;
  vstr_free_base(evnt->io_r); evnt->io_r = NULL;
  
  evnt_poll_del(evnt);
}

static void evnt__free2(struct sockaddr *sa, Timer_q_node *tm_o)
{ /* post callbacks, evnt no longer exists */
  free(sa);
  
  if (tm_o)
    timer_q_quick_del_node(tm_o);
}

void evnt_free(struct Evnt *evnt)
{
  if (evnt)
  {
    struct sockaddr *sa = evnt->sa;
    Timer_q_node *tm_o  = evnt->tm_o;
  
    evnt__free1(evnt);

    ASSERT(evnt__num >= 1); /* in case they come back in via. the cb */
    --evnt__num;

    evnt->cbs->cb_func_free(evnt);
    evnt__free2(sa, tm_o);
  }
}

static void evnt__uninit(struct Evnt *evnt)
{
  ASSERT((evnt->flag_q_connect + evnt->flag_q_accept + evnt->flag_q_recv +
          evnt->flag_q_send_recv + evnt->flag_q_none) == 0);

  evnt__free1(evnt);
  evnt__free2(evnt->sa, evnt->tm_o);
}

static int evnt__make_true(struct Evnt **que, struct Evnt *evnt, int flags)
{
  evnt_add(que, evnt);
  
  ++evnt__num;

  ASSERT(evnt__valid(evnt));

  evnt_wait_cntl_add(evnt, flags);

  return (TRUE);
}

static int evnt_fd__set_nodelay(int fd, int val)
{
  return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) != -1);
}

static int evnt_fd__set_reuse(int fd, int val)
{
  return (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != -1);
}

static void evnt__fd_close_noerrno(int fd)
{
  int saved_errno = errno;
  close(fd);
  errno = saved_errno;
}

int evnt_make_con_ipv4(struct Evnt *evnt, const char *ipv4_string, short port)
{
  int fd = -1;
  socklen_t alloc_len = sizeof(struct sockaddr_in);
  
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    goto sock_fail;
  
  if (!evnt_init(evnt, fd, alloc_len))
    goto init_fail;
  evnt->flag_q_pkt_move = TRUE;
  
  if (!evnt->flag_io_nagle)
    evnt_fd__set_nodelay(fd, TRUE);
  
  EVNT_SA_IN(evnt)->sin_family = AF_INET;
  EVNT_SA_IN(evnt)->sin_port = htons(port);
  EVNT_SA_IN(evnt)->sin_addr.s_addr = inet_addr(ipv4_string);

  ASSERT(port && (EVNT_SA_IN(evnt)->sin_addr.s_addr != htonl(INADDR_ANY)));
  
  if (connect(fd, evnt->sa, alloc_len) == -1)
  {
    if (errno == EINPROGRESS)
    { /* The connection needs more time....*/
      evnt->flag_q_connect = TRUE;
      return (evnt__make_true(&q_connect, evnt, POLLOUT));
    }

    goto connect_fail;
  }
  
  evnt->flag_q_none = TRUE;
  return (evnt__make_true(&q_none, evnt, 0));
  
 connect_fail:
  evnt__uninit(evnt);
 init_fail:
  evnt__fd_close_noerrno(fd);
 sock_fail:
  return (FALSE);
}

int evnt_make_con_local(struct Evnt *evnt, const char *fname)
{
  int fd = -1;
  size_t len = strlen(fname) + 1;
  struct sockaddr_un tmp_sun;
  socklen_t alloc_len = 0;

  tmp_sun.sun_path[0] = 0;
  alloc_len = SUN_LEN(&tmp_sun) + len;
  
  if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
    goto sock_fail;
  
  if (!evnt_init(evnt, fd, alloc_len))
    goto init_fail;
  evnt->flag_q_pkt_move = TRUE;
  
  if (!evnt->flag_io_nagle)
    evnt_fd__set_nodelay(fd, TRUE);
  
  EVNT_SA_UN(evnt)->sun_family = AF_LOCAL;
  memcpy(EVNT_SA_UN(evnt)->sun_path, fname, len);
  
  if (connect(fd, evnt->sa, alloc_len) == -1)
  {
    if (errno == EINPROGRESS)
    { /* The connection needs more time....*/
      evnt->flag_q_connect = TRUE;
      return (evnt__make_true(&q_connect, evnt, POLLOUT));
    }
    
    goto connect_fail;
  }

  evnt->flag_q_none = TRUE;
  return (evnt__make_true(&q_none, evnt, 0));
  
 connect_fail:
  evnt__uninit(evnt);
 init_fail:
  evnt__fd_close_noerrno(fd);
 sock_fail:
  return (FALSE);
}

int evnt_make_acpt(struct Evnt *evnt, int fd,struct sockaddr *sa, socklen_t len)
{
  if (!evnt_init(evnt, fd, len))
    return (FALSE);
  memcpy(evnt->sa, sa, len);
  
  if (!evnt->flag_io_nagle)
    evnt_fd__set_nodelay(fd, TRUE);

  evnt->flag_q_recv = TRUE;
  return (evnt__make_true(&q_recv, evnt, POLLIN));
}

int evnt_make_bind_ipv4(struct Evnt *evnt,
                        const char *acpt_addr, short server_port)
{
  int fd = -1;
  int saved_errno = 0;
  socklen_t alloc_len = sizeof(struct sockaddr_in);
  
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
    goto sock_fail;
  
  if (!evnt_init(evnt, fd, alloc_len))
    goto init_fail;

  EVNT_SA_IN(evnt)->sin_family = AF_INET;
  if (acpt_addr)
    EVNT_SA_IN(evnt)->sin_addr.s_addr = inet_addr(acpt_addr); /* FIXME */
  else
  {
    EVNT_SA_IN(evnt)->sin_addr.s_addr = htonl(INADDR_ANY);
    acpt_addr = "any";
  }
  EVNT_SA_IN(evnt)->sin_port = htons(server_port);

  if (!evnt_fd__set_reuse(fd, TRUE))
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
  
  evnt->flag_q_accept = TRUE;
  return (evnt__make_true(&q_accept, evnt, POLLIN));
  
 bind_fail:
  saved_errno = errno;
  vlg_warn(vlg, "bind(%s:%hd): %m\n", acpt_addr, server_port);
  errno = saved_errno;
 listen_fail:
 reuse_fail:
  evnt__uninit(evnt);
 init_fail:
  evnt__fd_close_noerrno(fd);
 sock_fail:
  return (FALSE);
}

int evnt_make_bind_local(struct Evnt *evnt, const char *fname)
{
  int fd = -1;
  int saved_errno = 0;
  size_t len = strlen(fname) + 1;
  struct sockaddr_un tmp_sun;
  socklen_t alloc_len = 0;

  tmp_sun.sun_path[0] = 0;
  alloc_len = SUN_LEN(&tmp_sun) + len;
  
  if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
    goto sock_fail;
  
  if (!evnt_init(evnt, fd, alloc_len))
    goto init_fail;

  EVNT_SA_UN(evnt)->sun_family = AF_LOCAL;
  memcpy(EVNT_SA_UN(evnt)->sun_path, fname, len);

  unlink(fname);
  
  if (bind(fd, evnt->sa, alloc_len) == -1)
    goto bind_fail;

  if (fchmod(fd, 0600) == -1)
    goto fchmod_fail;
  
  if (listen(fd, 512) == -1)
    goto listen_fail;

  vstr_free_base(evnt->io_r); evnt->io_r = NULL;
  vstr_free_base(evnt->io_w); evnt->io_w = NULL;
  
  evnt->flag_q_accept = TRUE;
  return (evnt__make_true(&q_accept, evnt, POLLIN));
  
 bind_fail:
  saved_errno = errno;
  vlg_warn(vlg, "bind(%s): %m\n", fname);
  errno = saved_errno;
 fchmod_fail:
 listen_fail:
  evnt__uninit(evnt);
 init_fail:
  evnt__fd_close_noerrno(fd);
 sock_fail:
  return (FALSE);
}

int evnt_make_custom(struct Evnt *evnt, int fd, socklen_t sa_len, int flags)
{
  if (!evnt_init(evnt, fd, sa_len))
  {
    evnt__fd_close_noerrno(fd);
    return (FALSE);
  }
  
  if (flags & POLLIN)
  {
    evnt->flag_q_recv = TRUE;
    return (evnt__make_true(&q_recv, evnt, POLLIN));
  }
  
  evnt->flag_q_none = TRUE;
  return (evnt__make_true(&q_none, evnt, 0));
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

static void evnt__del_whatever(struct Evnt *evnt)
{
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
}

static void EVNT__ATTR_USED() evnt__add_whatever(struct Evnt *evnt)
{
  if (0) { }
  else if (evnt->flag_q_accept)
    evnt_add(&q_accept, evnt);
  else if (evnt->flag_q_connect)
    evnt_add(&q_connect, evnt);
  else if (evnt->flag_q_recv)
    evnt_add(&q_recv, evnt);
  else if (evnt->flag_q_send_recv)
    evnt_add(&q_send_recv, evnt);
  else if (evnt->flag_q_none)
    evnt_add(&q_none, evnt);
  else
    ASSERT_NOT_REACHED();
}

void evnt_close(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));

  /* close the fd, so it goes away "instantly" */
  close(SOCKET_POLL_INDICATOR(evnt->ind)->fd);
  SOCKET_POLL_INDICATOR(evnt->ind)->fd = -1;

  /* now queue for deletion ... as the bt might still be using the ptr */
  evnt->flag_q_closed = TRUE;
  evnt->c_next = q_closed;
  q_closed = evnt;
}

void evnt_put_pkt(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));

  if (evnt->flag_q_pkt_move && evnt->flag_q_none)
  {
    ASSERT(evnt->acct.req_put >= evnt->acct.req_got);
    evnt_del(&q_none, evnt); evnt->flag_q_none = FALSE;
    evnt_add(&q_recv, evnt); evnt->flag_q_recv = TRUE;
  }
  
  ++evnt->acct.req_put;
  
  ASSERT(evnt__valid(evnt));
}

void evnt_got_pkt(struct Evnt *evnt)
{
  ASSERT(evnt__valid(evnt));
  
  ++evnt->acct.req_got;
  
  if (evnt->flag_q_pkt_move &&
      !evnt->flag_q_send_recv && (evnt->acct.req_put == evnt->acct.req_got))
  {
    ASSERT(evnt->acct.req_put >= evnt->acct.req_got);
    evnt_del(&q_recv, evnt), evnt->flag_q_recv = FALSE;
    evnt_add(&q_none, evnt), evnt->flag_q_none = TRUE;
  }
  
  ASSERT(evnt__valid(evnt));
}

static int evnt__call_send(struct Evnt *evnt, unsigned int *ern)
{
  size_t tmp = evnt->io_w->len;
  int fd = evnt_fd(evnt);
  
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

int evnt_shutdown_r(struct Evnt *evnt, int got_eof)
{
  int ret = FALSE;
  unsigned int ern = 0;

  evnt_wait_cntl_del(evnt, POLLIN);
  
  if (!got_eof)
  {
    /* only a "small" amount of data left now... */
    while (!got_eof && (ret = evnt_recv(evnt, &ern)))
    { }

    vlg_dbg3(vlg, "shutdown(SHUT_RD) from[$<sa:%p>]\n", evnt->sa);

    if (shutdown(evnt_fd(evnt), SHUT_RD) == -1)
    {
      if (errno != ENOTCONN)
        vlg_warn(vlg, "shutdown(SHUT_RD): %m\n");
      return (FALSE);
    }
  }
  
  evnt->io_r_shutdown = TRUE;

  return (TRUE);
}

static void evnt__send_fin(struct Evnt *evnt)
{
  if (0)
  { /* nothing */ }
  else if ( evnt->flag_q_send_recv && !evnt->io_w->len)
  {
    evnt_del(&q_send_recv, evnt);
    if (evnt->flag_q_pkt_move && (evnt->acct.req_put == evnt->acct.req_got))
      evnt_add(&q_none, evnt), evnt->flag_q_none = TRUE;
    else
      evnt_add(&q_recv, evnt), evnt->flag_q_recv = TRUE;
    evnt_wait_cntl_del(evnt, POLLOUT);
    evnt->flag_q_send_recv = FALSE;
  }
  else if (!evnt->flag_q_send_recv &&  evnt->io_w->len)
  {
    ASSERT(evnt->flag_q_none || evnt->flag_q_recv);
    if (evnt->flag_q_none)
      evnt_del(&q_none, evnt), evnt->flag_q_none = FALSE;
    else
      evnt_del(&q_recv, evnt), evnt->flag_q_recv = FALSE;
    evnt_add(&q_send_recv, evnt);
    evnt_wait_cntl_add(evnt, POLLOUT);
    evnt->flag_q_send_recv = TRUE;
  }
}

int evnt_shutdown_w(struct Evnt *evnt)
{
  Vstr_base *out = evnt->io_w;
  
  ASSERT(evnt__valid(evnt));

  vlg_dbg3(vlg, "shutdown(SHUT_WR) from[$<sa:%p>]\n", evnt->sa);
  
  if (shutdown(evnt_fd(evnt), SHUT_WR) == -1)
  {
    if (errno != ENOTCONN)
      vlg_warn(vlg, "shutdown(SHUT_WR): %m\n");
    return (FALSE);
  }
  evnt_wait_cntl_del(evnt, POLLOUT);
  evnt->io_w_shutdown = TRUE;

  vstr_del(out, 1, out->len);
  
  evnt__send_fin(evnt);
  
  ASSERT(evnt__valid(evnt));

  return (TRUE);
}

int evnt_recv(struct Evnt *evnt, unsigned int *ern)
{
  Vstr_base *data = evnt->io_r;
  int ret = TRUE;
  size_t tmp = evnt->io_r->len;
  unsigned int num_min = 2;
  unsigned int num_max = 6; /* ave. browser reqs are 500ish, ab is much less */
  
  ASSERT(evnt__valid(evnt) && ern);

  /* FIXME: this is set for HTTPD's default buf sizes of 120 (128 - 8) */
  if (evnt->prev_bytes_r > (120 * 3))
    num_max =  8;
  if (evnt->prev_bytes_r > (120 * 6))
    num_max = 32;
  
  vstr_sc_read_iov_fd(data, data->len, evnt_fd(evnt), num_min, num_max, ern);
  evnt->prev_bytes_r = (evnt->io_r->len - tmp);
  evnt->acct.bytes_r += evnt->prev_bytes_r;
  
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

  evnt__send_fin(evnt);
  
  ASSERT(evnt__valid(evnt));
  
  return (TRUE);
}

#ifdef VSTR_AUTOCONF_lseek64
# define sendfile64 sendfile
#endif

int evnt_sendfile(struct Evnt *evnt, int ffd, VSTR_AUTOCONF_uintmax_t *f_off,
                  VSTR_AUTOCONF_uintmax_t *f_len, unsigned int *ern)
{
  ssize_t ret = 0;
  off64_t tmp_off = *f_off;
  size_t  tmp_len = *f_len;
  
  *ern = 0;
  
  ASSERT(evnt__valid(evnt));

  if (evnt->io_w->len)
  {
    if (!evnt__call_send(evnt, ern))
      return (FALSE);

    gettimeofday(&evnt->mtime, NULL);
    
    return (TRUE);
  }
  
  ASSERT(evnt__valid(evnt));

  if (*f_len > SSIZE_MAX)
    tmp_len =  SSIZE_MAX;
  
  if ((ret = sendfile64(evnt_fd(evnt), ffd, &tmp_off, tmp_len)) == -1)
  {
    if (errno == EAGAIN)
      return (TRUE);

    *ern = VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO;
    return (FALSE);
  }

  if (!ret)
  {
    *ern = VSTR_TYPE_SC_READ_FD_ERR_EOF;
    return (FALSE);
  }
  
  *f_off = tmp_off;
  
  evnt->acct.bytes_w += ret;
  gettimeofday(&evnt->mtime, NULL);
  
  *f_len -= ret;

  return (TRUE);
}

static int evnt__get_timeout(void)
{
  const struct timeval *tv = NULL;
  int msecs = -1;
  
  if (q_send_now)
    msecs = 0;
  else if ((tv = timer_q_first_timeval()))
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
  
  return (msecs);
}

static void evnt__close_1(struct Evnt *scan)
{
  while (scan)
  {
    struct Evnt *scan_next = scan->next;
    
    close(SOCKET_POLL_INDICATOR(scan->ind)->fd);
    evnt_free(scan);
    
    scan = scan_next;
  }
}

static void evnt__scan_q_close(void)
{
  struct Evnt *scan = q_closed;

  q_closed = NULL;
  while (scan)
  {
    struct Evnt *scan_c_next = scan->c_next;
    
    evnt__del_whatever(scan);
    evnt_free(scan);
    
    scan = scan_c_next;
  }
}

static void evnt__close_now(struct Evnt *evnt)
{
  if (evnt->flag_q_closed)
    return;
  
  close(SOCKET_POLL_INDICATOR(evnt->ind)->fd);
  evnt__del_whatever(evnt);
  evnt_free(evnt);
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

    if (scan->flag_q_closed)
      goto next_connect;
    
    assert(!(SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN));
    /* done as one so we get error code */
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & (POLLOUT|bad_poll_flags))
    {
      int ern = 0;
      socklen_t len = sizeof(int);
      int ret = 0;
      
      done = TRUE;
      
      ret = getsockopt(evnt_fd(scan), SOL_SOCKET, SO_ERROR, &ern, &len);
      if (ret == -1)
        vlg_err(vlg, EXIT_FAILURE, "getsockopt(SO_ERROR): %m\n");
      else if (ern)
      {
        errno = ern;
        vlg_warn(vlg, "connect(): %m\n");

        evnt__close_now(scan);
      }
      else
      {
        SOCKET_POLL_INDICATOR(scan->ind)->events  = 0;
        SOCKET_POLL_INDICATOR(scan->ind)->revents = 0;
        
        evnt_del(&q_connect, scan); scan->flag_q_connect = FALSE;
        evnt_add(&q_none,    scan); scan->flag_q_none    = TRUE;
        
        if (!scan->cbs->cb_func_connect(scan))
          evnt__close_now(scan);
      }
      goto next_connect;
    }
    ASSERT(!done);
    if (evnt_epoll_enabled()) break;

   next_connect:
    SOCKET_POLL_INDICATOR(scan->ind)->revents = 0;
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

    if (scan->flag_q_closed)
      goto next_accept;
    
    assert(!(SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT));
    if (!done && (SOCKET_POLL_INDICATOR(scan->ind)->revents & bad_poll_flags))
    { /* done first as it's an error with the accept fd, whereas accept
       * generates new fds */
      done = TRUE;
      evnt__close_now(scan);
      goto next_accept;
    }

    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
    {
      struct sockaddr_in sa;
      socklen_t len = sizeof(struct sockaddr_in);
      int fd = 0;
      struct Evnt *tmp = NULL;
      unsigned int acpt_num = 0;
      
      done = TRUE;

      vlg_dbg3(vlg, "accept(%p)\n", scan);

      /* ignore all accept() errors -- bad_poll_flags fixes here */
      while ((fd = accept(evnt_fd(scan), (struct sockaddr *) &sa, &len)) != -1)
      {
        if (!(tmp = scan->cbs->cb_func_accept(fd,
                                              (struct sockaddr *) &sa, len)))
        {
          close(fd);
          goto next_accept;
        }
      
        ++ready; /* give a read event to this new event */
        assert(SOCKET_POLL_INDICATOR(tmp->ind)->events  == POLLIN);
        assert(SOCKET_POLL_INDICATOR(tmp->ind)->revents == POLLIN);

        if (++acpt_num >= evnt__accept_limit)
          break;
      }
      /* FIXME: apache seems to assume certain errors are really bad and we
       * should just kill the listen socket and wait to die */
      
      goto next_accept;
    }
    ASSERT(!done);
    if (evnt_epoll_enabled()) break;
    
   next_accept:
    SOCKET_POLL_INDICATOR(scan->ind)->revents = 0;
    if (done)
      --ready;

    scan = scan_next;
  }  

  scan = q_recv;
  while (scan && ready)
  {
    struct Evnt *scan_next = scan->next;
    int done = FALSE;
    
    if (scan->flag_q_closed)
      goto next_recv;
    
    assert(!(SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT));
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
    {
      done = TRUE;
      if (!scan->cbs->cb_func_recv(scan))
        evnt__close_now(scan);
    }

    if (!done && (SOCKET_POLL_INDICATOR(scan->ind)->revents & bad_poll_flags))
    {
      done = TRUE;
      if (scan->io_r_shutdown || scan->io_w_shutdown ||
          !scan->cbs->cb_func_shutdown_r(scan))
        evnt__close_now(scan);
    }

    if (!done && evnt_epoll_enabled()) break;

   next_recv:
    if (done)
      --ready;
    
    scan = scan_next;
  }

  scan = q_send_recv;
  while (scan && ready)
  {
    struct Evnt *scan_next = scan->next;
    int done = FALSE;
    
    if (scan->flag_q_closed)
      goto next_send;

    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLIN)
    {
      done = TRUE;
      if (!scan->cbs->cb_func_recv(scan))
      {
        evnt__close_now(scan);
        goto next_send;
      }
    }
    
    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & POLLOUT)
    {
      done = TRUE; /* need groups so we can do direct send here */
      if (!evnt_send_add(scan, TRUE, max_sz))
        evnt__close_now(scan);
    }

    if (!done && (SOCKET_POLL_INDICATOR(scan->ind)->revents & bad_poll_flags))
    {
      done = TRUE;
      if (scan->io_r_shutdown || !scan->cbs->cb_func_shutdown_r(scan))
        evnt__close_now(scan);
    }

    if (!done && evnt_epoll_enabled()) break;

   next_send:
    if (done)
      --ready;

    scan = scan_next;
  }

  scan = q_none;
  while (scan && ready)
  {
    struct Evnt *scan_next = scan->next;
    int done = FALSE;

    if (scan->flag_q_closed)
      goto next_none;

    if (SOCKET_POLL_INDICATOR(scan->ind)->revents & (bad_poll_flags | POLLIN))
    { /* POLLIN == EOF ? */
      /* FIXME: failure cb */
      done = TRUE;

      evnt__close_now(scan);
      goto next_none;
    }
    else
      assert(!SOCKET_POLL_INDICATOR(scan->ind)->revents);

    ASSERT(!done);
    if (evnt_epoll_enabled()) break;
    
   next_none:
    if (done)
      --ready;

    scan = scan_next;
  }

  if (q_closed)
    evnt__scan_q_close();
  else if (ready)
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
      evnt__close_now(scan);
      
    scan = scan_s_next;
  }

  if (q_closed)
    evnt__scan_q_close();
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

void evnt_stats_add(struct Evnt *dst, const struct Evnt *src)
{
  dst->acct.req_put += src->acct.req_put;
  dst->acct.req_put += src->acct.req_got;

  dst->acct.bytes_r += src->acct.bytes_r;
  dst->acct.bytes_w += src->acct.bytes_w;
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

struct Evnt *evnt_find_least_used(void)
{
  struct Evnt *con     = NULL;
  struct Evnt *con_min = NULL;

  /*  Find a usable connection, tries to find the least used connection
   * preferring ones not blocking on send IO */
  if (!(con = q_none) &&
      !(con = q_recv) &&
      !(con = q_send_recv))
    return (NULL);
  
  /* FIXME: not optimal, only want to change after a certain level */
  con_min = con;
  while (con)
  {
    if (con_min->io_w->len > con->io_w->len)
      con_min = con;
    con = con->next;
  }

  return (con_min);
}

#define MATCH_Q_NAME(x)                         \
    if (CSTREQ(qname, #x ))                     \
      return ( q_ ## x )                        \

struct Evnt *evnt_queue(const char *qname)
{
  MATCH_Q_NAME(connect);
  MATCH_Q_NAME(accept);
  MATCH_Q_NAME(send_recv);
  MATCH_Q_NAME(recv);
  MATCH_Q_NAME(none);
  MATCH_Q_NAME(send_now);

  return (NULL);
}

#ifdef VSTR_AUTOCONF_HAVE_TCP_CORK
# define USE_TCP_CORK 1
#else
# define USE_TCP_CORK 0
# define TCP_CORK 0
#endif

void evnt_fd_set_cork(struct Evnt *evnt, int val)
{ /* assume it can't work for set and fail for unset */
  ASSERT(evnt__valid(evnt));

  if (!USE_TCP_CORK)
    return;
  
  if (!evnt->flag_io_cork == !val)
    return;

  if (!evnt->flag_io_nagle) /* flags can't be combined ... stupid */
  {
    evnt_fd__set_nodelay(evnt_fd(evnt), FALSE);
    evnt->flag_io_nagle = TRUE;
  }
  
  if (setsockopt(evnt_fd(evnt), IPPROTO_TCP, TCP_CORK, &val, sizeof(val)) == -1)
    return;
  
  evnt->flag_io_cork = !!val;
}

static void evnt__free_base_noerrno(Vstr_base *s1)
{
  int saved_errno = errno;
  vstr_free_base(s1);
  errno = saved_errno;
}

int evnt_fd_set_filter(struct Evnt *evnt, const char *fname)
{
  int fd = evnt_fd(evnt);
  Vstr_base *s1 = NULL;
  unsigned int ern = 0;

  if (!CONF_USE_SOCKET_FILTERS)
    return (TRUE);
  
  if (!(s1 = vstr_make_base(NULL)))
    VLG_WARNNOMEM_RET(FALSE, (vlg, "filter_attach0: %m\n"));

  vstr_sc_read_len_file(s1, 0, fname, 0, 0, &ern);

  if (ern &&
      ((ern != VSTR_TYPE_SC_READ_FILE_ERR_OPEN_ERRNO) || (errno != ENOENT)))
  {
    evnt__free_base_noerrno(s1);
    VLG_WARN_RET(FALSE, (vlg, "filter_attach1(%s): %m\n", fname));
  }
  else if ((s1->len / sizeof(struct sock_filter)) > USHRT_MAX)
  {
    vstr_free_base(s1);  
    errno = E2BIG;
    VLG_WARN_RET(FALSE, (vlg, "filter_attach2(%s): %m\n", fname));
  }
  else if (!s1->len)
  {
    if (!evnt->flag_io_filter)
      vlg_warn(vlg, "filter_attach3(%s): Empty file\n", fname);
    else if (setsockopt(fd, SOL_SOCKET, SO_DETACH_FILTER, NULL, 0) == -1)
    {
      evnt__free_base_noerrno(s1);
      VLG_WARN_RET(FALSE, (vlg, "setsockopt(SOCKET, DETACH_FILTER, %s): %m\n",
                           fname));
    }
    
    evnt->flag_io_filter = FALSE;
    return (TRUE);
  }
  else
  {
    struct sock_fprog filter[1];
    socklen_t len = sizeof(filter);

    filter->len    = s1->len / sizeof(struct sock_filter);
    filter->filter = (void *)vstr_export_cstr_ptr(s1, 1, s1->len);
    
    if (!filter->filter)
      VLG_WARNNOMEM_RET(FALSE, (vlg, "filter_attach4: %m\n"));
  
    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, filter, len) == -1)
    {
      evnt__free_base_noerrno(s1);
      VLG_WARN_RET(FALSE, (vlg,  "setsockopt(SOCKET, ATTACH_FILTER, %s): %m\n",
                           fname));
    }
  }

  evnt->flag_io_filter = TRUE;
  
  vstr_free_base(s1);

  return (TRUE);
}

static Timer_q_base *evnt__timeout_1   = NULL;
static Timer_q_base *evnt__timeout_10  = NULL;
static Timer_q_base *evnt__timeout_100 = NULL;

static Timer_q_node *evnt__timeout_mtime_make(struct Evnt *evnt,
                                              struct timeval *tv,
                                              unsigned long msecs)
{
  Timer_q_node *tm_o = NULL;
  
  if (0) { }
  else if (msecs >= ( 99 * 1000))
  {
    TIMER_Q_TIMEVAL_ADD_SECS(tv, 100, 0);
    tm_o = timer_q_add_node(evnt__timeout_100, evnt, tv,
                            TIMER_Q_FLAG_NODE_DEFAULT);
  }
  else if (msecs >= (  9 * 1000))
  {
    TIMER_Q_TIMEVAL_ADD_SECS(tv,  10, 0);
    tm_o = timer_q_add_node(evnt__timeout_10,  evnt, tv,
                            TIMER_Q_FLAG_NODE_DEFAULT);
  }
  else
  {
    TIMER_Q_TIMEVAL_ADD_SECS(tv,   1, 0);
    tm_o = timer_q_add_node(evnt__timeout_1,   evnt, tv,
                            TIMER_Q_FLAG_NODE_DEFAULT);
  }

  return (tm_o);
}

static void evnt__timer_cb_mtime(int type, void *data)
{
  struct Evnt *evnt = data;
  struct timeval tv[1];
  unsigned long diff = 0;
  
  if (type == TIMER_Q_TYPE_CALL_RUN_ALL)
    return;

  evnt->tm_o = NULL;
  
  if (type == TIMER_Q_TYPE_CALL_DEL)
    return;
  
  gettimeofday(tv, NULL);

  /* find out time elapsed */
  diff = timer_q_timeval_udiff_secs(tv, &evnt->mtime);
  if (diff >= evnt->msecs_tm_mtime)
  {
    vlg_dbg2(vlg, "timeout = %p[$<sa:%p>] (%lu, %lu)\n",
             evnt, evnt->sa, diff, evnt->msecs_tm_mtime);
    evnt_close(evnt);
    return;
  }
  
  diff = evnt->msecs_tm_mtime - diff; /* seconds left until timeout */
  if (!(evnt->tm_o = evnt__timeout_mtime_make(evnt, tv, diff)))
  {
    errno = ENOMEM, vlg_warn(vlg, "%s: %m\n", "timer reinit");
    evnt_close(evnt);
  }
}

void evnt_timeout_init(void)
{
  ASSERT(!evnt__timeout_1);
  
  evnt__timeout_1   = timer_q_add_base(evnt__timer_cb_mtime,
                                       TIMER_Q_FLAG_BASE_DEFAULT);
  evnt__timeout_10  = timer_q_add_base(evnt__timer_cb_mtime,
                                       TIMER_Q_FLAG_BASE_DEFAULT);
  evnt__timeout_100 = timer_q_add_base(evnt__timer_cb_mtime,
                                       TIMER_Q_FLAG_BASE_DEFAULT);

  if (!evnt__timeout_1 || !evnt__timeout_10 || !evnt__timeout_100)
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "timer init"));

  /* FIXME: massive hack 1.0.5 is broken */
  timer_q_cntl_base(evnt__timeout_1,
                    TIMER_Q_CNTL_BASE_SET_FLAG_INSERT_FROM_END, FALSE);
  timer_q_cntl_base(evnt__timeout_10,
                    TIMER_Q_CNTL_BASE_SET_FLAG_INSERT_FROM_END, FALSE);
  timer_q_cntl_base(evnt__timeout_100,
                    TIMER_Q_CNTL_BASE_SET_FLAG_INSERT_FROM_END, FALSE);
}

void evnt_timeout_exit(void)
{
  ASSERT(evnt__timeout_1);
  
  timer_q_del_base(evnt__timeout_1);   evnt__timeout_1   = NULL;
  timer_q_del_base(evnt__timeout_10);  evnt__timeout_10  = NULL;
  timer_q_del_base(evnt__timeout_100); evnt__timeout_100 = NULL;
}

int evnt_sc_timeout_via_mtime(struct Evnt *evnt, unsigned int msecs)
{
  struct timeval tv[1];

  evnt->msecs_tm_mtime = msecs;
  
  gettimeofday(tv, NULL);
  
  if (!(evnt->tm_o = evnt__timeout_mtime_make(evnt, tv, msecs)))
    return (FALSE);

  return (TRUE);
}

void evnt_vlg_stats_info(struct Evnt *evnt, const char *prefix)
{
  vlg_info(vlg, "%s from[$<sa:%p>] req_got[%'u:%u] req_put[%'u:%u]"
           " recv[${BKMG.ju:%ju}:%ju] send[${BKMG.ju:%ju}:%ju]\n",
           prefix, evnt->sa,
           evnt->acct.req_got, evnt->acct.req_got,
           evnt->acct.req_put, evnt->acct.req_put,
           evnt->acct.bytes_r, evnt->acct.bytes_r,
           evnt->acct.bytes_w, evnt->acct.bytes_w);
}

#ifdef TCP_DEFER_ACCEPT
# define USE_TCP_DEFER_ACCEPT 1
#else
# define USE_TCP_DEFER_ACCEPT 0
# define TCP_DEFER_ACCEPT 0
#endif

void evnt_fd_set_defer_accept(struct Evnt *evnt, int val)
{
  socklen_t len = sizeof(int);

  ASSERT(evnt__valid(evnt));

  if (!USE_TCP_DEFER_ACCEPT)
    return;
  
  /* ignore return val */
  setsockopt(evnt_fd(evnt), IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, len);
}

#ifndef  EVNT_USE_EPOLL
# ifdef  VSTR_AUTOCONF_HAVE_SYS_EPOLL_H
#  define EVNT_USE_EPOLL 1
# else
#  define EVNT_USE_EPOLL 0
# endif
#endif

#if !EVNT_USE_EPOLL
int evnt_epoll_init(void)
{
  return (FALSE);
}

int evnt_epoll_enabled(void)
{
  return (FALSE);
}

void evnt_wait_cntl_add(struct Evnt *evnt, int flags)
{
  SOCKET_POLL_INDICATOR(evnt->ind)->events  |= flags;
  SOCKET_POLL_INDICATOR(evnt->ind)->revents |= flags;  
}

void evnt_wait_cntl_del(struct Evnt *evnt, int flags)
{
  SOCKET_POLL_INDICATOR(evnt->ind)->events  &= ~flags;
  SOCKET_POLL_INDICATOR(evnt->ind)->revents &= ~flags;  
}

unsigned int evnt_poll_add(struct Evnt *EVNT__ATTR_UNUSED(evnt), int fd)
{
  return (socket_poll_add(fd));
}

void evnt_poll_del(struct Evnt *evnt)
{
  socket_poll_del(evnt->ind);
}

int evnt_poll_swap_accept_read(struct Evnt *evnt, int fd)
{
  int old_fd = SOCKET_POLL_INDICATOR(evnt->ind)->fd;

  assert(old_fd != fd);
  
  ASSERT(evnt__valid(evnt));
  
  SOCKET_POLL_INDICATOR(evnt->ind)->fd = fd;

  if (!(evnt->io_r = vstr_make_base(NULL)) ||
      !(evnt->io_w = vstr_make_base(NULL)))
    return (FALSE);

  SOCKET_POLL_INDICATOR(evnt->ind)->events |= POLLIN;
  
  evnt_del(&q_accept, evnt); evnt->flag_q_accept = FALSE;
  evnt_add(&q_recv, evnt);   evnt->flag_q_recv   = TRUE;

  ASSERT(evnt__valid(evnt));

  return (TRUE);
}

int evnt_poll(void)
{
  int msecs = evnt__get_timeout();

  return (socket_poll_update_all(msecs));
}
#else

#include <sys/epoll.h>

static int evnt__epoll_fd = -1;

int evnt_epoll_init(void)
{
  assert(POLLIN  == EPOLLIN);
  assert(POLLOUT == EPOLLOUT);
  assert(POLLHUP == EPOLLHUP);
  assert(POLLERR == EPOLLERR);

  if (!CONF_EVNT_NO_EPOLL)
  {
    evnt__epoll_fd = epoll_create(1); /* size does nothing */
  
    vlg_dbg2(vlg, "epoll_create(%d): %m\n", evnt__epoll_fd);
  }
  
  return (evnt__epoll_fd != -1);
}

int evnt_epoll_enabled(void)
{
  return (evnt__epoll_fd != -1);
}

void evnt_wait_cntl_add(struct Evnt *evnt, int flags)
{
  if ((SOCKET_POLL_INDICATOR(evnt->ind)->events & flags) == flags)
    return;
  
  SOCKET_POLL_INDICATOR(evnt->ind)->events  |=  flags;
  SOCKET_POLL_INDICATOR(evnt->ind)->revents |= (flags & POLLIN);
  
  if (evnt_epoll_enabled())
  {
    struct epoll_event epevent[1];
    
    flags = SOCKET_POLL_INDICATOR(evnt->ind)->events;    
    vlg_dbg3(vlg, "epoll_mod_add(%p,%d)\n", evnt, flags);
    epevent->events   = flags;
    epevent->data.ptr = evnt;
    if (epoll_ctl(evnt__epoll_fd, EPOLL_CTL_MOD, evnt_fd(evnt), epevent) == -1)
      vlg_err(vlg, EXIT_FAILURE, "epoll: %m\n");
  }
}

void evnt_wait_cntl_del(struct Evnt *evnt, int flags)
{
  if (!(SOCKET_POLL_INDICATOR(evnt->ind)->events & flags))
    return;
  
  SOCKET_POLL_INDICATOR(evnt->ind)->events  &= ~flags;
  SOCKET_POLL_INDICATOR(evnt->ind)->revents &= ~flags;
  
  if (flags && evnt_epoll_enabled())
  {
    struct epoll_event epevent[1];
    
    flags = SOCKET_POLL_INDICATOR(evnt->ind)->events;
    vlg_dbg3(vlg, "epoll_mod_del(%p,%d)\n", evnt, flags);
    epevent->events   = flags;
    epevent->data.ptr = evnt;
    if (epoll_ctl(evnt__epoll_fd, EPOLL_CTL_MOD, evnt_fd(evnt), epevent) == -1)
      vlg_err(vlg, EXIT_FAILURE, "epoll: %m\n");
  }
}

unsigned int evnt_poll_add(struct Evnt *evnt, int fd)
{
  unsigned int ind = socket_poll_add(fd);

  if (ind && evnt_epoll_enabled())
  {
    struct epoll_event epevent[1];
    int flags = 0;

    vlg_dbg3(vlg, "epoll_add(%p,%d)\n", evnt, flags);
    epevent->events   = flags;
    epevent->data.ptr = evnt;
    
    if (epoll_ctl(evnt__epoll_fd, EPOLL_CTL_ADD, fd, epevent) == -1)
    {
      vlg_warn(vlg, "epoll: %m\n");
      socket_poll_del(fd);
    }  
  }
  
  return (ind);
}

void evnt_poll_del(struct Evnt *evnt)
{
  socket_poll_del(evnt->ind);

  /* done via. the close() */
  if (FALSE && evnt_epoll_enabled())
  {
    int fd = SOCKET_POLL_INDICATOR(evnt->ind)->fd;
    struct epoll_event epevent[1];

    vlg_dbg3(vlg, "epoll_del(%p,%d)\n", evnt, 0);
    epevent->events   = 0;
    epevent->data.ptr = evnt;
    
    if (epoll_ctl(evnt__epoll_fd, EPOLL_CTL_DEL, fd, epevent) == -1)
      vlg_abort(vlg, "epoll: %m\n");
  }
}

int evnt_poll_swap_accept_read(struct Evnt *evnt, int fd)
{
  int old_fd = SOCKET_POLL_INDICATOR(evnt->ind)->fd;

  assert(old_fd != fd);
  
  ASSERT(evnt__valid(evnt));
  
  SOCKET_POLL_INDICATOR(evnt->ind)->fd = fd;

  if (!(evnt->io_r = vstr_make_base(NULL)) ||
      !(evnt->io_w = vstr_make_base(NULL)))
    return (FALSE);

  if (evnt_epoll_enabled())
  {
    struct epoll_event epevent[1];

    vlg_dbg3(vlg, "epoll_swap(%p,%d)\n", evnt, fd);
    
    epevent->events   = SOCKET_POLL_INDICATOR(evnt->ind)->events;
    epevent->data.ptr = evnt;
    
    if (epoll_ctl(evnt__epoll_fd, EPOLL_CTL_ADD, fd, epevent) == -1)
    {
      vlg_warn(vlg, "epoll: %m\n");
      return (FALSE);
    }
    
    epevent->events   = POLLIN;
    epevent->data.ptr = evnt;
    
    if (epoll_ctl(evnt__epoll_fd, EPOLL_CTL_DEL, old_fd, epevent) == -1)
      vlg_abort(vlg, "epoll: %m\n");
  }

  SOCKET_POLL_INDICATOR(evnt->ind)->events |= POLLIN;

  evnt_del(&q_accept, evnt); evnt->flag_q_accept = FALSE;
  evnt_add(&q_recv, evnt);   evnt->flag_q_recv   = TRUE;

  ASSERT(evnt__valid(evnt));
  
  return (TRUE);
}

#define EVNT__EPOLL_EVENTS 128
int evnt_poll(void)
{
  struct epoll_event events[EVNT__EPOLL_EVENTS];
  int msecs = evnt__get_timeout();
  int ret = -1;
  unsigned int scan = 0;
  
  if (!evnt_epoll_enabled())
    return (socket_poll_update_all(msecs));

  ret = epoll_wait(evnt__epoll_fd, events, EVNT__EPOLL_EVENTS, msecs);
  if (ret == -1)
    return (ret);

  scan = ret;
  ASSERT(scan <= EVNT__EPOLL_EVENTS);
  while (scan-- > 0)
  {
    struct Evnt *evnt  = NULL;
    unsigned int flags = 0;
    
    evnt  = events[scan].data.ptr;
    flags = events[scan].events;

    ASSERT(evnt__valid(evnt));
    
    vlg_dbg3(vlg, "epoll_wait(%p,%d)\n", evnt, flags);
    vlg_dbg3(vlg, "epoll_wait[flags]=a=%d|r=%d\n",
             evnt->flag_q_accept, evnt->flag_q_recv);

    assert(((SOCKET_POLL_INDICATOR(evnt->ind)->events & flags) == flags) ||
           ((POLLHUP|POLLERR) & flags));
    
    SOCKET_POLL_INDICATOR(evnt->ind)->revents = flags;
    
    evnt__del_whatever(evnt);
    evnt__add_whatever(evnt);
  }

  return (ret);
}
#endif
