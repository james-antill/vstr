#ifndef VSTR__HEADER_H
#define VSTR__HEADER_H

/*
 *  Copyright (C) 1999, 2000, 2001, 2002  James Antill
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  email: james@and.org
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <vstr-conf.h>
#ifndef VSTR_AUTOCONF_HAVE_POSIX_HOST
/* only undef the things we use externaly */
# undef VSTR_AUTOCONF_HAVE_MMAP
# undef VSTR_AUTOCONF_HAVE_WRITEV
# undef  VSTR_AUTOCONF_mode_t
# define VSTR_AUTOCONF_mode_t int
# undef  VSTR_AUTOCONF_off64_t
# define VSTR_AUTOCONF_off64_t long
#endif

#include <vstr-switch.h>

#if VSTR_COMPILE_INCLUDE

# ifdef VSTR_AUTOCONF_HAVE_POSIX_HOST
#  ifndef  _LARGEFILE64_SOURCE
#   define _LARGEFILE64_SOURCE 1
#  endif
# endif

# include <stdlib.h> /* size_t */
# include <stdarg.h> /* va_list */
# include <string.h> /* strlen()/memcpy()/memset() in headers */

# ifdef VSTR_AUTOCONF_HAVE_POSIX_HOST
#  include <sys/types.h>  /* mode_t off64_t/off_t */
#  include <sys/uio.h>    /* struct iovec */
# else
struct iovec /* need real definition, as it's used inline */
{
 void *iov_base;
 size_t iov_len;
};

# endif

#ifdef VSTR_AUTOCONF_HAVE_INTTYPES_H
# include <inttypes.h>
#else
# ifdef VSTR_AUTOCONF_HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#endif

#include <vstr-const.h>
#include <vstr-def.h>
#include <vstr-extern.h>

#if defined(VSTR_AUTOCONF_HAVE_INLINE) && VSTR_COMPILE_INLINE
# include <vstr-inline.h>
#endif

#ifdef __cplusplus
}
#endif


#endif
