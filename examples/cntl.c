
#include "cntl.h"
#include <unistd.h>

#include <poll.h>
#include <socket_poll.h>

#define TRUE 1
#define FALSE 0

#include "vlg_assert.h"

static Vlg *vlg = NULL;
static struct Evnt *acpt_evnt = NULL;
static struct Evnt *acpt_cntl_evnt = NULL;

static void cntl__fin(Vstr_base *out)
{
  size_t ns1 = 0;
  size_t ns2 = 0;

  if (!(ns1 = vstr_add_netstr_beg(out, out->len)))
    return;
  if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
    return;
  vstr_add_netstr_end(out, ns2, out->len);
  vstr_add_netstr_end(out, ns1, out->len);
}

static void cntl__close(Vstr_base *out)
{
  size_t ns1 = 0;
  size_t ns2 = 0;

  if (!(ns1 = vstr_add_netstr_beg(out, out->len)))
    return;

  if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
    return;
  vstr_add_cstr_ptr(out, out->len, "CLOSE");
  vstr_add_netstr_end(out, ns2, out->len);
    
  if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
    return;
  vstr_add_fmt(out, out->len, "$<sa:%p>", acpt_evnt->sa);
  vstr_add_netstr_end(out, ns2, out->len);
    
  vstr_add_netstr_end(out, ns1, out->len);

  vlg_dbg3(vlg, "evnt_close acpt %p\n", acpt_evnt);
  evnt_close(acpt_evnt);
  vlg_dbg3(vlg, "evnt_close acpt cntl %p\n", acpt_cntl_evnt);
  evnt_close(acpt_cntl_evnt);
}

static void cntl__scan_events(Vstr_base *out, const char *tag, struct Evnt *beg)
{
  struct Evnt *ev = beg;
  
  while (ev)
  {
    size_t ns1 = 0;
    size_t ns2 = 0;

    if (!(ns1 = vstr_add_netstr_beg(out, out->len)))
      return;

    if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
      return;
    vstr_add_fmt(out, out->len, "EVNT %s", tag);
    vstr_add_netstr_end(out, ns2, out->len);
    
    if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
      return;
    vstr_add_fmt(out, out->len, "from[$<sa:%p>]", ev->sa);
    vstr_add_netstr_end(out, ns2, out->len);

    if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
      return;
    vstr_add_fmt(out, out->len, "recv[${BKMG.ju:%ju}]", ev->acct.bytes_r);
    vstr_add_netstr_end(out, ns2, out->len);

    if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
      return;
    vstr_add_fmt(out, out->len, "send[${BKMG.ju:%ju}]", ev->acct.bytes_w);
    vstr_add_netstr_end(out, ns2, out->len);
    
    vstr_add_netstr_end(out, ns1, out->len);
    
    ev = ev->next;
  }
}

static void cntl__status(Vstr_base *out)
{
  size_t ns1 = 0;
  size_t ns2 = 0;

  if (!(ns1 = vstr_add_netstr_beg(out, out->len)))
    return;

  if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
    return;
  vstr_add_cstr_ptr(out, out->len, "PID");
  vstr_add_netstr_end(out, ns2, out->len);
    
  if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
    return;
  vstr_add_fmt(out, out->len, "%lu", (unsigned long)getpid());
  vstr_add_netstr_end(out, ns2, out->len);

  if (acpt_evnt)
  {
    if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
      return;
    vstr_add_cstr_ptr(out, out->len, "ADDR");
    vstr_add_netstr_end(out, ns2, out->len);
    
    if (!(ns2 = vstr_add_netstr_beg(out, out->len)))
      return;
    vstr_add_fmt(out, out->len, "$<sa:%p>", acpt_evnt->sa);
    vstr_add_netstr_end(out, ns2, out->len);
  }
  
  vstr_add_netstr_end(out, ns1, out->len);
}

static void cntl__cb_func_free(struct Evnt *evnt)
{
  vlg_info(vlg, "CNTL FREE from[$<sa:%p>]"
           " recv[${BKMG.ju:%ju}] send[${BKMG.ju:%ju}]\n", evnt->sa,
           evnt->acct.bytes_r, evnt->acct.bytes_w);

  free(evnt);
}

static int cntl__cb_func_recv(struct Evnt *evnt)
{
  int ret = evnt_cb_func_recv(evnt);

  if (!ret)
    goto malloc_bad;

  vlg_dbg3(vlg, "CNTL recv %zu\n", evnt->io_r->len);

  while (evnt->io_r->len)
  {
    size_t pos = 0;
    size_t len = 0;
    size_t ns1 = 0;
    
    if (!(ns1 = vstr_parse_netstr(evnt->io_r, 1, evnt->io_r->len, &pos, &len)))
    {
      if (!(SOCKET_POLL_INDICATOR(evnt->ind)->events & POLLIN))
        return (FALSE);
      return (TRUE);
    }

    if (0){ }
    else if (vstr_cmp_cstr_eq(evnt->io_r, pos, len, "CLOSE"))
    {
      if (acpt_evnt)
        cntl__close(evnt->io_w);
      cntl__fin(evnt->io_w);
    }
    else if (vstr_cmp_cstr_eq(evnt->io_r, pos, len, "LIST"))
    {
      cntl__scan_events(evnt->io_w, "CONNECT",   evnt_queue("connect"));
      cntl__scan_events(evnt->io_w, "ACCEPT",    evnt_queue("accept"));
      cntl__scan_events(evnt->io_w, "SEND/RECV", evnt_queue("send_recv"));
      cntl__scan_events(evnt->io_w, "RECV",      evnt_queue("recv"));
      cntl__scan_events(evnt->io_w, "NONE",      evnt_queue("none"));
      cntl__scan_events(evnt->io_w, "SEND_NOW",  evnt_queue("send_now"));
      cntl__fin(evnt->io_w);
    }
    else if (vstr_cmp_cstr_eq(evnt->io_r, pos, len, "STATUS"))
    {
      cntl__status(evnt->io_w);
      cntl__fin(evnt->io_w);
    }
    else
      return (FALSE);
  
    if (evnt->io_w->conf->malloc_bad)
      goto malloc_bad;
    
    if (!evnt_send_add(evnt, FALSE, 32))
      return (FALSE);
  
    vstr_del(evnt->io_r, 1, ns1);
  }
  
  return (TRUE);
  
 malloc_bad:
  evnt->io_r->conf->malloc_bad = FALSE;
  evnt->io_w->conf->malloc_bad = FALSE;
  return (FALSE);
}


static void cntl__cb_func_acpt_free(struct Evnt *evnt)
{
  vlg_dbg3(vlg, "cntl acpt free %p %p\n", evnt, acpt_cntl_evnt);
  
  ASSERT(acpt_cntl_evnt == evnt);
  
  free(evnt);

  acpt_cntl_evnt = NULL;
}

static struct Evnt *cntl__cb_func_accept(int fd,
                                         struct sockaddr *sa, socklen_t len)
{
  struct Evnt *evnt = NULL;

  ASSERT(acpt_cntl_evnt);
  
  if (sa->sa_family != AF_LOCAL)
    goto make_acpt_fail;
  
  if (!(evnt = malloc(sizeof(struct Evnt))) ||
      !evnt_make_acpt(evnt, fd, sa, len))
    goto make_acpt_fail;

  vlg_info(vlg, "CNTL CONNECT from[$<sa:%p>]\n", evnt->sa);

  evnt->cbs->cb_func_recv = cntl__cb_func_recv;
  evnt->cbs->cb_func_free = cntl__cb_func_free;

  return (evnt);

 make_acpt_fail:
  free(evnt);
  VLG_WARNNOMEM_RET(NULL, (vlg, "%s: %m\n", "accept"));

  return (NULL);
}

void cntl_init(Vlg *passed_vlg, const char *fname,
               struct Evnt *passed_acpt_evnt)
{
  struct Evnt *evnt = NULL;

  ASSERT(!vlg && passed_vlg);

  vlg = passed_vlg;
  
  ASSERT(fname);
  ASSERT(!acpt_evnt && passed_acpt_evnt);
  ASSERT(!acpt_cntl_evnt);
  
  acpt_evnt = passed_acpt_evnt;
    
  if (!(evnt = malloc(sizeof(struct Evnt))))
    VLG_ERRNOMEM((vlg, EXIT_FAILURE, "cntl file: %m\n"));
  
  if (!evnt_make_bind_local(evnt, fname))
    vlg_err(vlg, EXIT_FAILURE, "cntl file: %m\n");
  
  evnt->cbs->cb_func_accept = cntl__cb_func_accept;
  evnt->cbs->cb_func_free   = cntl__cb_func_acpt_free;

  acpt_cntl_evnt = evnt;
}

void cntl_free_acpt(struct Evnt *evnt)
{
  ASSERT(acpt_evnt == evnt);

  acpt_evnt = NULL;
}

static void cntl__cb_func_pipe_acpt_free(struct Evnt *evnt)
{
  vlg_dbg3(vlg, "cntl pipe acpt free %p %p\n", evnt, acpt_cntl_evnt);
  
  ASSERT(acpt_cntl_evnt == evnt);
  
  free(evnt);

  acpt_cntl_evnt = NULL;

  if (acpt_evnt)
    evnt_close(acpt_evnt);
}

/* only used to get death sig atm. */
void cntl_pipe_acpt_fds(int fd_r, int fd_w)
{
  ASSERT(fd_w == -1);
  
  if (acpt_evnt)
  {
    int old_fd = SOCKET_POLL_INDICATOR(acpt_cntl_evnt->ind)->fd;
    
    ASSERT(acpt_cntl_evnt);
    
    if (!evnt_poll_swap(acpt_cntl_evnt, fd_r))
      vlg_abort(vlg, "swap_acpt: %m\n");

    close(old_fd);
    
    acpt_cntl_evnt->cbs->cb_func_free   = cntl__cb_func_pipe_acpt_free;
  }
}
