#ifndef CNTL_H
#define CNTL_H

#include "evnt.h"

extern void cntl_make_file(Vlg *, struct Evnt *, const char *);
extern void cntl_free_acpt(struct Evnt *);
extern void cntl_pipe_acpt_fds(Vlg *, struct Evnt *, int);

extern void cntl_child_make(unsigned int);
extern void cntl_child_free(void);

extern void cntl_child_pid(pid_t, int);

extern void cntl_sc_multiproc(Vlg *, struct Evnt *, unsigned int, int);

#endif
