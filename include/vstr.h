#ifndef VSTR__HEADER_H
#define VSTR__HEADER_H

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

#ifdef __cplusplus
extern "C"
{
#endif

#include <vstr-conf.h>
#include <vstr-switch.h>

#if VSTR_COMPILE_INCLUDE
# include <stdlib.h>
# include <stdarg.h>
# include <sys/uio.h>
# ifdef VSTR__AUTOCONF_NEED_INTTYPES_H
#  include <stdint.h>
# endif
#endif

#include <vstr-const.h>
#include <vstr-def.h>
#include <vstr-extern.h>

#ifdef __cplusplus
}
#endif


#endif
