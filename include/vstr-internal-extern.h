/* DO NOT EDIT THIS FILE */
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

/* == macro functions == */
#define VSTR_SECTS_INIT(sects, p_sz, p_ptr, p_free_ptr) do { \
  (sects)->num = 0; \
  (sects)->sz = (p_sz); \
 \
  (sects)->malloc_bad = 0; \
  (sects)->free_ptr =   !!(p_free_ptr); \
  (sects)->can_add_sz = !!(p_free_ptr); \
  (sects)->can_del_sz = 0; \
 \
  (sects)->alloc_double = 1; \
 \
 \
  (sects)->ptr = (p_ptr); \
 } while (0)

#define VSTR__SECTS_DECL_INIT(p_sz) { 0, (p_sz), \
 0, 0, 0, 0, \
 \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 (Vstr_sect_node *)0}

/* This is more disgusting than it should be because C89 doesn't say you
 * can init part of a struct to another variable.
 *
 * It basically does the same thing as...
 *
 * struct Vstr_sects name[1] = VSTR__SECTS_DECL_INIT(sz);
 * struct Vstr_sect_node name[sz];
 *
 * ...asssuming that C allowed that, with the meaning of tack the second
 * declaration on the end of the first.
 *
 */
/* The allocation is calculated by working out the of the difference in structs
 * and then working out, from that, how many Vstr_sects nodes are wasted.
 * This wasted value is then taken from the number passed, to come up with how
 * many should be used.
 * NOTE: the wasted number is rounded down, so we waste at most
 * sizeof(Vstr_sects) bytes.
 */
#define VSTR_SECTS_EXTERN_DECL(name, sz) \
 struct Vstr_sects name[1 + \
   (((unsigned int) (sz)) - (((sizeof(struct Vstr_sects) - \
              sizeof(struct Vstr_sect_node)) * ((unsigned int) (sz))) / \
            sizeof(struct Vstr_sects)))]

#define VSTR_SECTS_DECL(name, sz) \
 VSTR_SECTS_EXTERN_DECL(name, sz) = {VSTR__SECTS_DECL_INIT(sz)}

#define VSTR_SECTS_DECL_INIT(sects) \
 ((void)((sects)->ptr = (sects)->integrated_objs))

#define VSTR_SECTS_NUM(sects, pos) (&((sects)->ptr[((unsigned int) (pos)) - 1]))

#define VSTR_FMT_CB_ARG_PTR(spec, num) \
 (       (spec)->data_ptr[(size_t) (num)])
#define VSTR_FMT_CB_ARG_VAL(spec, T, num) \
 (*(T *)((spec)->data_ptr[(size_t) (num)]))

#define VSTR_CMP_EQ(x, p1, l1, y, p2, l2) (((l1) == (l2)) && \
 !vstr_nx_cmp(x, p1, l1, y, p2, l1))
#define VSTR_CMP_CSTR(x, p1, l1, y) \
  vstr_nx_cmp_buf(x, p1, l1, y, strlen(y))
#define VSTR_CMP_BUF_EQ(x, p1, l1, y, l2) (((l1) == (l2)) && \
 !vstr_nx_cmp_buf(x, p1, l1, y, l1))
#define VSTR_CMP_CSTR_EQ(x, p1, l1, y) (((l1) == strlen(y)) && \
 !vstr_nx_cmp_buf(x, p1, l1, y, l1))
#define VSTR_CMP_CASE_EQ(x, p1, l1, y, p2, l2) (((l1) == (l2)) && \
 !vstr_nx_cmp_case(x, p1, l1, y, p2, l1))
#define VSTR_CMP_CASE_BUF_EQ(x, p1, l1, y, l2) (((l1) == (l2)) && \
 !vstr_nx_cmp_case_buf(x, p1, l1, y, l1))
#define VSTR_CMP_CASE_CSTR(x, p1, l1, y) \
  vstr_nx_cmp_case_buf(x, p1, l1, y, strlen(y))
#define VSTR_CMP_CASE_CSTR_EQ(x, p1, l1, y) (((l1) == strlen(y)) && \
 !vstr_nx_cmp_case_buf(x, p1, l1, y, l1))
#define VSTR_CMP_VERS_EQ(x, p1, l1, y, p2, l2) (((l1) == (l2)) && \
 !vstr_nx_cmp(x, p1, l1, y, p2, l1))
#define VSTR_CMP_VERS_BUF_EQ(x, p1, l1, y, l2) (((l1) == (l2)) && \
 !vstr_nx_cmp_buf(x, p1, l1, y, l1))
#define VSTR_CMP_VERS_CSTR(x, p1, l1, y) \
  vstr_nx_cmp_vers_buf(x, p1, l1, y, strlen(y))
#define VSTR_CMP_VERS_CSTR_EQ(x, p1, l1, y) (((l1) == strlen(y)) && \
 !vstr_nx_cmp_vers_buf(x, p1, l1, y, l1))


#define VSTR_ADD_CSTR_BUF(x, y, buf) vstr_nx_add_buf(x, y, buf, strlen(buf))
#define VSTR_ADD_CSTR_PTR(x, y, ptr) vstr_nx_add_ptr(x, y, ptr, strlen(ptr))
#define VSTR_ADD_CSTR_REF(x, y, ref, off) \
 vstr_nx_add_ref(x, y, ref, off, strlen(((const char *)(ref)->ptr) + (off)))


#define VSTR_DUP_CSTR_BUF(x, buf) vstr_nx_dup_buf(x, buf, strlen(buf))
#define VSTR_DUP_CSTR_PTR(x, ptr) vstr_nx_dup_ptr(x, ptr, strlen(ptr))
#define VSTR_DUP_CSTR_REF(x, ref, off) \
 vstr_nx_dup_ref(x, ref, off, strlen(((const char *)(ref)->ptr) + (off)))


#define VSTR_SUB_CSTR_BUF(x, y, z, buf) \
 vstr_nx_sub_buf(x, y, z, buf, strlen(buf))
#define VSTR_SUB_CSTR_PTR(x, y, z, ptr) \
 vstr_nx_sub_ptr(x, y, z, ptr, strlen(ptr))
#define VSTR_SUB_CSTR_REF(x, y, z, ref, off) \
 vstr_nx_sub_ref(x, y, z, ref, off, strlen(((const char *)(ref)->ptr) + (off)))


#define VSTR_SRCH_CSTR_BUF_FWD(x, y, z, buf) \
 vstr_nx_srch_buf_fwd(x, y, z, buf, strlen(buf))
#define VSTR_SRCH_CSTR_BUF_REV(x, y, z, buf) \
 vstr_nx_srch_buf_rev(x, y, z, buf, strlen(buf))

#define VSTR_SRCH_CSTR_CHRS_FWD(x, y, z, chrs) \
 vstr_nx_srch_chrs_fwd(x, y, z, chrs, strlen(chrs))
#define VSTR_SRCH_CSTR_CHRS_REV(x, y, z, chrs) \
 vstr_nx_srch_chrs_rev(x, y, z, chrs, strlen(chrs))

#define VSTR_CSRCH_CSTR_CHRS_FWD(x, y, z, chrs) \
 vstr_nx_csrch_chrs_fwd(x, y, z, chrs, strlen(chrs))
#define VSTR_CSRCH_CSTR_CHRS_REV(x, y, z, chrs) \
 vstr_nx_csrch_chrs_rev(x, y, z, chrs, strlen(chrs))

#define VSTR_SRCH_CASE_CSTR_BUF_FWD(x, y, z, buf) \
 vstr_nx_srch_case_buf_fwd(x, y, z, buf, strlen(buf))
#define VSTR_SRCH_CASE_CSTR_BUF_REV(x, y, z, buf) \
 vstr_nx_srch_case_buf_rev(x, y, z, buf, strlen(buf))


#define VSTR_SPN_CSTR_CHRS_FWD(x, y, z, chrs) \
 vstr_nx_spn_chrs_fwd(x, y, z, chrs, strlen(chrs))
#define VSTR_SPN_CSTR_CHRS_REV(x, y, z, chrs) \
 vstr_nx_spn_chrs_rev(x, y, z, chrs, strlen(chrs))

#define VSTR_CSPN_CSTR_CHRS_FWD(x, y, z, chrs) \
 vstr_nx_cspn_chrs_fwd(x, y, z, chrs, strlen(chrs))
#define VSTR_CSPN_CSTR_CHRS_REV(x, y, z, chrs) \
 vstr_nx_cspn_chrs_rev(x, y, z, chrs, strlen(chrs))


#define VSTR_SPLIT_CSTR_BUF(x, y, z, buf, sect, lim, flags) \
 vstr_nx_split_buf(x, y, z, buf, strlen(buf), sect, lim, flags)
#define VSTR_SPLIT_CSTR_CHRS(x, y, z, chrs, sect, lim, flags) \
 vstr_nx_split_chrs(x, y, z, chrs, strlen(chrs), sect, lim, flags)


#define VSTR__CACHE(x) ((struct Vstr__base_cache *)(x))->cache

/* == real functions == */

/* not really vectored string functions ... just stuff needed */
extern void vstr_nx_ref_cb_free_nothing(struct Vstr_ref *) VSTR__ATTR_H() ;
extern void vstr_nx_ref_cb_free_ref(struct Vstr_ref *) VSTR__ATTR_H() ;
extern void vstr_nx_ref_cb_free_ptr(struct Vstr_ref *) VSTR__ATTR_H() ;
extern void vstr_nx_ref_cb_free_ptr_ref(struct Vstr_ref *) VSTR__ATTR_H() ;

extern struct Vstr_ref *vstr_nx_ref_add(struct Vstr_ref *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern void vstr_nx_ref_del(struct Vstr_ref *) VSTR__ATTR_H() ;
extern struct Vstr_ref *vstr_nx_ref_make_ptr(void *,
                                          void (*)(struct Vstr_ref *))
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern struct Vstr_ref *vstr_nx_ref_make_malloc(size_t)
    VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;

/* real start of vector string functions */

extern int  vstr_nx_init(void) VSTR__ATTR_H() ;
extern void vstr_nx_exit(void) VSTR__ATTR_H() ;

extern struct Vstr_conf *vstr_nx_make_conf(void)
    VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern void vstr_nx_free_conf(struct Vstr_conf *) VSTR__ATTR_H() ;
extern int vstr_nx_swap_conf(struct Vstr_base *, struct Vstr_conf **)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern struct Vstr_base *vstr_nx_make_base(struct Vstr_conf *)
    VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern void vstr_nx_free_base(struct Vstr_base *) VSTR__ATTR_H() ;

extern unsigned int vstr_nx_make_spare_nodes(struct Vstr_conf *,
                                          unsigned int,
                                          unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern unsigned int vstr_nx_free_spare_nodes(struct Vstr_conf *,
                                          unsigned int,
                                          unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* add data functions */
extern int vstr_nx_add_buf(struct Vstr_base *, size_t, const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_add_ptr(struct Vstr_base *, size_t, const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_add_non(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_add_ref(struct Vstr_base *, size_t,
                        struct Vstr_ref *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_add_vstr(struct Vstr_base *, size_t,
                         const struct Vstr_base *, size_t, size_t,
                         unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_add_rep_chr(struct Vstr_base *, size_t, char, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern struct Vstr_base *vstr_nx_dup_buf(struct Vstr_conf *, const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_L((2)) VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern struct Vstr_base *vstr_nx_dup_ptr(struct Vstr_conf *, const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_L((2)) VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern struct Vstr_base *vstr_nx_dup_non(struct Vstr_conf *, size_t)
                                      VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern struct Vstr_base *vstr_nx_dup_ref(struct Vstr_conf *,
                                      struct Vstr_ref *ref, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_L((2)) VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern struct Vstr_base *vstr_nx_dup_vstr(struct Vstr_conf *,
                                       const struct Vstr_base *, size_t, size_t,
                                       unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_L((2)) VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern struct Vstr_base *vstr_nx_dup_rep_chr(struct Vstr_conf *, char, size_t)
    VSTR__COMPILE_ATTR_NONNULL_L((2)) VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;

extern size_t vstr_nx_add_vfmt(struct Vstr_base *, size_t, const char *, va_list)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__COMPILE_ATTR_FMT(3, 0) VSTR__ATTR_H() ;
extern size_t vstr_nx_add_fmt(struct Vstr_base *, size_t, const char *, ...)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 3)) VSTR__COMPILE_ATTR_FMT(3, 4) VSTR__ATTR_H() ;
extern size_t vstr_nx_add_vsysfmt(struct Vstr_base *, size_t, const char *, va_list)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__COMPILE_ATTR_FMT(3, 0) VSTR__ATTR_H() ;
extern size_t vstr_nx_add_sysfmt(struct Vstr_base *, size_t, const char *, ...)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 3)) VSTR__COMPILE_ATTR_FMT(3, 4) VSTR__ATTR_H() ;

extern size_t vstr_nx_add_iovec_buf_beg(struct Vstr_base *, size_t,
                                     unsigned int, unsigned int,
                                     struct iovec **,
                                     unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 5)) VSTR__ATTR_H() ;
extern void vstr_nx_add_iovec_buf_end(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* NOTE: netstr2 allows leading '0' s, netstr doesn't */
extern size_t vstr_nx_add_netstr_beg(struct Vstr_base *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_add_netstr_end(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_add_netstr2_beg(struct Vstr_base *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_add_netstr2_end(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* delete functions */
extern int vstr_nx_del(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* substitution functions */
extern int vstr_nx_sub_buf(struct Vstr_base *, size_t, size_t,
                        const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sub_ptr(struct Vstr_base *, size_t, size_t,
                        const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sub_non(struct Vstr_base *, size_t, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sub_ref(struct Vstr_base *, size_t, size_t,
                        struct Vstr_ref *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sub_vstr(struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t,
                         unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sub_rep_chr(struct Vstr_base *, size_t, size_t,
                            char, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* convertion functions */
extern int vstr_nx_conv_lowercase(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_conv_uppercase(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_conv_unprintable_chr(struct Vstr_base *, size_t, size_t,
                                     unsigned int, char)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_conv_unprintable_del(struct Vstr_base *, size_t, size_t,
                                     unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_conv_encode_uri(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_conv_decode_uri(struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;


/* move functions */
extern int vstr_nx_mov(struct Vstr_base *, size_t,
                    struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* control misc stuff */
extern int vstr_nx_cntl_opt(int, ...) VSTR__ATTR_H() ;
extern int vstr_nx_cntl_base(struct Vstr_base *, int, ...)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern int vstr_nx_cntl_conf(struct Vstr_conf *, int, ...)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;

/* --------------------------------------------------------------------- */
/* constant base functions below here */
/* --------------------------------------------------------------------- */

/* data about the vstr */
extern unsigned int vstr_nx_num(const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* comparing functions */
extern int vstr_nx_cmp(const struct Vstr_base *, size_t, size_t,
                    const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_cmp_buf(const struct Vstr_base *, size_t, size_t,
                        const void *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_cmp_case(const struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_cmp_case_buf(const struct Vstr_base *, size_t, size_t,
                             const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_cmp_vers(const struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_cmp_vers_buf(const struct Vstr_base *, size_t, size_t,
                             const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* search functions */
extern size_t vstr_nx_srch_chr_fwd(const struct Vstr_base *, size_t, size_t, char)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_srch_chr_rev(const struct Vstr_base *, size_t, size_t, char)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern size_t vstr_nx_srch_chrs_fwd(const struct Vstr_base *, size_t, size_t,
                                 const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_srch_chrs_rev(const struct Vstr_base *, size_t, size_t,
                                 const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_csrch_chrs_fwd(const struct Vstr_base *, size_t, size_t,
                                  const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_csrch_chrs_rev(const struct Vstr_base *, size_t, size_t,
                                  const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern size_t vstr_nx_srch_buf_fwd(const struct Vstr_base *, size_t, size_t,
                                const void *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_srch_buf_rev(const struct Vstr_base *, size_t, size_t,
                                const void *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern size_t vstr_nx_srch_vstr_fwd(const struct Vstr_base *, size_t, size_t,
                                 const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_srch_vstr_rev(const struct Vstr_base *, size_t, size_t,
                                 const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern size_t vstr_nx_srch_case_chr_fwd(const struct Vstr_base *, size_t, size_t, 
                                     char)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_srch_case_chr_rev(const struct Vstr_base *, size_t, size_t,
                                     char)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern size_t vstr_nx_srch_case_buf_fwd(const struct Vstr_base *, size_t, size_t,
                                     const void *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_srch_case_buf_rev(const struct Vstr_base *, size_t, size_t,
                                     const void *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern size_t vstr_nx_srch_case_vstr_fwd(const struct Vstr_base *, size_t, size_t,
                                      const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_srch_case_vstr_rev(const struct Vstr_base *, size_t, size_t,
                                      const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* spanning and compliment spanning */
extern size_t vstr_nx_spn_chrs_fwd(const struct Vstr_base *, size_t, size_t,
                                const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_spn_chrs_rev(const struct Vstr_base *, size_t, size_t,
                                const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_cspn_chrs_fwd(const struct Vstr_base *, size_t, size_t,
                                 const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern size_t vstr_nx_cspn_chrs_rev(const struct Vstr_base *, size_t, size_t,
                                 const char *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;


/* export functions */
extern size_t vstr_nx_export_iovec_ptr_all(const struct Vstr_base *,
                                        struct iovec **,
                                        unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern size_t vstr_nx_export_iovec_cpy_buf(const struct Vstr_base *,
                                        size_t, size_t,
                                        struct iovec *, unsigned int,
                                        unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 4)) VSTR__ATTR_H() ;
extern size_t vstr_nx_export_iovec_cpy_ptr(const struct Vstr_base *,
                                        size_t, size_t,
                                        struct iovec *, unsigned int,
                                        unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 4)) VSTR__ATTR_H() ;
extern size_t vstr_nx_export_buf(const struct Vstr_base *, size_t, size_t,
                              void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern char vstr_nx_export_chr(const struct Vstr_base *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern struct Vstr_ref *vstr_nx_export_ref(const struct Vstr_base *,
                                        size_t, size_t, size_t *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern char *vstr_nx_export_cstr_ptr(const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern char *vstr_nx_export_cstr_malloc(const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern size_t vstr_nx_export_cstr_buf(const struct Vstr_base *, size_t, size_t,
                                   void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern struct Vstr_ref *vstr_nx_export_cstr_ref(const struct Vstr_base *,
                                             size_t, size_t, size_t *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

/* functions for parsing data out of a vstr */
extern short vstr_nx_parse_short(const struct Vstr_base *,
                              size_t, size_t, unsigned int, size_t *,
                              unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern unsigned short vstr_nx_parse_ushort(const struct Vstr_base *,
                                        size_t, size_t,
                                        unsigned int, size_t *,
                                        unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern int vstr_nx_parse_int(const struct Vstr_base *,
                          size_t, size_t, unsigned int, size_t *,
                          unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern unsigned int vstr_nx_parse_uint(const struct Vstr_base *,
                                    size_t, size_t, unsigned int, size_t *,
                                    unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern long vstr_nx_parse_long(const struct Vstr_base *,
                            size_t, size_t, unsigned int, size_t *,
                            unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern unsigned long vstr_nx_parse_ulong(const struct Vstr_base *,
                                      size_t, size_t, unsigned int, size_t *,
                                      unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern VSTR_AUTOCONF_intmax_t vstr_nx_parse_intmax(const struct Vstr_base *,
                                                size_t, size_t, unsigned int,
                                                size_t *, unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern VSTR_AUTOCONF_uintmax_t vstr_nx_parse_uintmax(const struct Vstr_base *,
                                                  size_t, size_t,
                                                  unsigned int, size_t *,
                                                  unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;

extern int vstr_nx_parse_ipv4(const struct Vstr_base *,
                           size_t, size_t,
                           unsigned char *, unsigned int *, unsigned int,
                           size_t *, unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 4)) VSTR__ATTR_H() ;

extern size_t vstr_nx_parse_netstr(const struct Vstr_base *, size_t, size_t,
                                size_t *, size_t *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern size_t vstr_nx_parse_netstr2(const struct Vstr_base *, size_t, size_t,
                                 size_t *, size_t *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;

/* section functions -- seperate namespace */
extern struct Vstr_sects *vstr_nx_sects_make(unsigned int)
    VSTR__COMPILE_ATTR_MALLOC() VSTR__ATTR_H() ;
extern void vstr_nx_sects_free(struct Vstr_sects *) VSTR__ATTR_H() ;

extern int vstr_nx_sects_add(struct Vstr_sects *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sects_del(struct Vstr_sects *, unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern unsigned int vstr_nx_sects_srch(struct Vstr_sects *, size_t, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern int vstr_nx_sects_foreach(const struct Vstr_base *, struct Vstr_sects *,
                              unsigned int,
                              unsigned int (*) (const struct Vstr_base *,
                                                size_t, size_t, void *),
                              void *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 2, 4)) VSTR__ATTR_H() ;

extern int vstr_nx_sects_update_add(const struct Vstr_base *,
                                 struct Vstr_sects *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sects_update_del(const struct Vstr_base *,
                                 struct Vstr_sects *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;


/* split functions */
extern unsigned int vstr_nx_split_buf(const struct Vstr_base *, size_t, size_t,
                                   const void *, size_t, struct Vstr_sects *,
                                   unsigned int, unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern unsigned int vstr_nx_split_chrs(const struct Vstr_base *, size_t, size_t,
                                    const char *, size_t, struct Vstr_sects *,
                                    unsigned int, unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;


/* cache functions */
extern unsigned int vstr_nx_cache_add(struct Vstr_conf *, const char *,
                                   void *(*)(const struct Vstr_base *,
                                             size_t, size_t,
                                             unsigned int, void *))
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern unsigned int vstr_nx_cache_srch(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern void *vstr_nx_cache_get(const struct Vstr_base *, unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_cache_set(const struct Vstr_base *, unsigned int, void *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;

extern void vstr_nx_cache_cb_sub(const struct Vstr_base *, size_t, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern void vstr_nx_cache_cb_free(const struct Vstr_base *, unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;


/* custom format specifier registration functions */
extern int vstr_nx_fmt_add(struct Vstr_conf *, const char *,
                        int (*)(struct Vstr_base *, size_t,
                                struct Vstr_fmt_spec *), ...)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern void vstr_nx_fmt_del(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_fmt_srch(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;


/* shortcut functions ... */
extern int vstr_nx_sc_fmt_cb_beg(struct Vstr_base *, size_t *,
                              struct Vstr_fmt_spec *, size_t *, unsigned int)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_cb_end(struct Vstr_base *, size_t,
                              struct Vstr_fmt_spec *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_vstr(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_buf(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_ptr(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_non(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_ref(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_bkmg_Byte_uint(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_bkmg_bit_uint(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_bkmg_Bytes_uint(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_bkmg_bits_uint(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern int vstr_nx_sc_fmt_add_ipv4_ptr(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern int vstr_nx_sc_fmt_add_ipv6_ptr(struct Vstr_conf *, const char *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

extern int vstr_nx_sc_mmap_fd(struct Vstr_base *, size_t, int, 
                           VSTR_AUTOCONF_off64_t, size_t, unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern int vstr_nx_sc_mmap_file(struct Vstr_base *, size_t,
                             const char *, VSTR_AUTOCONF_off64_t, size_t, 
                             unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 3)) VSTR__ATTR_H() ;
extern int vstr_nx_sc_read_iov_fd(struct Vstr_base *, size_t, int,
                               unsigned int, unsigned int,
                               unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern int vstr_nx_sc_read_len_fd(struct Vstr_base *, size_t, int,
                               size_t, unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern int vstr_nx_sc_read_iov_file(struct Vstr_base *, size_t,
           	                 const char *, VSTR_AUTOCONF_off64_t, 
                                 unsigned int, unsigned int,
                                 unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 3)) VSTR__ATTR_H() ;
extern int vstr_nx_sc_read_len_file(struct Vstr_base *, size_t,
           	                 const char *, VSTR_AUTOCONF_off64_t, size_t,
                                 unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 3)) VSTR__ATTR_H() ;
extern int vstr_nx_sc_write_fd(struct Vstr_base *, size_t, size_t, int,
                            unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1)) VSTR__ATTR_H() ;
extern int vstr_nx_sc_write_file(struct Vstr_base *, size_t, size_t,
                              const char *, int, VSTR_AUTOCONF_mode_t,
                              VSTR_AUTOCONF_off64_t, unsigned int *)
    VSTR__COMPILE_ATTR_NONNULL_L((1, 4)) VSTR__ATTR_H() ;

extern void vstr_nx_sc_basename(const struct Vstr_base *, size_t, size_t,
                             size_t *, size_t *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
extern void vstr_nx_sc_dirname(const struct Vstr_base *, size_t, size_t,
                            size_t *)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;


/* == inline helper functions == */
/* indented because they aren't documented */
 extern int vstr_nx_extern_inline_add_buf(struct Vstr_base *, size_t,
                                       const void *, size_t)
     VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern int vstr_nx_extern_inline_del(struct Vstr_base *, size_t, size_t)
     VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern int vstr_nx_extern_inline_sects_add(struct Vstr_sects *, size_t, size_t)
     VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern int vstr_nx_extern_inline_add_rep_chr(struct Vstr_base *, size_t, 
                                           char, size_t)
     VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

 extern void *vstr_nx_wrap_memcpy(void *, const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern int   vstr_nx_wrap_memcmp(const void *, const void *, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern void *vstr_nx_wrap_memchr(const void *, int, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern void *vstr_nx_wrap_memrchr(const void *, int, size_t)
    VSTR__COMPILE_ATTR_PURE() VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern void *vstr_nx_wrap_memset(void *, int, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;
 extern void *vstr_nx_wrap_memmove(void *, const void *, size_t)
    VSTR__COMPILE_ATTR_NONNULL_A() VSTR__ATTR_H() ;

