#define VSTR_INLINE_C
/*
 *  Copyright (C) 2002, 2003  James Antill
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
/* functions which are inlined */

#undef  VSTR_COMPILE_INLINE /* gcc3 SEGVs if it sees the inline's twice */
#define VSTR_COMPILE_INLINE 0

#include "main.h"

/* everything is done in vstr-inline.h with magic added vstr-internal.h */
/* This works with gcc */
# undef extern
# define extern /* nothing */
# undef inline
# define inline /* nothing */

#ifndef NDEBUG
# ifdef USE_ASSERT_LOOP
#  define VSTR__ASSERT(x) do { \
 if (x) {} else \
  vstr__assert_loop(#x, __FILE__, __LINE__, __func__); } \
 while (FALSE)
#  define VSTR__ASSERT_RET(x, y)  do { \
 if (x) {} else \
  vstr__assert_loop(#x, __FILE__, __LINE__, __func__); } \
 while (FALSE)
# else
#  define VSTR__ASSERT(x) do { \
 if (x) {} else { \
  fprintf(stderr, " -=> ASSERT (%s) failed in (%s) from %d %s.\n", \
          #x , __func__, __LINE__, __FILE__); \
  abort(); } } \
 while (FALSE)
#  define VSTR__ASSERT_RET(x, y)  do { \
 if (x) {} else { \
  fprintf(stderr, " -=> ASSERT (%s) failed in (%s) from %d %s.\n", \
          #x , __func__, __LINE__, __FILE__); \
  abort(); } } \
 while (FALSE)
# endif
# define VSTR__ASSERT_NO_SWITCH_DEF() break; default: VSTR__ASSERT(FALSE)
#endif

# include "vstr-inline.h"
# include "vstr-nx-inline.h"
