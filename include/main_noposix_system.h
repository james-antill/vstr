#ifndef MAIN_NOPOSIX_SYSTEM_H
#define MAIN_NOPOSIX_SYSTEM_H

#undef _GNU_SOURCE
#define _GNU_SOURCE 1

#define USE_ASSERT_LOOP 1

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#if STDC_HEADERS
# include <string.h>
#else
/* this probably needs more stuff in it -- stdarg.h ? */
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <wchar.h>

#include <locale.h>

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

struct iovec
{
 void *iov_base;
 size_t iov_len;
}; /* normally part of <sys/uio.h> ... but that isn't here now */

#if !defined(VSTR_AUTOCONF_intmax_t) || !defined(VSTR_AUTOCONF_uintmax_t)
# include <inttypes.h>
#endif

/* useful */
#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#endif