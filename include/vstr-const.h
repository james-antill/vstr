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

#define VSTR_INIT_REF { vstr_ref_cb_free_nothing, NULL, 0 }

#define VSTR_INIT_IOVEC { NULL, 0, 0 }

#define VSTR_INIT_BASE { NULL, NULL, 0, 0, VSTR_INIT_IOVEC, NULL, NULL, 0, \
 0, 0, 0, 0 \
 0,0,0,0 \
 0,0,0,0,0,0,0,0 \
 0,0,0,0,0,0,0,0 \
 0,0,0,0,0,0,0,0 }

#define VSTR_TYPE_NODE_BUF 1
#define VSTR_TYPE_NODE_NON 2
#define VSTR_TYPE_NODE_PTR 3
#define VSTR_TYPE_NODE_REF 4

#define VSTR_TYPE_ADD_DEF 0
#define VSTR_TYPE_ADD_BUF_PTR 1
#define VSTR_TYPE_ADD_BUF_REF 2
#define VSTR_TYPE_ADD_ALL_REF 3

#define VSTR__CNTL(x, y) ((VSTR__CNTL_ ## x ## _OFFSET) + (y))

#define VSTR__CNTL_OPT_OFFSET 4000
#define VSTR__CNTL_BASE_OFFSET 5000
#define VSTR__CNTL_CONF_OFFSET 6000

#define VSTR_CNTL_OPT_GET_CONF VSTR__CNTL(OPT, 1)
#define VSTR_CNTL_OPT_SET_CONF VSTR__CNTL(OPT, 2)

#define VSTR_CNTL_BASE_GET_CONF VSTR__CNTL(BASE, 1)
#define VSTR_CNTL_BASE_SET_CONF VSTR__CNTL(BASE, 2)

#define VSTR_CNTL_CONF_GET_NUM_REF VSTR__CNTL(CONF, 1)
/* #define VSTR_CNTL_CONF_SET_NUM_REF VSTR__CNTL(CONF, 2) */
#define VSTR_CNTL_CONF_GET_NUM_IOV_MIN_ALLOC VSTR__CNTL(CONF, 3)
#define VSTR_CNTL_CONF_SET_NUM_IOV_MIN_ALLOC VSTR__CNTL(CONF, 4)
#define VSTR_CNTL_CONF_GET_NUM_IOV_MIN_OFFSET VSTR__CNTL(CONF, 5)
#define VSTR_CNTL_CONF_SET_NUM_IOV_MIN_OFFSET VSTR__CNTL(CONF, 6)
#define VSTR_CNTL_CONF_GET_NUM_BUF_SZ VSTR__CNTL(CONF, 7)
#define VSTR_CNTL_CONF_SET_NUM_BUF_SZ VSTR__CNTL(CONF, 8)
/* more nums ... */
#define VSTR_CNTL_CONF_GET_FLAG_MALLOC_BAD VSTR__CNTL(CONF, 21)
#define VSTR_CNTL_CONF_SET_FLAG_MALLOC_BAD VSTR__CNTL(CONF, 22)
