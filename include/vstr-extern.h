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

/* == macro functions == */
#define VSTR_DECL_INIT_SECTS(sz, ptr) { 0, (sz), \
 0, 0, 0, 0, \
 \
 1, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 0, 0, 0, 0, \
 ptr}

#define VSTR_DECL_SECTS(pre, name, sz) \
 pre struct Vstr_sect_node vstr__sect_array__ ## name [sz]; \
 pre struct Vstr_sects name = VSTR_DECL_INIT_SECTS(sz, vstr__sect_array__ ## name)

#define VSTR_SECTS_NUM(sects, off) ((sects)->ptr[(off) - 1])

/* == real functions == */

/* not really vectored string functions ... just stuff needed */
extern void vstr_ref_cb_free_nothing(struct Vstr_ref *);
extern void vstr_ref_cb_free_ref(struct Vstr_ref *);
extern void vstr_ref_cb_free_ptr(struct Vstr_ref *);
extern void vstr_ref_cb_free_ptr_ref(struct Vstr_ref *);

extern struct Vstr_ref *vstr_ref_add(struct Vstr_ref *);
extern void vstr_ref_del(struct Vstr_ref *);

/* real start of vector string functions */

extern int vstr_init(void);

extern struct Vstr_conf *vstr_make_conf(void);
extern void vstr_free_conf(struct Vstr_conf *);

extern int vstr_init_base(struct Vstr_conf *, struct Vstr_base *);
extern struct Vstr_base *vstr_make_base(struct Vstr_conf *);
extern void vstr_free_base(struct Vstr_base *);

extern unsigned int vstr_add_spare_nodes(struct Vstr_conf *,
                                         unsigned int,
                                         unsigned int);
extern unsigned int vstr_del_spare_nodes(struct Vstr_conf *,
                                         unsigned int,
                                         unsigned int);

/* add data functions */
extern int vstr_add_buf(struct Vstr_base *, size_t, const void *, size_t);
extern int vstr_add_ptr(struct Vstr_base *, size_t, const void *, size_t);
extern int vstr_add_non(struct Vstr_base *, size_t, size_t);
extern int vstr_add_ref(struct Vstr_base *, size_t,
                        struct Vstr_ref *, size_t, size_t);
extern int vstr_add_vstr(struct Vstr_base *, size_t,
                         const struct Vstr_base *, size_t, size_t,
                         unsigned int);

extern struct Vstr_base *vstr_dup_buf(struct Vstr_conf *, const void *, size_t);
extern struct Vstr_base *vstr_dup_ptr(struct Vstr_conf *, const void *, size_t);
extern struct Vstr_base *vstr_dup_non(struct Vstr_conf *, size_t);
extern struct Vstr_base *vstr_dup_vstr(struct Vstr_conf *,
                                       const struct Vstr_base *, size_t, size_t,
                                       unsigned int);

extern size_t vstr_add_vfmt(struct Vstr_base *, size_t, const char *, va_list)
    __attribute__ ((__format__ (__printf__, 3, 0)));
extern size_t vstr_add_fmt(struct Vstr_base *, size_t, const char *, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));

extern size_t vstr_add_iovec_buf_beg(struct Vstr_base *, size_t,
                                     unsigned int, unsigned int,
                                     struct iovec **,
                                     unsigned int *);
extern void vstr_add_iovec_buf_end(struct Vstr_base *, size_t, size_t);

/* NOTE: netstr2 allows leading '0' s, netstr doesn't */
extern size_t vstr_add_netstr_beg(struct Vstr_base *, size_t);
extern int vstr_add_netstr_end(struct Vstr_base *, size_t, size_t);
extern size_t vstr_add_netstr2_beg(struct Vstr_base *, size_t);
extern int vstr_add_netstr2_end(struct Vstr_base *, size_t, size_t);

/* delete functions */
extern int vstr_del(struct Vstr_base *, size_t, size_t);

/* substitution functions */
extern int vstr_sub_buf(struct Vstr_base *, size_t, size_t,
                        const void *, size_t);
extern int vstr_sub_ptr(struct Vstr_base *, size_t, size_t,
                        const void *, size_t);
extern int vstr_sub_non(struct Vstr_base *, size_t, size_t, size_t);
extern int vstr_sub_ref(struct Vstr_base *, size_t, size_t,
                        struct Vstr_ref *, size_t, size_t);
extern int vstr_sub_vstr(struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t,
                         unsigned int);

/* convertion functions */
extern int vstr_conv_lowercase(struct Vstr_base *, size_t, size_t);
extern int vstr_conv_uppercase(struct Vstr_base *, size_t, size_t);
extern int vstr_conv_unprintable(struct Vstr_base *, size_t, size_t,
                                 unsigned int, char);


/* move functions */
extern int vstr_mov(struct Vstr_base *, size_t,
                    struct Vstr_base *, size_t, size_t);

/* control misc stuff */
extern int vstr_cntl_opt(int, ...);
extern int vstr_cntl_base(struct Vstr_base *, int, ...);
extern int vstr_cntl_conf(struct Vstr_conf *, int, ...);

/* --------------------------------------------------------------------- */
/* constant base functions below here */
/* --------------------------------------------------------------------- */

/* comparing functions */
extern int vstr_cmp(const struct Vstr_base *, size_t, size_t,
                    const struct Vstr_base *, size_t, size_t);
extern int vstr_cmp_buf(const struct Vstr_base *, size_t, size_t,
                        const void *, size_t);
extern int vstr_cmp_case(const struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t);
extern int vstr_cmp_vers(const struct Vstr_base *, size_t, size_t,
                         const struct Vstr_base *, size_t, size_t);

/* search functions */
extern size_t vstr_srch_chr_fwd(const struct Vstr_base *, size_t, size_t, char);
extern size_t vstr_srch_chr_rev(const struct Vstr_base *, size_t, size_t, char);

extern size_t vstr_srch_buf_fwd(const struct Vstr_base *, size_t, size_t,
                                const void *, size_t);
extern size_t vstr_srch_buf_rev(const struct Vstr_base *, size_t, size_t,
                                const void *, size_t);

extern size_t vstr_srch_vstr_fwd(const struct Vstr_base *, size_t, size_t,
                                 const struct Vstr_base *, size_t, size_t);
extern size_t vstr_srch_vstr_rev(const struct Vstr_base *, size_t, size_t,
                                 const struct Vstr_base *, size_t, size_t);

extern size_t vstr_srch_netstr_fwd(const struct Vstr_base *, size_t, size_t,
                                   size_t *, size_t *);
extern size_t vstr_srch_netstr2_fwd(const struct Vstr_base *, size_t, size_t,
                                    size_t *, size_t *);

/* spanning and compliment spanning */
extern size_t vstr_spn_buf_fwd(const struct Vstr_base *, size_t, size_t,
                               const char *, size_t);
extern size_t vstr_spn_buf_rev(const struct Vstr_base *, size_t, size_t,
                               const char *, size_t);
extern size_t vstr_cspn_buf_fwd(const struct Vstr_base *, size_t, size_t,
                                const char *, size_t);
extern size_t vstr_cspn_buf_rev(const struct Vstr_base *, size_t, size_t,
                                const char *, size_t);


/* export functions */
extern size_t vstr_export_iovec_ptr_all(const struct Vstr_base *,
                                        struct iovec **,
                                        unsigned int *);
extern size_t vstr_export_iovec_cpy_buf(const struct Vstr_base *,
                                        size_t, size_t,
                                        struct iovec *, unsigned int,
                                        unsigned int *);
extern size_t vstr_export_iovec_cpy_ptr(const struct Vstr_base *,
                                        size_t, size_t,
                                        struct iovec *, unsigned int,
                                        unsigned int *);
extern size_t vstr_export_buf(const struct Vstr_base *, size_t, size_t, void *);
extern char vstr_export_chr(const struct Vstr_base *, size_t);

extern short vstr_parse_short(const struct Vstr_base *,
                              size_t, size_t, unsigned int, size_t *,
                              unsigned int *);
extern unsigned short vstr_parse_ushort(const struct Vstr_base *,
                                        size_t, size_t,
                                        unsigned int, size_t *,
                                        unsigned int *);
extern int vstr_parse_int(const struct Vstr_base *,
                          size_t, size_t, unsigned int, size_t *,
                          unsigned int *);
extern unsigned int vstr_parse_uint(const struct Vstr_base *,
                                    size_t, size_t, unsigned int, size_t *,
                                    unsigned int *);
extern long vstr_parse_long(const struct Vstr_base *,
                            size_t, size_t, unsigned int, size_t *,
                            unsigned int *);
extern unsigned long vstr_parse_ulong(const struct Vstr_base *,
                                      size_t, size_t, unsigned int, size_t *,
                                      unsigned int *);
extern VSTR_AUTOCONF_intmax_t vstr_parse_intmax(const struct Vstr_base *,
                                                size_t, size_t, unsigned int,
                                                size_t *, unsigned int *);
extern VSTR_AUTOCONF_uintmax_t vstr_parse_uintmax(const struct Vstr_base *,
                                                  size_t, size_t,
                                                  unsigned int, size_t *,
                                                  unsigned int *);

extern char *vstr_export_cstr_ptr(const struct Vstr_base *, size_t, size_t);
extern void vstr_export_cstr_buf(const struct Vstr_base *, size_t, size_t,
                                 void *, size_t);
extern struct Vstr_ref *vstr_export_cstr_ref(const struct Vstr_base *,
                                             size_t, size_t);
/* section functions */
extern struct Vstr_sects *vstr_make_sects(unsigned int);
extern void vstr_free_sects(struct Vstr_sects *);

extern int vstr_sects_add(struct Vstr_sects *, size_t, size_t);
extern int vstr_sects_del(struct Vstr_sects *, unsigned int);

extern unsigned int vstr_sects_srch(struct Vstr_sects *, size_t, size_t);

extern int vstr_sects_foreach(struct Vstr_base *, struct Vstr_sects *,
                              unsigned int,
                              int (*) (const struct Vstr_base *, size_t, size_t,
                                       void *),
                              void *);

/* split functions */
extern unsigned int vstr_split_buf(const struct Vstr_base *, size_t, size_t,
                                   const void *, size_t, struct Vstr_sects *,
                                   unsigned int, unsigned int);

/* shortcut functions ... */
extern int vstr_sc_add_fd(struct Vstr_base *, size_t, int, off_t, size_t,
                          unsigned int *);
extern int vstr_sc_add_file(struct Vstr_base *, size_t, const char *,
                            unsigned int *);
extern int vstr_sc_read_fd(struct Vstr_base *, size_t, int,
                           unsigned int, unsigned int,
                           unsigned int *);
extern int vstr_sc_write_fd(struct Vstr_base *, size_t, size_t, int,
                            unsigned int *);
extern int vstr_sc_write_file(struct Vstr_base *, size_t, size_t,
                              const char *, int, int, unsigned int *);

    
