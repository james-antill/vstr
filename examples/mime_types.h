#ifndef MIME_TYPES_H
#define MIME_TYPES_H

#include <vstr.h>

extern int mime_types_init(const char *);
extern void mime_types_exit(void);

extern int mime_types_load_simple(const char *);

extern int mime_types_match(const Vstr_base *, size_t, size_t,
                            Vstr_base **, size_t *, size_t *);

#endif
