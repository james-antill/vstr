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

/* not really vectored string functions ... just stuff needed */
extern void vstr_ref_cb_free_nothing(struct Vstr_ref *);
extern void vstr_ref_cb_free_ref(struct Vstr_ref *);
extern void vstr_ref_cb_free_ptr(struct Vstr_ref *);
extern void vstr_ref_cb_free_ptr_ref(struct Vstr_ref *);

extern struct Vstr_ref *vstr_ref_add_ref(struct Vstr_ref *);
extern void vstr_ref_del_ref(struct Vstr_ref *);

/* real start of vector string functions */

extern int vstr_init(void);

extern struct Vstr_conf *vstr_make_conf(void);

extern int vstr_init_base(struct Vstr_base *, struct Vstr_conf *);
extern struct Vstr_base *vstr_make_base(struct Vstr_conf *);
extern void vstr_free_base(struct Vstr_base *);

extern unsigned int vstr_add_spare_nodes(struct Vstr_conf *,
                                         unsigned int,
                                         unsigned int);
extern unsigned int vstr_del_spare_nodes(struct Vstr_conf *,
                                         unsigned int,
                                         unsigned int);

extern int vstr_add_buf(struct Vstr_base *, size_t, const void *, size_t);
extern int vstr_add_ptr(struct Vstr_base *, size_t, const void *, size_t);
extern int vstr_add_non(struct Vstr_base *, size_t, size_t);
extern int vstr_add_ref(struct Vstr_base *, size_t,
                        struct Vstr_ref *, size_t, size_t);
extern int vstr_add_vstr(struct Vstr_base *, size_t,
                         struct Vstr_base *, size_t, size_t, unsigned int);

extern struct Vstr_base *vstr_dup_buf(struct Vstr_conf *, const void *, size_t);
extern struct Vstr_base *vstr_dup_ptr(struct Vstr_conf *, const void *, size_t);
extern struct Vstr_base *vstr_dup_non(struct Vstr_conf *, size_t);
extern struct Vstr_base *vstr_dup_vstr(struct Vstr_conf *,
                                       struct Vstr_base *, size_t, size_t,
                                       unsigned int);

extern size_t vstr_add_vfmt(struct Vstr_base *base, size_t,
                            const char *fmt, va_list ap)
    __attribute__ ((__format__ (__printf__, 3, 0)));
extern size_t vstr_add_fmt(struct Vstr_base *base, size_t,
                           const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));

extern size_t vstr_add_iovec_buf_beg(struct Vstr_base *base,
                                     unsigned int min, unsigned int max,
                                     struct iovec **ret_iovs,
                                     unsigned int *num);
extern void vstr_add_iovec_buf_end(struct Vstr_base *base, size_t bytes);

extern size_t vstr_add_netstr_beg(struct Vstr_base *);
extern int vstr_add_netstr_end(struct Vstr_base *, size_t);

/* netstr2 allows leading '0' s */
extern size_t vstr_add_netstr2_beg(struct Vstr_base *);
extern int vstr_add_netstr2_end(struct Vstr_base *, size_t);

extern int vstr_del(struct Vstr_base *, size_t, size_t);

extern int vstr_sub_buf(struct Vstr_base *, size_t, size_t,
                        const void *, size_t);
extern int vstr_sub_ptr(struct Vstr_base *, size_t, size_t,
                        const void *, size_t);
extern int vstr_sub_non(struct Vstr_base *, size_t, size_t, size_t);
extern int vstr_sub_ref(struct Vstr_base *, size_t, size_t,
                        struct Vstr_ref *, size_t, size_t);
extern int vstr_sub_vstr(struct Vstr_base *, size_t, size_t,
                         struct Vstr_base *, size_t, size_t, unsigned int);

/* Control misc stuff ... */
extern int vstr_cntl_opt(int option, ...);
extern int vstr_cntl_base(struct Vstr_base *, int option, ...);
extern int vstr_cntl_conf(struct Vstr_conf *, int option, ...);

/* --------------------------------------------------------------------- */
/* constant base functions below here */
/* --------------------------------------------------------------------- */

extern int vstr_cmp(const struct Vstr_base *, size_t, size_t,
                    const struct Vstr_base *, size_t, size_t);
extern int vstr_cmp_buf(const struct Vstr_base *, size_t, size_t,
                        const void *, size_t);
extern int vstr_cmp_case(const struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t);
extern int vstr_cmp_vers(const struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t);

extern size_t vstr_srch_chr_fwd(const struct Vstr_base *, size_t, size_t, int);
extern size_t vstr_srch_chr_rev(const struct Vstr_base *, size_t, size_t, int);

extern size_t vstr_srch_buf_fwd(const struct Vstr_base *, size_t, size_t,
                                const void *, size_t);
extern size_t vstr_srch_buf_rev(const struct Vstr_base *, size_t, size_t,
                                const void *, size_t);

extern size_t vstr_srch_vstr_fwd(const struct Vstr_base *, size_t, size_t,
                                 const struct Vstr_base *, size_t, size_t);
extern size_t vstr_srch_vstr_rev(const struct Vstr_base *, size_t, size_t,
                                 const struct Vstr_base *, size_t, size_t);

extern size_t vstr_spn_buf_fwd(const struct Vstr_base *, size_t, size_t,
                               const char *, size_t);
extern size_t vstr_spn_buf_rev(const struct Vstr_base *, size_t, size_t,
                               const char *, size_t);
extern size_t vstr_cspn_buf_fwd(const struct Vstr_base *, size_t, size_t,
                                const char *, size_t);
extern size_t vstr_cspn_buf_rev(const struct Vstr_base *, size_t, size_t,
                                const char *, size_t);

extern size_t vstr_srch_netstr_fwd(const struct Vstr_base *, size_t, size_t,
                                   size_t *, size_t *);
extern size_t vstr_srch_netstr2_fwd(const struct Vstr_base *, size_t, size_t,
                                    size_t *, size_t *);

extern size_t vstr_export_iovec_ptr_all(const struct Vstr_base *,
                                        struct iovec **,
                                        unsigned int *);
/* extern size_t vstr_export_iovec_cpy(struct Vstr_base *,
 *                                     size_t, size_t, struct iovec *,
 *                                     unsigned int, unsigned int *); */
extern size_t vstr_export_buf(const struct Vstr_base *, size_t, size_t, void *);
extern char vstr_export_chr(const struct Vstr_base *, size_t);

extern char *vstr_export_cstr_ptr(const struct Vstr_base *,
                                  size_t, size_t);
extern struct Vstr_ref *vstr_export_cstr_ref(const struct Vstr_base *,
                                             size_t, size_t);
