#ifndef VSTR__HEADER_H
# error " You must _just_ #include <vstr.h>"
#endif
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

/* macro functions to make flags readable ... don't forget that expantion
 * happens on second macro call */
#define VSTR__FLAG01(B, x1) \
   B ## x1
#define VSTR__FLAG02(B, x1, x2) ( \
 ( B ## x1 ) | \
 ( B ## x2 ) | \
 0)
#define VSTR__FLAG04(B, x1, x2, x3, x4) ( \
 ( B ## x1 ) | \
 ( B ## x2 ) | \
 ( B ## x3 ) | \
 ( B ## x4 ) | \
 0)

#define VSTR_FLAG01(T, x1) ( \
 VSTR__FLAG01( VSTR_FLAG_ ## T , _ ## x1 ) | \
 0)
#define VSTR_FLAG02(T, x1, x2) ( \
 VSTR__FLAG02( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 ) | \
 0)
#define VSTR_FLAG03(T, x1, x2, x3) ( \
 VSTR__FLAG02( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 ) | \
 VSTR__FLAG01( VSTR_FLAG_ ## T , _ ## x3 ) | \
 0)
#define VSTR_FLAG04(T, x1, x2, x3, x4) ( \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 , _ ## x3 , _ ## x4 ) | \
 0)
#define VSTR_FLAG05(T, x1, x2, x3, x4, x5) ( \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 , _ ## x3 , _ ## x4 ) | \
 VSTR__FLAG01( VSTR_FLAG_ ## T , _ ## x5 ) | \
 0)
#define VSTR_FLAG06(T, x1, x2, x3, x4, x5, x6) ( \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 , _ ## x3 , _ ## x4 ) | \
 VSTR__FLAG02( VSTR_FLAG_ ## T , _ ## x5 , _ ## x6 ) | \
 0)
#define VSTR_FLAG07(T, x1, x2, x3, x4, x5, x6, x7) ( \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 , _ ## x3 , _ ## x4 ) | \
 VSTR__FLAG02( VSTR_FLAG_ ## T , _ ## x5 , _ ## x6 ) | \
 VSTR__FLAG01( VSTR_FLAG_ ## T , _ ## x7 ) | \
 0)
#define VSTR_FLAG08(T, x1, x2, x3, x4, x5, x6, x7, x8) ( \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 , _ ## x3 , _ ## x4 ) | \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x5 , _ ## x6 , _ ## x7 , _ ## x8 ) | \
 0)
#define VSTR_FLAG09(T, x1, x2, x3, x4, x5, x6, x7, x8, x9) ( \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 , _ ## x3 , _ ## x4 ) | \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x5 , _ ## x6 , _ ## x7 , _ ## x8 ) | \
 VSTR__FLAG01( VSTR_FLAG_ ## T , _ ## x9 ) | \
 0)
#define VSTR_FLAG10(T, x1, x2, x3, x4, x5, x6, x7, x8, x9, xA) ( \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x1 , _ ## x2 , _ ## x3 , _ ## x4 ) | \
 VSTR__FLAG04( VSTR_FLAG_ ## T , _ ## x5 , _ ## x6 , _ ## x7 , _ ## x8 ) | \
 VSTR__FLAG02( VSTR_FLAG_ ## T , _ ## x9 , _ ## xA ) | \
 0)

/* start of constants ... */
#define VSTR_TYPE_NODE_BUF 1
#define VSTR_TYPE_NODE_NON 2
#define VSTR_TYPE_NODE_PTR 3
#define VSTR_TYPE_NODE_REF 4

#define VSTR_TYPE_ADD_DEF 0
#define VSTR_TYPE_ADD_BUF_PTR 1
#define VSTR_TYPE_ADD_BUF_REF 2
#define VSTR_TYPE_ADD_ALL_REF 3
#define VSTR_TYPE_ADD_ALL_BUF 4

/* aliases to make life more readable */
#define VSTR_TYPE_SUB_DEF VSTR_TYPE_ADD_DEF
#define VSTR_TYPE_SUB_BUF_PTR VSTR_TYPE_ADD_BUF_PTR
#define VSTR_TYPE_SUB_BUF_REF VSTR_TYPE_ADD_BUF_REF
#define VSTR_TYPE_SUB_ALL_REF VSTR_TYPE_ADD_ALL_REF
#define VSTR_TYPE_SUB_ALL_BUF VSTR_TYPE_ADD_ALL_BUF

#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_NONE   0
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_NUL   (1<<0)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BEL   (1<<1)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BS    (1<<2)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HT    (1<<3)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_LF    (1<<4)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_VT    (1<<5)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_FF    (1<<6)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_CR    (1<<7)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_SP    (1<<8)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_COMMA (1<<9)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_DOT   (1<<9)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW__     (1<<10)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_ESC   (1<<11)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_DEL   (1<<12)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HSP   (1<<13)
#define VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HIGH  (1<<14)
#define VSTR_FLAG_CONV_UNPRINTABLE_DEF \
 VSTR_FLAG04(CONV_UNPRINTABLE_ALLOW, SP, COMMA, DOT, _)

#define VSTR_TYPE_PARSE_NUM_ERR_NONE 0
#define VSTR_TYPE_PARSE_NUM_ERR_ONLY_S 1
#define VSTR_TYPE_PARSE_NUM_ERR_ONLY_SPM 2
#define VSTR_TYPE_PARSE_NUM_ERR_ONLY_SPMX 3
#define VSTR_TYPE_PARSE_NUM_ERR_OOB 4
#define VSTR_TYPE_PARSE_NUM_ERR_OVERFLOW 5
#define VSTR_TYPE_PARSE_NUM_ERR_NEGATIVE 6
#define VSTR_TYPE_PARSE_NUM_ERR_BEG_ZERO 7

#define VSTR_FLAG_PARSE_NUM_DEF 0
#define VSTR__MASK_PARSE_NUM_BASE (63) /* (1<<6) - 1 */
#define VSTR_FLAG_PARSE_NUM_LOCAL (1<<6)
#define VSTR_FLAG_PARSE_NUM_SEP (1<<7)
#define VSTR_FLAG_PARSE_NUM_OVERFLOW (1<<8)
#define VSTR_FLAG_PARSE_NUM_SPACE (1<<9)
#define VSTR_FLAG_PARSE_NUM_NO_BEG_ZERO (1<<10)
#define VSTR_FLAG_PARSE_NUM_NO_BEG_PM (1<<11)
/* #define VSTR_FLAG_PARSE_NUM_LOC_SEP ???? */

#define VSTR_TYPE_PARSE_IPV4_ERR_NONE 0
#define VSTR_TYPE_PARSE_IPV4_ERR_IPV4_OOB 1
#define VSTR_TYPE_PARSE_IPV4_ERR_IPV4_FULL 2
#define VSTR_TYPE_PARSE_IPV4_ERR_CIDR_OOB 3
#define VSTR_TYPE_PARSE_IPV4_ERR_CIDR_FULL 4
#define VSTR_TYPE_PARSE_IPV4_ERR_NETMASK_OOB 5
#define VSTR_TYPE_PARSE_IPV4_ERR_NETMASK_FULL 6
#define VSTR_TYPE_PARSE_IPV4_ERR_ONLY 7

#define VSTR_FLAG_PARSE_IPV4_DEF 0
#define VSTR_FLAG_PARSE_IPV4_LOCAL (1<<0)
#define VSTR_FLAG_PARSE_IPV4_ZEROS (1<<1)
#define VSTR_FLAG_PARSE_IPV4_FULL (1<<2)
#define VSTR_FLAG_PARSE_IPV4_CIDR (1<<3)
#define VSTR_FLAG_PARSE_IPV4_CIDR_FULL (1<<4)
#define VSTR_FLAG_PARSE_IPV4_NETMASK (1<<5)
#define VSTR_FLAG_PARSE_IPV4_NETMASK_FULL (1<<6)
#define VSTR_FLAG_PARSE_IPV4_ONLY (1<<7)

#define VSTR_FLAG_SPLIT_DEF 0
#define VSTR_FLAG_SPLIT_BEG_NULL (1<<0)
#define VSTR_FLAG_SPLIT_MID_NULL (1<<1)
#define VSTR_FLAG_SPLIT_END_NULL (1<<2)
#define VSTR_FLAG_SPLIT_POST_NULL (1<<3)
#define VSTR_FLAG_SPLIT_NO_RET (1<<4)
#define VSTR_FLAG_SPLIT_REMAIN (1<<5)

#define VSTR_FLAG_SECTS_FOREACH_DEF 0
#define VSTR_FLAG_SECTS_FOREACH_BACKWARDS (1<<0)
#define VSTR_FLAG_SECTS_FOREACH_ALLOW_NULL (1<<1)

#define VSTR_TYPE_SECTS_FOREACH_DEF 0
#define VSTR_TYPE_SECTS_FOREACH_DEL 1
#define VSTR_TYPE_SECTS_FOREACH_RET 2

#define VSTR_TYPE_CACHE_ADD 1
#define VSTR_TYPE_CACHE_DEL 2
#define VSTR_TYPE_CACHE_SUB 3
#define VSTR_TYPE_CACHE_FREE 4
/* #define VSTR_TYPE_CACHE_LOC 5 */

#define VSTR_TYPE_SC_ADD_FD_ERR_NONE 0
#define VSTR_TYPE_SC_ADD_FD_ERR_FSTAT_ERRNO 2
#define VSTR_TYPE_SC_ADD_FD_ERR_MMAP_ERRNO 3
#define VSTR_TYPE_SC_ADD_FD_ERR_MEM 5
#define VSTR_TYPE_SC_ADD_FD_ERR_TOO_LARGE 6

#define VSTR_TYPE_SC_ADD_FILE_ERR_NONE 0
#define VSTR_TYPE_SC_ADD_FILE_ERR_OPEN_ERRNO 1
#define VSTR_TYPE_SC_ADD_FILE_ERR_FSTAT_ERRNO VSTR_TYPE_SC_ADD_FD_ERR_FSTAT_ERRNO
#define VSTR_TYPE_SC_ADD_FILE_ERR_MMAP_ERRNO VSTR_TYPE_SC_ADD_FD_ERR_FSTAT_ERRNO
#define VSTR_TYPE_SC_ADD_FILE_ERR_CLOSE_ERRNO 4
#define VSTR_TYPE_SC_ADD_FILE_ERR_MEM VSTR_TYPE_SC_ADD_FD_ERR_MEM
#define VSTR_TYPE_SC_ADD_FILE_ERR_TOO_LARGE VSTR_TYPE_SC_ADD_FD_ERR_TOO_LARGE

#define VSTR_TYPE_SC_READ_FD_ERR_NONE 0
#define VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO 1
#define VSTR_TYPE_SC_READ_FD_ERR_EOF 2
#define VSTR_TYPE_SC_READ_FD_ERR_MEM 3

#define VSTR_TYPE_SC_WRITE_FD_ERR_NONE 0
#define VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO 2
#define VSTR_TYPE_SC_WRITE_FD_ERR_MEM 4

#define VSTR_TYPE_SC_WRITE_FILE_ERR_NONE 0
#define VSTR_TYPE_SC_WRITE_FILE_ERR_OPEN_ERRNO 1
#define VSTR_TYPE_SC_WRITE_FILE_ERR_WRITE_ERRNO VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO
#define VSTR_TYPE_SC_WRITE_FILE_ERR_CLOSE_ERRNO 3
#define VSTR_TYPE_SC_WRITE_FILE_ERR_MEM VSTR_TYPE_SC_WRITE_FD_ERR_MEM

#define VSTR_MAX_NODE_BUF 0xFFFF /* used is 16 bits */
#define VSTR_MAX_NODE_ALL 0xFFFFFFF /* node->len is 28 bits */

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
/* #define VSTR_CNTL_CONF_GET_LOC_CSTR_AUTO_NAME_NUMERIC VSTR__CNTL(CONF, 9) */
#define VSTR_CNTL_CONF_SET_LOC_CSTR_AUTO_NAME_NUMERIC VSTR__CNTL(CONF, 10)
#define VSTR_CNTL_CONF_GET_LOC_CSTR_NAME_NUMERIC VSTR__CNTL(CONF, 11)
#define VSTR_CNTL_CONF_SET_LOC_CSTR_NAME_NUMERIC VSTR__CNTL(CONF, 12)
#define VSTR_CNTL_CONF_GET_LOC_CSTR_DEC_POINT VSTR__CNTL(CONF, 13)
#define VSTR_CNTL_CONF_SET_LOC_CSTR_DEC_POINT VSTR__CNTL(CONF, 14)
#define VSTR_CNTL_CONF_GET_LOC_CSTR_THOU_SEP VSTR__CNTL(CONF, 15)
#define VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_SEP VSTR__CNTL(CONF, 16)
#define VSTR_CNTL_CONF_GET_LOC_CSTR_THOU_GRP VSTR__CNTL(CONF, 17)
#define VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_GRP VSTR__CNTL(CONF, 18)
#define VSTR_CNTL_CONF_GET_FLAG_IOV_UPDATE VSTR__CNTL(CONF, 19)
#define VSTR_CNTL_CONF_SET_FLAG_IOV_UPDATE VSTR__CNTL(CONF, 20)
#define VSTR_CNTL_CONF_GET_FLAG_DEL_SPLIT VSTR__CNTL(CONF, 21)
#define VSTR_CNTL_CONF_SET_FLAG_DEL_SPLIT VSTR__CNTL(CONF, 22)



