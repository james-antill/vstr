#ifndef CNTL_H
#define CNTL_H

#include "evnt.h"

extern void cntl_init(Vlg *, const char *, struct Evnt *);
extern void cntl_free_acpt(struct Evnt *);
extern void cntl_pipe_acpt_fds(Vlg *, int, int, struct Evnt *);

#endif
