#ifndef EVENT_H
#define EVENT_H

#include "vlg.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <timer_q.h>

struct Evnt;

struct Evnt_cbs
{
 struct Evnt *(*cb_func_accept)(int, struct sockaddr *, socklen_t);
 int (*cb_func_connect)(struct Evnt *);
 int (*cb_func_recv)(struct Evnt *);
 int (*cb_func_send)(struct Evnt *);
 void (*cb_func_free)(struct Evnt *);
};

struct Evnt
{
 struct Evnt *next;
 struct Evnt *prev;

 struct Evnt_cbs cbs[1];
 
 unsigned int ind; /* socket poll */

 struct sockaddr *sa;
 
 Vstr_base *io_r;
 Vstr_base *io_w;

 struct Evnt *s_next;
 
 Timer_q_node *tm_o;

 struct timeval ctime;
 struct timeval mtime;

 struct
 {
  unsigned int            req_put;
  unsigned int            req_got;
  VSTR_AUTOCONF_uintmax_t bytes_r;
  VSTR_AUTOCONF_uintmax_t bytes_w;
 } acct;
 
 unsigned int flag_q_accept    : 1;
 unsigned int flag_q_connect   : 1;
 unsigned int flag_q_recv      : 1;
 unsigned int flag_q_send_recv : 1;
 unsigned int flag_q_none      : 1;
 
 unsigned int flag_q_send_now  : 1;
};

#define EVNT_SA(x)    (                      (x)->sa)
#define EVNT_SA_IN(x) ((struct sockaddr_in *)(x)->sa)
#define EVNT_SA_UN(x) ((struct sockaddr_un *)(x)->sa)

extern void evnt_logger(Vlg *);

extern void evnt_fd_set_nonblock(int, int);

extern int evnt_cb_func_connect(struct Evnt *);
extern struct Evnt *evnt_cb_func_accept(int, struct sockaddr *, socklen_t);
extern int evnt_cb_func_recv(struct Evnt *);
extern int evnt_cb_func_send(struct Evnt *);
extern void evnt_cb_func_free(struct Evnt *);

extern int evnt_make_con_ipv4(struct Evnt *, const char *, short);
extern int evnt_make_con_local(struct Evnt *, const char *);
extern int evnt_make_bind_ipv4(struct Evnt *, const char *, short);
extern int evnt_make_bind_local(struct Evnt *, const char *);
extern int evnt_make_acpt(struct Evnt *, int, struct sockaddr *, socklen_t);

extern void evnt_free(struct Evnt *);
extern void evnt_close(struct Evnt *);
extern void evnt_close_all(void);

extern void evnt_add(struct Evnt **, struct Evnt *);
extern void evnt_del(struct Evnt **, struct Evnt *);
extern void evnt_put_pkt(struct Evnt *);
extern void evnt_got_pkt(struct Evnt *);
extern int evnt_recv(struct Evnt *, unsigned int *ern);
extern int evnt_send(struct Evnt *);
extern int  evnt_send_add(struct Evnt *, int, size_t);
extern void evnt_send_del(struct Evnt *);
extern int evnt_poll(void);
extern void evnt_scan_fds(unsigned int, size_t);
extern void evnt_scan_send_fds(void);

extern unsigned int evnt_num_all(void);
extern int evnt_waiting(void);

extern void evnt_out_dbg3(const char *);

extern int evnt_opt_nagle;

extern struct Evnt *q_send_now;  /* Try a send "now" */

extern struct Evnt *q_none;      /* nothing */
extern struct Evnt *q_accept;    /* connections - recv */
extern struct Evnt *q_connect;   /* connections - send */
extern struct Evnt *q_recv;      /* recv */
extern struct Evnt *q_send_recv; /* recv + send */

#endif
