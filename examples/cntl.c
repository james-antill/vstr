
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

static void cntl__ns_out_fmt(Vstr_base *out, const char *fmt, ...)
   VSTR__COMPILE_ATTR_FMT(2, 3);
static void cntl__ns_out_fmt(Vstr_base *out, const char *fmt, ...)
{
  va_list ap;
  size_t ns = 0;

  if (!(ns = vstr_add_netstr_beg(out, out->len)))
    return;

  va_start(ap, fmt);
  vstr_add_vfmt(out, out->len, fmt, ap);
  va_end(ap);

  vstr_add_netstr_end(out, ns, out->len);
}

static void cntl__scan_events(Vstr_base *out, const char *tag, struct Evnt *beg)
{
  struct Evnt *ev = beg;
  
  while (ev)
  {
    size_t ns = 0;

    if (!(ns = vstr_add_netstr_beg(out, out->len)))
      return;

    cntl__ns_out_fmt(out, "EVNT %s", tag);
    cntl__ns_out_fmt(out, "from[$<sa:%p>]", ev->sa);
    cntl__ns_out_fmt(out, "req_got[%'u:%u]",
                     ev->acct.req_got, ev->acct.req_got);
    cntl__ns_out_fmt(out, "req_put[%'u:%u]",
                     ev->acct.req_put, ev->acct.req_put);
    cntl__ns_out_fmt(out, "recv[${BKMG.ju:%ju}:%ju]",
                     ev->acct.bytes_r, ev->acct.bytes_r);
    cntl__ns_out_fmt(out, "send[${BKMG.ju:%ju}:%ju]",
                     ev->acct.bytes_w, ev->acct.bytes_w);

    vstr_add_netstr_end(out, ns, out->len);
    
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
  evnt_vlg_stats_info(evnt, "CNTL FREE");

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

    evnt_got_pkt(evnt);

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
    
    evnt_put_pkt(evnt);
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
{ /* can be NULL if we didn't use a cntl-file */
  ASSERT(!acpt_evnt || (acpt_evnt == evnt));

  acpt_evnt = NULL;
}

static void cntl__cb_func_pipe_acpt_free(struct Evnt *evnt)
{
  vlg_dbg3(vlg, "cntl pipe acpt free %p %p\n", evnt, acpt_cntl_evnt);
  
  ASSERT(acpt_cntl_evnt == evnt);
  
  acpt_cntl_evnt = NULL;

  free(evnt);

  if (acpt_evnt)
    evnt_close(acpt_evnt);
}

/* only used to get death sig atm. */
void cntl_pipe_acpt_fds(Vlg *passed_vlg, int fd_r, int fd_w,
                        struct Evnt *passed_acpt_evnt)
{
  ASSERT(fd_w == -1);
  
  if (acpt_cntl_evnt)
  {
    int old_fd = SOCKET_POLL_INDICATOR(acpt_cntl_evnt->ind)->fd;
    
    ASSERT(vlg       == passed_vlg);
    ASSERT(acpt_evnt == passed_acpt_evnt);
    
    if (!evnt_poll_swap(acpt_cntl_evnt, fd_r))
      vlg_abort(vlg, "%s: %m\n", "swap_acpt");

    close(old_fd);
    
    acpt_cntl_evnt->cbs->cb_func_free = cntl__cb_func_pipe_acpt_free;
  }
  else
  {
    ASSERT(!vlg && passed_vlg);

    vlg = passed_vlg;
  
    ASSERT(!acpt_evnt && passed_acpt_evnt);
  
    acpt_evnt = passed_acpt_evnt;
  
    if (!(acpt_cntl_evnt = malloc(sizeof(struct Evnt))))
      VLG_ERRNOMEM((vlg, EXIT_FAILURE, "%s: %m\n", "cntl file"));

    if (!evnt_make_custom(acpt_cntl_evnt, fd_r, 0, 0))
      vlg_err(vlg, EXIT_FAILURE, "%s: %m\n", "cntl file");
  
    acpt_cntl_evnt->cbs->cb_func_free = cntl__cb_func_pipe_acpt_free;
  }
}
