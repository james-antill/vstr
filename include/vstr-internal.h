#ifndef VSTR__INTERNAL_HEADER_H
#define VSTR__INTERNAL_HEADER_H

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

    /* ISO C magic, converts a ptr to ->next into ->next->prev */
#define VSTR__CONV_PTR_NEXT_PREV(x) \
 ((Vstr_node *)(((char *)(x)) - offsetof(Vstr_node, next)))

#define VSTR__CONST_STR_SPACE_64 \
"                                                                "

#define VSTR__CONST_STR_SPACE_256 \
  VSTR__CONST_STR_SPACE_64 VSTR__CONST_STR_SPACE_64 \
  VSTR__CONST_STR_SPACE_64 VSTR__CONST_STR_SPACE_64

#define VSTR__CONST_STR_SPACE_1024 \
  VSTR__CONST_STR_SPACE_256 VSTR__CONST_STR_SPACE_256 \
  VSTR__CONST_STR_SPACE_256 VSTR__CONST_STR_SPACE_256

#define VSTR__CONST_STR_SPACE VSTR__CONST_STR_SPACE_1024

#define VSTR__CONST_STR_ZERO_64 \
"0000000000000000000000000000000000000000000000000000000000000000"

#define VSTR__CONST_STR_ZERO_256 \
  VSTR__CONST_STR_ZERO_64 VSTR__CONST_STR_ZERO_64 \
  VSTR__CONST_STR_ZERO_64 VSTR__CONST_STR_ZERO_64

#define VSTR__CONST_STR_ZERO_1024 \
  VSTR__CONST_STR_ZERO_256 VSTR__CONST_STR_ZERO_256 \
  VSTR__CONST_STR_ZERO_256 VSTR__CONST_STR_ZERO_256

#define VSTR__CONST_STR_ZERO VSTR__CONST_STR_ZERO_1024


#define VSTR__UC(x) ((unsigned char)(x))
#define VSTR__IS_ASCII_LOWER(x) ((VSTR__UC(x) >= 0x61) && (VSTR__UC(x) <= 0x7A))
#define VSTR__IS_ASCII_UPPER(x) ((VSTR__UC(x) >= 0x41) && (VSTR__UC(x) <= 0x5A))
#define VSTR__IS_ASCII_DIGIT(x) ((VSTR__UC(x) >= 0x30) && (VSTR__UC(x) <= 0x39))

#define VSTR__TO_ASCII_LOWER(x) (VSTR__UC(x) + 0x20) /* must be IS_ASCII_U */
#define VSTR__TO_ASCII_UPPER(x) (VSTR__UC(x) - 0x20) /* must be IS_ASCII_L */

#define VSTR__ASCII_DIGIT_0() (0x30)
#define VSTR__ASCII_COLON() (0x3A)
#define VSTR__ASCII_COMMA() (0x2C)

typedef struct Vstr_cache_data_iovec
{
 struct iovec *v;
 unsigned char *t;
 /* num == base->num */
 unsigned int off;
 unsigned int sz;
} Vstr_cache_data_iovec;

typedef struct Vstr_cache_data_cstr
{
 size_t pos;
 size_t len;
 struct Vstr_ref *ref;
} Vstr_cache_data_cstr;

typedef struct Vstr_cache_data_pos
{
 size_t pos;
 unsigned int num;
 struct Vstr_node *node;
} Vstr_cache_data_pos;

typedef struct Vstr_cache
{
 unsigned int sz;

 struct Vstr_cache_data_iovec *vec;

 void *VSTR__STRUCT_HACK_ARRAY(data);
} Vstr_cache;

typedef struct Vstr__base_cache
{
 Vstr_base base;
 Vstr_cache *cache;
} Vstr__base_cache;

#define VSTR__CACHE(x) ((Vstr__base_cache *)(x))->cache

typedef struct Vstr__options
{
 Vstr_conf *def;
} Vstr__options;

typedef struct Vstr__buf_ref
{
 Vstr_ref ref;
 char VSTR__STRUCT_HACK_ARRAY(buf);
} Vstr__buf_ref;

typedef struct Vstr__sc_mmap_ref
{
 Vstr_ref ref;
 size_t mmap_len;
} Vstr__sc_mmap_ref;

extern Vstr__options vstr__options;

/* the size of ULONG_MAX converted to a string */
#ifndef VSTR_AUTOCONF_ULONG_MAX_LEN
extern size_t vstr__netstr2_ULONG_MAX_len;
#define VSTR__ULONG_MAX_LEN vstr__netstr2_ULONG_MAX_len
#define VSTR__ULONG_MAX_SET_LEN(x) (vstr__netstr2_ULONG_MAX_len = (x))
#else
#define VSTR__ULONG_MAX_LEN VSTR_AUTOCONF_ULONG_MAX_LEN
#define VSTR__ULONG_MAX_SET_LEN(x) /* do nothing */
#endif

#ifndef NDEBUG
extern int vstr__check_real_nodes(Vstr_base *);
extern int vstr__check_spare_nodes(Vstr_conf *);
extern void vstr__ref_cb_free_buf_ref(struct Vstr_ref *);
#else
#define vstr__ref_cb_free_buf_ref vstr_ref_cb_free_ref
#endif

extern int vstr_init_base(struct Vstr_conf *, struct Vstr_base *);

extern void vstr__add_conf(Vstr_conf *);
extern void vstr__add_no_node_conf(Vstr_conf *);
extern void vstr__add_base_conf(Vstr_base *, Vstr_conf *);
extern void vstr__del_conf(Vstr_conf *);

extern void vstr__ref_cb_free_bufnode(struct Vstr_ref *);
extern void vstr__ref_cb_free_bufnode_ref(struct Vstr_ref *);

extern char *vstr__export_node_ptr(const Vstr_node *);

extern Vstr_node *vstr__base_split_node(Vstr_base *, Vstr_node *, size_t);

extern unsigned int vstr__num_node(Vstr_base *, Vstr_node *);
extern Vstr_node *vstr__base_pos(const Vstr_base *, size_t *,
                                 unsigned int *, int);
extern Vstr_node *vstr__base_scan_fwd_beg(const Vstr_base *, size_t, size_t *,
                                          unsigned int *,
                                          char **, size_t *);
extern Vstr_node *vstr__base_scan_fwd_nxt(const Vstr_base *, size_t *,
                                          unsigned int *, Vstr_node *,
                                          char **, size_t *);
extern int vstr__base_scan_rev_beg(const Vstr_base *, size_t, size_t *,
                                   unsigned int *, unsigned int *,
                                   char **, size_t *);
extern int vstr__base_scan_rev_nxt(const Vstr_base *, size_t *,
                                   unsigned int *, unsigned int *,
                                   char **, size_t *);

extern int vstr__cache_iovec_alloc(const Vstr_base *, unsigned int);
extern void vstr__cache_iovec_add_node_end(Vstr_base *, unsigned int,
                                           unsigned int);
extern void vstr__cache_iovec_reset_node(const Vstr_base *base, Vstr_node *node,
                                        unsigned int num);
extern int vstr__cache_iovec_valid(Vstr_base *); /* makes it valid */

extern void vstr__cache_free_cstr(const Vstr_base *);
extern void vstr__cache_free_pos(const Vstr_base *);
extern void vstr__cache_del(const Vstr_base *, size_t, size_t);
extern void vstr__cache_add(const Vstr_base *, size_t, size_t);
extern void vstr__cache_cstr_cpy(const Vstr_base *, size_t, size_t,
                                 const Vstr_base *, size_t);
extern int  vstr__make_cache(const Vstr_base *);
extern void vstr__free_cache(const Vstr_base *);
extern int  vstr__cache_conf_init(Vstr_conf *);

extern size_t vstr__loc_thou_grp_strlen(const char *);
extern int vstr__make_conf_loc_numeric(Vstr_conf *, const char *);

extern void vstr__add_fmt_cleanup_spec(void);

/* so the linker-script does the right thing */
extern void vstr_version_func(void);
    

#endif
