#ifndef VSTR__INTERNAL_HEADER_H
#define VSTR__INTERNAL_HEADER_H

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

    /* ISO C magic, converts a ptr to ->next into ->next->prev */
#define VSTR__CONV_PTR_NEXT_PREV(x) \
 ((Vstr_node *)(((char *)(x)) - offsetof(Vstr_node, next)))
    
typedef struct Vstr__options
{
 Vstr_conf *def;
} Vstr__options;

typedef struct Vstr__cstr_ref
{
 Vstr_ref ref;
 char VSTR__STRUCT_HACK_ARRAY(buf);
} Vstr__cstr_ref;

extern Vstr__options vstr__options;

extern size_t vstr__netstr2_num_len;

#ifndef NDEBUG
extern int vstr__check_real_nodes(Vstr_base *);
extern int vstr__check_spare_nodes(Vstr_conf *);
#endif

extern void vstr__del_conf(Vstr_conf *);
extern void vstr__add_conf(Vstr_conf *);
extern void vstr__base_add_conf(Vstr_base *, Vstr_conf *);

extern void vstr__ref_cb_free_bufnode(struct Vstr_ref *);
extern void vstr__ref_cb_free_bufnode_ref(struct Vstr_ref *);

extern char *vstr__export_node_ptr(Vstr_node *);

extern int vstr__base_iovec_alloc(Vstr_base *, unsigned int);
extern void vstr__base_iovec_reset_node(Vstr_base *base, Vstr_node *node,
                                        unsigned int num);
extern int vstr__base_iovec_valid(Vstr_base *);

extern Vstr_node *vstr__base_split_node(Vstr_base *, Vstr_node *, size_t);

extern Vstr_node *vstr__base_pos(const Vstr_base *, size_t *,
                                 unsigned int *, int);
extern Vstr_node *vstr__base_scan_fwd_beg(const Vstr_base *, size_t, size_t *,
                                          unsigned int *,
                                          char **, size_t *);
extern Vstr_node *vstr__base_scan_fwd_nxt(const Vstr_base *, size_t *,
                                          unsigned int *, Vstr_node *,
                                          char **, size_t *);
extern int vstr__base_scan_rev_beg(const Vstr_base *, size_t, size_t *,
                                   unsigned int *, char **, size_t *);
extern int vstr__base_scan_rev_nxt(const Vstr_base *, size_t *, unsigned int *, 
                                   char **, size_t *);

extern void vstr__cache_free_cstr(Vstr_cache *);
extern void vstr__cache_free_pos(Vstr_cache *);
extern void vstr__cache_del(Vstr_base *, size_t, size_t);
extern void vstr__cache_add(Vstr_base *, size_t, size_t);
extern void vstr__cache_cpy(Vstr_base *, size_t, size_t, Vstr_base *, size_t);
extern void vstr__cache_chg(Vstr_base *, size_t, size_t);
extern int vstr__make_cache(Vstr_base *);

extern void vstr__version_func(void);
    
#ifdef __cplusplus
}
#endif


#endif
