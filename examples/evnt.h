#ifndef EVENT_H
#define EVENT_H

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
 
 Vstr_base *io_r;
 Vstr_base *io_w;

 struct Evnt *s_next;
 
 Timer_q_node *tm_o;

 struct timeval ctime;
 struct timeval mtime;
 
 unsigned int req_put;
 unsigned int req_got;
 
 unsigned int flag_q_send_recv : 1;
 unsigned int flag_q_send_now  : 1;
 unsigned int flag_q_none      : 1;
};


extern void evnt_fd_set_nonblock(int, int);

extern int evnt_make_con_ipv4(struct Evnt *, const char *, short);
extern int evnt_make_bind_ipv4(struct Evnt *, const char *, short);
extern int evnt_make_acpt(struct Evnt *, int, struct sockaddr *, socklen_t);

extern void evnt_free(struct Evnt *);
extern void evnt_close(struct Evnt *);
extern void evnt_close_all(void);

extern void evnt_add(struct Evnt **, struct Evnt *);
extern void evnt_del(struct Evnt **, struct Evnt *);
extern void evnt_put_pkt(struct Evnt *);
extern void evnt_got_pkt(struct Evnt *);
extern int evnt_recv(struct Evnt *);
extern int evnt_send(struct Evnt *);
extern int  evnt_send_add(struct Evnt *, int, size_t);
extern void evnt_send_del(struct Evnt *);
extern int evnt_poll(void);
extern void evnt_scan_fds(unsigned int, size_t);
extern void evnt_scan_send_fds(void);

extern int evnt_opt_nagle;

extern struct Evnt *q_send_now;  /* Try a send "now" */

extern struct Evnt *q_none;      /* nothing */
extern struct Evnt *q_accept;    /* connections - recv */
extern struct Evnt *q_connect;   /* connections - send */
extern struct Evnt *q_recv;      /* recv */
extern struct Evnt *q_send_recv; /* recv + send */

#endif