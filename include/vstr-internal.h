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

#if defined(HAVE_ATTRIB_ALIAS)
# define VSTR__ATTR_A(x) __attribute__((alias(x)))
#endif

#if defined(HAVE_ATTRIB_DEPRECATED)
# define VSTR__ATTR_D() __attribute__((deprecated))
#else
# define VSTR__ATTR_D() 
#endif

#if defined(HAVE_ATTRIB_VISIBILITY)
# define VSTR__ATTR_H() __attribute__((visibility("hidden")))
# define VSTR__ATTR_I() __attribute__((visibility("internal")))
#else
# define VSTR__ATTR_H()
# define VSTR__ATTR_I()
#endif

/* include the normal extern declarations ... but fixed */
#if !(defined(HAVE_ATTRIB_ALIAS) && \
      (defined(HAVE_ATTRIB_VISIBILITY) || !defined(NDEBUG)) && \
      defined(HAVE___TYPEOF))
/* if we can't do aliases etc. ... just #define the internal symbols to
 * the external name */
# include "vstr-internal-cpp-symbols.h"
# include "vstr-internal-cpp-inline-symbols.h"
#endif

#include "vstr-internal-extern.h"

#define VSTR_TYPE_FMT_UCHAR 1
#define VSTR_TYPE_FMT_USHORT 2

#define VSTR_TYPE_CACHE_NOTHING 0

/* ISO C magic, converts a ptr to ->next into ->next->prev
 *   (even though ->prev doesn't exist) */
#define VSTR__CONV_PTR_NEXT_PREV(x) \
 ((Vstr_node *)(((char *)(x)) - offsetof(Vstr_node, next)))

#define VSTR__UC(x) ((unsigned char)(x))
#define VSTR__IS_ASCII_LOWER(x) ((VSTR__UC(x) >= 0x61) && (VSTR__UC(x) <= 0x7A))
#define VSTR__IS_ASCII_UPPER(x) ((VSTR__UC(x) >= 0x41) && (VSTR__UC(x) <= 0x5A))
#define VSTR__IS_ASCII_DIGIT(x) ((VSTR__UC(x) >= 0x30) && (VSTR__UC(x) <= 0x39))
#define VSTR__IS_ASCII_ALPHA(x) (VSTR__IS_ASCII_LOWER(x) || \
                                 VSTR__IS_ASCII_UPPER(x))

#define VSTR__TO_ASCII_LOWER(x) (VSTR__UC(x) + 0x20) /* must be IS_ASCII_U */
#define VSTR__TO_ASCII_UPPER(x) (VSTR__UC(x) - 0x20) /* must be IS_ASCII_L */

#define VSTR__ASCII_DIGIT_0() (0x30)
#define VSTR__ASCII_COLON()   (0x3A)
#define VSTR__ASCII_COMMA()   (0x2C)

#define VSTR_REF_INIT() { vstr_nx_ref_cb_free_nothing, NULL, 0 }

#define VSTR__CACHE_INTERNAL_POS_MAX 2

typedef struct Vstr__options
{
 Vstr_conf *def;
 unsigned int mmap_count;
} Vstr__options;

typedef struct Vstr__cache_data_iovec Vstr__cache_data_iovec;

typedef struct Vstr__cache_data_cstr Vstr__cache_data_cstr;

typedef struct Vstr__cache_data_pos Vstr__cache_data_pos;

typedef struct Vstr__cache Vstr__cache;

typedef struct Vstr__base_cache Vstr__base_cache;

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

typedef struct Vstr__fmt_usr_name_node
{
 struct Vstr__fmt_usr_name_node *next;

 const char *name_str;
 size_t      name_len;
 int (*func)(Vstr_base *, size_t, struct Vstr_fmt_spec *);

 unsigned int sz;
 unsigned int VSTR__STRUCT_HACK_ARRAY(types);
} Vstr__fmt_usr_name_node;

extern Vstr__options VSTR__ATTR_H() vstr__options;

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
extern int vstr__check_real_nodes(Vstr_base *) VSTR__ATTR_I();
extern int vstr__check_spare_nodes(Vstr_conf *) VSTR__ATTR_I();
extern void vstr__ref_cb_free_buf_ref(struct Vstr_ref *) VSTR__ATTR_H();
#else
#define vstr__ref_cb_free_buf_ref vstr_ref_cb_free_ref
#endif

extern void vstr__add_user_conf(Vstr_conf *) VSTR__ATTR_I();
extern void vstr__add_base_conf(Vstr_base *, Vstr_conf *) VSTR__ATTR_I();
/* vstr__del_user_user == nx_free_conf */
extern void vstr__del_conf(Vstr_conf *) VSTR__ATTR_I();

extern Vstr_node *vstr__add_setup_pos(Vstr_base *, size_t *, unsigned int *,
                                      size_t *) VSTR__ATTR_I();

extern char *vstr__export_node_ptr(const Vstr_node *) VSTR__ATTR_I();

extern void vstr__base_zero_used(Vstr_base *base) VSTR__ATTR_I();
extern Vstr_node *vstr__base_split_node(Vstr_base *, Vstr_node *, size_t)
    VSTR__ATTR_I();

extern unsigned int vstr__num_node(Vstr_base *, Vstr_node *) VSTR__ATTR_I();
extern Vstr_node *vstr__base_pos(const Vstr_base *, size_t *,
                                 unsigned int *, int) VSTR__ATTR_I();
extern Vstr_node *vstr__base_scan_fwd_beg(const Vstr_base *, size_t, size_t *,
                                          unsigned int *,
                                          char **, size_t *) VSTR__ATTR_I();
extern Vstr_node *vstr__base_scan_fwd_nxt(const Vstr_base *, size_t *,
                                          unsigned int *, Vstr_node *,
                                          char **, size_t *) VSTR__ATTR_I();
extern int        vstr__base_scan_rev_beg(const Vstr_base *, size_t, size_t *,
                                          unsigned int *, unsigned int *,
                                          char **, size_t *) VSTR__ATTR_I();
extern int        vstr__base_scan_rev_nxt(const Vstr_base *, size_t *,
                                          unsigned int *, unsigned int *,
                                          char **, size_t *) VSTR__ATTR_I();

extern int vstr__cache_iovec_alloc(const Vstr_base *, unsigned int)
    VSTR__ATTR_I();
extern void vstr__cache_iovec_add_node_end(Vstr_base *, unsigned int,
                                           unsigned int) VSTR__ATTR_I();
extern void vstr__cache_iovec_reset_node(const Vstr_base *base, Vstr_node *node,
                                        unsigned int num)
    VSTR__ATTR_I();
extern int vstr__cache_iovec_valid(Vstr_base *) /* makes it valid */
    VSTR__ATTR_I();

extern void vstr__cache_free_cstr(const Vstr_base *) VSTR__ATTR_I();
extern void vstr__cache_free_pos(const Vstr_base *) VSTR__ATTR_I();
extern void vstr__cache_del(const Vstr_base *, size_t, size_t) VSTR__ATTR_I();
extern void vstr__cache_add(const Vstr_base *, size_t, size_t) VSTR__ATTR_I();
extern void vstr__cache_cstr_cpy(const Vstr_base *, size_t, size_t,
                                 const Vstr_base *, size_t) VSTR__ATTR_I();
extern int  vstr__make_cache(const Vstr_base *) VSTR__ATTR_I();
extern void vstr__free_cache(const Vstr_base *) VSTR__ATTR_I();
extern int  vstr__cache_conf_init(Vstr_conf *) VSTR__ATTR_I();
extern int  vstr__cache_subset_cbs(Vstr_conf *, Vstr_conf *) VSTR__ATTR_I();
extern int  vstr__cache_dup_cbs(Vstr_conf *, Vstr_conf *) VSTR__ATTR_I();


extern size_t vstr__loc_thou_grp_strlen(const char *) VSTR__ATTR_I();
extern int vstr__make_conf_loc_numeric(Vstr_conf *, const char *)
    VSTR__ATTR_I();

extern void vstr__add_fmt_cleanup_spec(void) VSTR__ATTR_I();

extern Vstr__fmt_usr_name_node *vstr__fmt_usr_match(Vstr_conf *, const char *)
    VSTR__ATTR_I();

/* so the linker-script does the right thing */
extern void vstr_version_func(void);

/* Setup _No eXtern_ symbols for use inside the library */
#if  (defined(HAVE_ATTRIB_ALIAS) && \
      (defined(HAVE_ATTRIB_VISIBILITY) || !defined(NDEBUG)) && \
      defined(HAVE___TYPEOF))

# define VSTR__SYM(x) \
 extern __typeof(vstr_nx_ ## x) vstr_ ## x \
    VSTR__ATTR_A("vstr_nx_" #x ) VSTR__ATTR_D() ;

# include "vstr-internal-alias-symbols.h"

# if defined(VSTR_AUTOCONF_HAVE_INLINE) && VSTR_COMPILE_INLINE
#  include "vstr-internal-inline.h"
# endif

# include "vstr-internal-alias-inline-symbols.h"

#endif

#endif
