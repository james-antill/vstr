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

VSTR__DECL_TYPEDEF1(struct Vstr_ref)
{
 void (*func) (struct Vstr_ref *); /* public/read|write */
 void *ptr; /* public/read|write */
 unsigned int ref; /* public/read|write */
}  VSTR__DECL_TYPEDEF2(Vstr_ref);

#define VSTR__DEF_BITFLAG_1_4(x) \
 unsigned int unused1_ ## x : 1; \
 unsigned int unused2_ ## x : 1; \
 unsigned int unused3_ ## x : 1; \
 unsigned int unused4_ ## x : 1

VSTR__DECL_TYPEDEF1(struct Vstr_node)
{
 struct Vstr_node *next; /* private */

 unsigned int len; /* private */
 
 unsigned int type : 4; /* private */

 VSTR__DEF_BITFLAG_1_4(4); /* private */
 VSTR__DEF_BITFLAG_1_4(5); /* private */
 VSTR__DEF_BITFLAG_1_4(6); /* private */
 VSTR__DEF_BITFLAG_1_4(7); /* private */
 VSTR__DEF_BITFLAG_1_4(8); /* private */
} VSTR__DECL_TYPEDEF2(Vstr_node);

VSTR__DECL_TYPEDEF1(struct Vstr_node_buf)
{
 struct Vstr_node s; /* private */
 char VSTR__STRUCT_HACK_ARRAY(buf); /* private */
} VSTR__DECL_TYPEDEF2(Vstr_node_buf);

VSTR__DECL_TYPEDEF1(struct Vstr_node_non)
{
 struct Vstr_node s; /* private */
} VSTR__DECL_TYPEDEF2(Vstr_node_non);

VSTR__DECL_TYPEDEF1(struct Vstr_node_ptr)
{
 struct Vstr_node s; /* private */
 void *ptr; /* private */
} VSTR__DECL_TYPEDEF2(Vstr_node_ptr);

VSTR__DECL_TYPEDEF1(struct Vstr_node_ref)
{
 struct Vstr_node s; /* private */
 struct Vstr_ref *ref; /* private */
 unsigned int off;
} VSTR__DECL_TYPEDEF2(Vstr_node_ref);

VSTR__DECL_TYPEDEF1(struct Vstr_conf)
{
 unsigned int spare_buf_num; /* private */
 struct Vstr_node_buf *spare_buf_beg; /* private */

 unsigned int spare_non_num; /* private */
 struct Vstr_node_non *spare_non_beg; /* private */

 unsigned int spare_ptr_num; /* private */
 struct Vstr_node_ptr *spare_ptr_beg; /* private */

 unsigned int spare_ref_num; /* private */
 struct Vstr_node_ref *spare_ref_beg; /* private */

 /* FIXME: struct Vstr_locale *cur_locale; */
 
 unsigned int iov_min_alloc; /* private */
 unsigned int iov_min_offset; /* private */
 
 unsigned int buf_sz; /* private */

 unsigned int free_do : 1; /* private */
 unsigned int malloc_bad : 1; /* public/read */
 unsigned int iovec_auto_update : 1; /* private */
 unsigned int split_buf_del : 1; /* private */
 
 VSTR__DEF_BITFLAG_1_4(2); /* private */
 VSTR__DEF_BITFLAG_1_4(3); /* private */
 VSTR__DEF_BITFLAG_1_4(4); /* private */
 VSTR__DEF_BITFLAG_1_4(5); /* private */
 VSTR__DEF_BITFLAG_1_4(6); /* private */
 VSTR__DEF_BITFLAG_1_4(7); /* private */
 VSTR__DEF_BITFLAG_1_4(8); /* private */
 
 int ref; /* private */
} VSTR__DECL_TYPEDEF2(Vstr_conf);


VSTR__DECL_TYPEDEF1(struct Vstr_iovec)
{
 struct iovec *v; /* private */
 /* num == base->num, or base->conf->spare_buf_num */
 unsigned int off; /* private */
 unsigned int sz; /* private */
} VSTR__DECL_TYPEDEF2(Vstr_iovec);

VSTR__DECL_TYPEDEF1(struct Vstr_cache)
{
 struct Vstr_cache_cstr
 {
  size_t pos; /* private */
  size_t len; /* private */
  struct Vstr_ref *ref; /* private */
 } cstr; /* private */
 
 struct Vstr_cache_pos
 {
  size_t pos; /* private */
  unsigned int num; /* private */
  struct Vstr_node *node; /* private */
 } pos; /* private */
} VSTR__DECL_TYPEDEF2(Vstr_cache);

VSTR__DECL_TYPEDEF1(struct Vstr_base)
{
 struct Vstr_node *beg; /* private */
 struct Vstr_node *end; /* private */
 
 size_t len; /* public/read -- bytes in vstr */
 unsigned int num; /* public/read -- nodes for export_iovec */
 
 struct Vstr_iovec vec; /* private */

 struct Vstr_conf *conf; /* public/read */

 struct Vstr_cache *cache; /* private */
 
 unsigned int used; /* private */
 
 unsigned int free_do : 1; /* private */
 unsigned int iovec_upto_date : 1; /* private */
 unsigned int unused03 : 1; /* private */
 unsigned int unused04 : 1; /* private */
 
 VSTR__DEF_BITFLAG_1_4(4); /* private */
 VSTR__DEF_BITFLAG_1_4(5); /* private */
 VSTR__DEF_BITFLAG_1_4(6); /* private */
 VSTR__DEF_BITFLAG_1_4(7); /* private */
 VSTR__DEF_BITFLAG_1_4(8); /* private */
} VSTR__DECL_TYPEDEF2(Vstr_base);
