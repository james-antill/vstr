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

 unsigned int len : 28; /* private */
 
 unsigned int type : 4; /* private */
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


VSTR__DECL_TYPEDEF1(struct Vstr_locale)
{
  char *name_lc_numeric_str; /* private */
  char *decimal_point_str; /* private */
  char *thousands_sep_str; /* private */
  char *grouping; /* private */
  size_t name_lc_numeric_len; /* private */
  size_t decimal_point_len; /* private */
  size_t thousands_sep_len; /* private */
} VSTR__DECL_TYPEDEF2(Vstr_locale);

struct Vstr_base; /* fwd declaration for callbacks */
VSTR__DECL_TYPEDEF1(struct Vstr_cache_cb)
{
  const char *name;
  void *(*cb_func)(const struct Vstr_base *, size_t, size_t,
                   unsigned int, void *);
} VSTR__DECL_TYPEDEF2(Vstr_cache_cb);

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

 struct Vstr_locale *loc; /* private */
 
 unsigned int iov_min_alloc; /* private */
 unsigned int iov_min_offset; /* private */
 
 unsigned int buf_sz; /* private */

 struct Vstr_cache_cb *cache_cbs_ents; /* private */ 
 unsigned int cache_cbs_sz; /* private */

 unsigned int cache_pos_cb_pos; /* private */
 unsigned int cache_pos_cb_iovec; /* private */
 unsigned int cache_pos_cb_cstr; /* private */
 
 int ref; /* private */
 
 unsigned int free_do : 1; /* private */
 unsigned int malloc_bad : 1; /* public/read|write */
 unsigned int iovec_auto_update : 1; /* private */
 unsigned int split_buf_del : 1; /* private */

 unsigned int no_node_ref : 2; /* private */
 unsigned int no_cache : 1; /* private */
 unsigned int unused04 : 1; /* private */

 VSTR__DEF_BITFLAG_1_4(3); /* private */
 VSTR__DEF_BITFLAG_1_4(4); /* private */
 VSTR__DEF_BITFLAG_1_4(5); /* private */
 VSTR__DEF_BITFLAG_1_4(6); /* private */
 VSTR__DEF_BITFLAG_1_4(7); /* private */
 VSTR__DEF_BITFLAG_1_4(8); /* private */
} VSTR__DECL_TYPEDEF2(Vstr_conf);

VSTR__DECL_TYPEDEF1(struct Vstr_base)
{
  size_t len; /* public/read -- bytes in vstr */
  
  struct Vstr_node *beg; /* private */
  struct Vstr_node *end; /* private */
  
  unsigned int num; /* private */
  
  struct Vstr_conf *conf; /* public/read */
  
  unsigned int used : 16; /* private */
  
  unsigned int free_do : 1; /* private */
  unsigned int iovec_upto_date : 1; /* private */
  unsigned int cache_available : 1; /* private */
  unsigned int unused04 : 1; /* private */
  
  VSTR__DEF_BITFLAG_1_4(6); /* private */
  VSTR__DEF_BITFLAG_1_4(7); /* private */
  VSTR__DEF_BITFLAG_1_4(8); /* private */
} VSTR__DECL_TYPEDEF2(Vstr_base);

VSTR__DECL_TYPEDEF1(struct Vstr_sect_node)
{
  size_t pos;
  size_t len;
} VSTR__DECL_TYPEDEF2(Vstr_sect_node);

VSTR__DECL_TYPEDEF1(struct Vstr_sects)
{
  unsigned int num; /* public/read|write */
  unsigned int sz; /* public/read|write */
  
  unsigned int malloc_bad : 1; /* public/read|write */
  unsigned int free_ptr : 1; /* public/read|write */
  unsigned int can_add_sz : 1; /* public/read|write */
  unsigned int can_del_sz : 1; /* public/read|write */
  
  unsigned int alloc_double : 1; /* public/read|write */
  unsigned int unused02 : 1; /* private */
  unsigned int unused03 : 1; /* private */
  unsigned int unused04 : 1; /* private */

  VSTR__DEF_BITFLAG_1_4(3); /* private */
  VSTR__DEF_BITFLAG_1_4(4); /* private */
  VSTR__DEF_BITFLAG_1_4(5); /* private */
  VSTR__DEF_BITFLAG_1_4(6); /* private */
  VSTR__DEF_BITFLAG_1_4(7); /* private */
  VSTR__DEF_BITFLAG_1_4(8); /* private */
  
  struct Vstr_sect_node *ptr; /* public/read|write */
} VSTR__DECL_TYPEDEF2(Vstr_sects);

