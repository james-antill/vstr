#ifndef OPT_H
#define OPT_H

#include <getopt.h>
#include <string.h>

#define OPT_TOGGLE_ARG(val) (val = opt_toggle(val, optarg))

extern int opt_toggle(int, const char *);

/* get program name ... but ignore "lt-" libtool prefix */
extern const char *opt_program_name(const char *, const char *);

extern const char *opt_def_toggle(int);


#endif
