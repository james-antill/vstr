#ifndef VSTR__HEADER_H
# error " You must _just_ #include <vstr.h>"
#endif
/*
 *  Copyright (C) 1999, 2000, 2001  James Antill
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


#ifndef VSTR_COMPILE_TYPEDEF
# define VSTR_COMPILE_TYPEDEF 1
#endif

#ifndef VSTR_COMPILE_INCLUDE
# define VSTR_COMPILE_INCLUDE 0
#endif

#if VSTR_COMPILE_TYPEDEF
# define VSTR__DECL_TYPEDEF1(x) typedef x
# define VSTR__DECL_TYPEDEF2(x) x
#else
# define VSTR__DECL_TYPEDEF1(x) x
# define VSTR__DECL_TYPEDEF2(x) /* nothing */
#endif

#ifdef VSTR_AUTOCONF_HAVE_C9x_STRUCT_HACK
# define VSTR__STRUCT_HACK_ARRAY(x) x[]
# define VSTR__STRUCT_HACK_SZ(type) (0)
#else
/*  The _SZ will be wrong if you compile your program with -ansi but *
 * the lib without ... but that doesn't matter because it's internal
 * so you shouldn't be using it anyway.
 *
 *  Technically _ARRAY will be too, but that doesn't matter as you can't
 * malloc() your own nodes either.
 */
# if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#  define VSTR__STRUCT_HACK_ARRAY(x) x[0]
#  define VSTR__STRUCT_HACK_SZ(type) (0)
# else
#  define VSTR__STRUCT_HACK_ARRAY(x) x[1]
#  define VSTR__STRUCT_HACK_SZ(type) sizeof(type)
# endif
#endif


#ifdef VSTR_AUTOCONF_NDEBUG
# define VSTR_TYPE_CONST_DEBUG_1 0
# define VSTR_TYPE_CONST_DEBUG_16 0
# define VSTR_TYPE_CONST_DEBUG_32 0
#else
# define VSTR_TYPE_CONST_DEBUG_1 1
# define VSTR_TYPE_CONST_DEBUG_16 0x5555 /* ok for signed too */
# define VSTR_TYPE_CONST_DEBUG_32 0x55555555 /* ok for signed too */
#endif

#ifndef VSTR_AUTOCONF_intmax_t
# define VSTR_AUTOCONF_intmax_t intmax_t
# ifndef VSTR__AUTOCONF_NEED_INTTYPES_H
#  define VSTR__AUTOCONF_NEED_INTTYPES_H 1
# endif
#endif

#ifndef VSTR_AUTOCONF_uintmax_t
# define VSTR_AUTOCONF_uintmax_t uintmax_t
# ifndef VSTR__AUTOCONF_NEED_INTTYPES_H
#  define VSTR__AUTOCONF_NEED_INTTYPES_H 1
# endif
#endif
