#ifndef MAIN_SYSTEM_H
#define MAIN_SYSTEM_H

#undef _GNU_SOURCE
#define _GNU_SOURCE 1

#define USE_ASSERT_LOOP 1

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <wchar.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
# warning "Don't have unistd.h ... Errm lets carry on, see what happens."
#endif
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <setjmp.h>
#include <dirent.h>
#include <locale.h>
#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#endif
#include <stdarg.h>
#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#else
# ifdef USE_DL_LOAD
#  undef USE_DL_LOAD
# endif
#endif
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/resource.h>

#ifdef HAVE_POLL
# include <sys/poll.h>
#endif

#ifdef HAVE_SENDFILE
# include <sys/sendfile.h>
#endif

#ifdef HAVE_VFORK_H
# include <vfork.h>
#endif

#ifdef HAVE_WRITEV
# include <sys/uio.h>
#else
# ifdef HAVE_SYS_UIO_H
#  warning "Detected uio.h, but haven't used writev as it might not work"
# endif
#endif

#ifdef USE_MMAP
# ifdef HAVE_MMAP
#  ifdef HAVE_SYS_MMAN_H
#   include <sys/mman.h>
#  else
#   warning "Using mmap, but have no <sys/mman.h>"
#  endif
# else
#  warning "Not using, even though you asked."
#  undef USE_MMAP
# endif
#endif

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>
#include <arpa/telnet.h>

#ifndef TELOPT_COMPRESS
# define TELOPT_COMPRESS 85
#endif

#ifdef HAVE_ZLIB_H
# include <zlib.h>
#endif

#ifdef HAVE_TERMCAP_H
# include <termcap.h>
#endif

#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif

/* useful */
#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#endif
