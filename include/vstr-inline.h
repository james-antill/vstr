
#ifndef VSTR__HEADER_H
# error " You must _just_ #include <vstr.h>"
#endif
/*
 *  Copyright (C) 2002, 2003  James Antill
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
/* exported functions which are inlined */
/* NOTE: this implementation can change when the ABI changes ... DO NOT use
 * undocumented knowledge from here */

#ifdef VSTR_AUTOCONF_USE_WRAP_MEMCPY
extern inline void *vstr_wrap_memcpy(void *dest, const void *src, size_t n)
{
  switch (n)
  {
    case 7:  ((unsigned char *)dest)[6] = ((const unsigned char *)src)[6];
    case 6:  ((unsigned char *)dest)[5] = ((const unsigned char *)src)[5];
    case 5:  ((unsigned char *)dest)[4] = ((const unsigned char *)src)[4];
    case 4:  ((unsigned char *)dest)[3] = ((const unsigned char *)src)[3];
    case 3:  ((unsigned char *)dest)[2] = ((const unsigned char *)src)[2];
    case 2:  ((unsigned char *)dest)[1] = ((const unsigned char *)src)[1];
    case 1:  ((unsigned char *)dest)[0] = ((const unsigned char *)src)[0];
      break;
    default: memcpy(dest, src, n);
      break;
  }
  
  return (dest);
}
#else
# define vstr_wrap_memcpy(x, y, z)  memcpy(x, y, z)
#endif

#ifdef VSTR_AUTOCONF_USE_WRAP_MEMCMP
extern inline int vstr_wrap_memcmp(const void *passed_s1,
                                   const void *passed_s2, size_t n)
{
  const unsigned char *s1 = passed_s1;
  const unsigned char *s2 = passed_s2;
  int ret = 0;
  int tmp = 0;

  switch (n)
  {
    case 7:  tmp = s1[6] - s2[6]; if (tmp) ret = tmp;
    case 6:  tmp = s1[5] - s2[5]; if (tmp) ret = tmp;
    case 5:  tmp = s1[4] - s2[4]; if (tmp) ret = tmp;
    case 4:  tmp = s1[3] - s2[3]; if (tmp) ret = tmp;
    case 3:  tmp = s1[2] - s2[2]; if (tmp) ret = tmp;
    case 2:  tmp = s1[1] - s2[1]; if (tmp) ret = tmp;
    case 1:  tmp = s1[0] - s2[0]; if (tmp) ret = tmp;
      break;
    default: ret = memcmp(s1, s2, n);
      break;
  }
  
  return (ret);
}
#else
# define vstr_wrap_memcmp(x, y, z)  memcmp(x, y, z)
#endif

#ifdef VSTR_AUTOCONF_USE_WRAP_MEMCHR
extern inline void *vstr_wrap_memchr(const void *passed_s1, int c, size_t n)
{
  const unsigned char *s1 = passed_s1;
  const void *ret = 0;
  int tmp = 0;

  switch (n)
  {
    case 7:  tmp = s1[6] == c; if (tmp) ret = s1 + 6;
    case 6:  tmp = s1[5] == c; if (tmp) ret = s1 + 5;
    case 5:  tmp = s1[4] == c; if (tmp) ret = s1 + 4;
    case 4:  tmp = s1[3] == c; if (tmp) ret = s1 + 3;
    case 3:  tmp = s1[2] == c; if (tmp) ret = s1 + 2;
    case 2:  tmp = s1[1] == c; if (tmp) ret = s1 + 1;
    case 1:  tmp = s1[0] == c; if (tmp) ret = s1 + 0;
      break;
    default: ret = memchr(s1, c, n);
      break;
  }
  
  return ((void *)ret);
}
#else
# define vstr_wrap_memchr(x, y, z)  memchr(x, y, z)
#endif

#ifdef VSTR_AUTOCONF_USE_WRAP_MEMSET
extern inline void *vstr_wrap_memset(void *dest, int c, size_t n)
{
  switch (n)
  {
    case 7:  ((unsigned char *)dest)[6] = c;
    case 6:  ((unsigned char *)dest)[5] = c;
    case 5:  ((unsigned char *)dest)[4] = c;
    case 4:  ((unsigned char *)dest)[3] = c;
    case 3:  ((unsigned char *)dest)[2] = c;
    case 2:  ((unsigned char *)dest)[1] = c;
    case 1:  ((unsigned char *)dest)[0] = c;
      break;
    default: memset(dest, c, n);
      break;
  }
  
  return (dest);
}
#else
# define vstr_wrap_memset(x, y, z)  memset(x, y, z)
#endif

#ifdef VSTR_AUTOCONF_USE_WRAP_MEMMOVE
extern inline void *vstr_wrap_memmove(void *dest, const void *src, size_t n)
{
  if (n < 8)
  {
    unsigned char tmp[8];
    vstr_wrap_memcpy(tmp,  src, n);
    vstr_wrap_memcpy(dest, tmp, n);
    return (dest);
  }

  return memmove(dest, src, n);
}
#else
# define vstr_wrap_memmove(x, y, z) memmove(x, y, z)
#endif

/* needed at the top so vstr_del() etc. can use it */
extern inline void vstr_ref_del(struct Vstr_ref *tmp)
{
  if (!tmp)
    return; /* std. free semantics */
  
  if (!--tmp->ref)
    (*tmp->func)(tmp);
}

extern inline struct Vstr_ref *vstr_ref_add(struct Vstr_ref *tmp)
{
  ++tmp->ref;
  
  return (tmp);
}

extern inline void *vstr_cache_get(const struct Vstr_base *base,
                                   unsigned int pos)
{
  if (!pos)
    return ((void *)0);

  if (!base->cache_available || !VSTR__CACHE(base))
    return ((void *)0);
  
  --pos;
  
  if (pos >= VSTR__CACHE(base)->sz)
    return ((void *)0);
  
  return (VSTR__CACHE(base)->data[pos]);
}

extern inline
int vstr_cache__pos(const struct Vstr_base *base,
                    struct Vstr_node *node, size_t pos, unsigned int num)
{
  struct Vstr__cache_data_pos *data = (struct Vstr__cache_data_pos *)0;
  
  if (!base->cache_available)
    return (0 /* FALSE */);

  if (!(data = (struct Vstr__cache_data_pos *)vstr_cache_get(base, 1)) &&
      !(data = vstr_extern_inline_make_cache_pos(base)))
    return (0 /* FALSE */);
  
  data->node = node;
  data->pos = pos;
  data->num = num;
  
  return (1 /* TRUE */);
}

extern inline
struct Vstr_node *vstr_base__pos(const struct Vstr_base *base,
                                 size_t *pos, unsigned int *num, int cache)
{
  size_t orig_pos = *pos;
  struct Vstr_node *scan = base->beg;
  struct Vstr__cache_data_pos *data = (struct Vstr__cache_data_pos *)0;
  unsigned int dummy_num = 0;

  if (!num) num = &dummy_num;
  
  *pos += base->used;
  *num = 1;
  
  if (*pos <= base->beg->len)
    return (base->beg);
  
  /* must be more than one node */
  
  if (orig_pos > (base->len - base->end->len))
  {
    *pos = orig_pos - (base->len - base->end->len);
    *num = base->num;
    return (base->end);
  }
  
  if ((data = (struct Vstr__cache_data_pos *)vstr_cache_get(base, 1)) &&
      data->node && (data->pos <= orig_pos))
  {
    scan = data->node;
    *num = data->num;
    *pos = (orig_pos - data->pos) + 1;
  }
  
  while (*pos > scan->len)
  {
    *pos -= scan->len;
    
    scan = scan->next;
    ++*num;
  }
  
  if (cache)
    vstr_cache__pos(base, scan, (orig_pos - *pos) + 1, *num);
 
  return (scan);
}

extern inline char *vstr_export__node_ptr(const struct Vstr_node *node)
{
  switch (node->type)
  {
    case VSTR_TYPE_NODE_BUF:
      return (((struct Vstr_node_buf *)node)->buf);
    case VSTR_TYPE_NODE_PTR:
      return (char *)(((const struct Vstr_node_ptr *)node)->ptr);
    case VSTR_TYPE_NODE_REF:
      return (((char *)((const struct Vstr_node_ref *)node)->ref->ptr) +
              ((const struct Vstr_node_ref *)node)->off);
    default: /* assert */
    case VSTR_TYPE_NODE_NON:
      break;
  }
  
  return ((char *)0);
}

extern inline char vstr_export_chr(const struct Vstr_base *base, size_t pos)
{
  struct Vstr_node *node = vstr_base__pos(base, &pos, (unsigned int *)0, 1);

  /* errors, requests for data from NON nodes and real data are all == 0 */
  if (!node) return (0);

  return (*(vstr_export__node_ptr(node) + --pos));
}

extern inline int vstr_iter_fwd_beg(const struct Vstr_base *base,
                                    size_t pos, size_t len,
                                    struct Vstr_iter *iter)
{
  /*
   * assert(base && num && len);
   *  
   * assert(pos && (((pos <= base->len) &&
   *        ((pos + *len - 1) <= base->len)) || !*len));
   */

  iter->node = (struct Vstr_node *)0;
  if ((pos > base->len) || !len)
    return (0);

  if ((pos + len - 1) > base->len)
    len = base->len - (pos - 1);
  
  iter->node = vstr_base__pos(base, &pos, &iter->num, 1);
  --pos;
  
  iter->len = iter->node->len - pos;
  if (iter->len > len)
    iter->len = len;
  len -= iter->len;
  
  iter->remaining = len;
    
  iter->ptr = (char *)0;
  if (iter->node->type != VSTR_TYPE_NODE_NON)
    iter->ptr = vstr_export__node_ptr(iter->node) + pos;
  
  return (1);
}

extern inline int vstr_iter_fwd_nxt(struct Vstr_iter *iter)
{
  /*
   *
   * assert(base && num && len);
   * assert(scan);
   *
   */
  
  if (!iter->remaining)
  {
    iter->node = (struct Vstr_node *)0;
    return (0);
  }

  iter->node = iter->node->next;
  ++iter->num;
  
  iter->len = iter->node->len;
  
  if (iter->len > iter->remaining)
    iter->len = iter->remaining; 
  iter->remaining -= iter->len;
  
  iter->ptr = (char *)0;
  if (iter->node->type != VSTR_TYPE_NODE_NON)
    iter->ptr = vstr_export__node_ptr(iter->node);
  
  return (1);
}

extern inline unsigned int vstr_num(const struct Vstr_base *base,
                                    size_t pos, size_t len)
{
  struct Vstr_iter dummy_iter;
  struct Vstr_iter *iter = &dummy_iter;
  unsigned int beg_num = 0;
  
  if (pos == 1 && len == base->len)
    return (base->num);

  if (!vstr_iter_fwd_beg(base, pos, len, iter))
    return (0);

  beg_num = iter->num;
  while (vstr_iter_fwd_nxt(iter))
  { /* do nothing */; }
  
  return ((iter->num - beg_num) + 1);
}

extern inline int vstr_add_buf(struct Vstr_base *base, size_t pos,
                               const void *buffer, size_t len)
{
  if (!len) return (1);

  if (base->len && (pos == base->len) &&
      (base->end->type == VSTR_TYPE_NODE_BUF) &&
      (len <= (base->conf->buf_sz - base->end->len)) &&
      (!base->cache_available || base->cache_internal))
  {
    struct Vstr_node *scan = base->end;

    vstr_wrap_memcpy(((struct Vstr_node_buf *)scan)->buf + scan->len, buffer, len);
    scan->len += len;
    base->len += len;

    if (base->iovec_upto_date)
    {
      unsigned int num = base->num + VSTR__CACHE(base)->vec->off - 1;
      VSTR__CACHE(base)->vec->v[num].iov_len += len;
    }
    
    return (1);
  }

  return (vstr_extern_inline_add_buf(base, pos, buffer, len));
}

extern inline size_t vstr_sc_posdiff(size_t beg_pos, size_t end_pos)
{
  return ((end_pos - beg_pos) + 1);
}

extern inline int vstr_del(struct Vstr_base *base, size_t pos, size_t len)
{
  if (!len)
    return (1);

  if (pos > base->len)
    return (0);

  if (!base->cache_available || base->cache_internal)
  {
    if ((pos == 1) && ((len + base->used) < base->beg->len))
    { /* delete from beginning, in one node */
      struct Vstr_node *scan = base->beg;
      void *data = (void *)0;
      
      base->len -= len;
      
      switch (scan->type)
      {
        case VSTR_TYPE_NODE_BUF:
          base->used += len;
          break;
        case VSTR_TYPE_NODE_NON:
          scan->len -= len;
          break;
        case VSTR_TYPE_NODE_PTR:
        {
          char *tmp = (char *)((struct Vstr_node_ptr *)scan)->ptr;
          ((struct Vstr_node_ptr *)scan)->ptr = tmp + len;
          scan->len -= len;
        }
        break;
        case VSTR_TYPE_NODE_REF:
          ((struct Vstr_node_ref *)scan)->off += len;
          scan->len -= len;
          break;
      }
      
      if ((data = vstr_cache_get(base, 3)))
      {
        struct Vstr__cache_data_cstr *pdata = (struct Vstr__cache_data_cstr *)0;

        pdata = (struct Vstr__cache_data_cstr *)data;

        if (vstr_sc_posdiff(1, pdata->pos) > len)
          pdata->pos += len;
        else
        {
          pdata->len -= (len - (vstr_sc_posdiff(1, pdata->pos) - 1));
          pdata->pos = 1;
        }
      }
      if (base->iovec_upto_date)
      {
        unsigned int num = 1 + VSTR__CACHE(base)->vec->off - 1;
        
        if (scan->type != VSTR_TYPE_NODE_NON)
        {
          char *tmp = (char *)VSTR__CACHE(base)->vec->v[num].iov_base;
          tmp += len;
          VSTR__CACHE(base)->vec->v[num].iov_base = tmp;
        }
        VSTR__CACHE(base)->vec->v[num].iov_len -= len;
      }
      if ((data = vstr_cache_get(base, 1)))
      {
        struct Vstr__cache_data_pos *pdata = (struct Vstr__cache_data_pos *)0;
        
        pdata = (struct Vstr__cache_data_pos *)data;
        pdata->node = (struct Vstr_node *)0;
      }
      
      return (1);
    }

    if ((pos > (base->len - (base->end->len - 1))) &&
        (len == vstr_sc_posdiff(pos, base->len)))
    { /* delete from end, in one node */
      struct Vstr_node *scan = base->end;
      void *data = (void *)0;
      
      base->len -= len;
      scan->len -= len;

      if ((data = vstr_cache_get(base, 3)))
      {
        struct Vstr__cache_data_cstr *pdata = (struct Vstr__cache_data_cstr *)0;

        pdata = (struct Vstr__cache_data_cstr *)data;
        
        vstr_ref_del(pdata->ref);
        pdata->ref = (struct Vstr_ref *)0;
      }
      if (base->iovec_upto_date)
      {
        unsigned int num = base->num + VSTR__CACHE(base)->vec->off - 1;
        
        VSTR__CACHE(base)->vec->v[num].iov_len -= len;
      }
      if ((data = vstr_cache_get(base, 1)))
      {
        struct Vstr__cache_data_pos *pdata = (struct Vstr__cache_data_pos *)0;
        
        pdata = (struct Vstr__cache_data_pos *)data;
        pdata->node = (struct Vstr_node *)0;
      }
      
      return (1);
    }
  }

  return (vstr_extern_inline_del(base, pos, len));
}

extern inline int vstr_sc_reduce(struct Vstr_base *base,
                                 size_t pos, size_t len, size_t reduce)
{ /* assert len >= reduce */
  if (!len)
    return (1); /* TRUE */
  
  if (reduce > len)
    reduce = len;
  
  return (vstr_del(base, pos + (len - reduce), reduce));
}

extern inline int vstr_sects_add(struct Vstr_sects *sects,
                                 size_t pos, size_t len)
{
  if (!sects->sz || (sects->num >= sects->sz))
  {
    if (!sects->can_add_sz)
      return (0);
    
    if (!vstr_extern_inline_sects_add(sects, pos, len))
      return (0);
  }
  
  sects->ptr[sects->num].pos = pos;
  sects->ptr[sects->num].len = len;
  ++sects->num;
  
  return (1);
}

extern inline int vstr_add_rep_chr(struct Vstr_base *base, size_t pos,
                                   char chr, size_t len)
{ /* almost embarassingly similar to add_buf */
  if (!len) return (1);

  if (base->len && (pos == base->len) &&
      (base->end->type == VSTR_TYPE_NODE_BUF) &&
      (len <= (base->conf->buf_sz - base->end->len)) &&
      (!base->cache_available || base->cache_internal))
  {
    struct Vstr_node *scan = base->end;

    vstr_wrap_memset(((struct Vstr_node_buf *)scan)->buf + scan->len, chr, len);
    scan->len += len;
    base->len += len;

    if (base->iovec_upto_date)
    {
      unsigned int num = base->num + VSTR__CACHE(base)->vec->off - 1;
      VSTR__CACHE(base)->vec->v[num].iov_len += len;
    }
    
    return (1);
  }

  return (vstr_extern_inline_add_rep_chr(base, pos, chr, len));
}

/* do inline versions of macro functions */
/* cmp */
extern inline int vstr_cmp_eq(const struct Vstr_base *s1, size_t p1, size_t l1,
                              const struct Vstr_base *s2, size_t p2, size_t l2)
{ return ((l1 == l2) && !vstr_cmp(s1, p1, l1, s2, p2, l1)); }
extern inline int vstr_cmp_buf_eq(const struct Vstr_base *s1,
                                  size_t p1, size_t l1,
                                  const void *buf, size_t buf_len)
{ return ((l1 == buf_len) && !vstr_cmp_buf(s1, p1, l1, buf, buf_len)); }
extern inline int vstr_cmp_cstr(const struct Vstr_base *s1,
                                size_t p1, size_t l1,
                                const char *buf)
{ return (vstr_cmp_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_cstr_eq(const struct Vstr_base *s1,
                                   size_t p1, size_t l1,
                                   const char *buf)
{ return (vstr_cmp_buf_eq(s1, p1, l1, buf, strlen(buf))); }

/* cmp case */
extern inline int vstr_cmp_case_eq(const struct Vstr_base *s1,
                                   size_t p1, size_t l1,
                                   const struct Vstr_base *s2,
                                   size_t p2, size_t l2)
{ return ((l1 == l2) && !vstr_cmp_case(s1, p1, l1, s2, p2, l1)); }
extern inline int vstr_cmp_case_buf_eq(const struct Vstr_base *s1,
                                       size_t p1, size_t l1,
                                       const char *buf, size_t buf_len)
{ return ((l1 == buf_len) && !vstr_cmp_case_buf(s1, p1, l1, buf, buf_len)); }
extern inline int vstr_cmp_case_cstr(const struct Vstr_base *s1,
                                     size_t p1, size_t l1,
                                     const char *buf)
{ return (vstr_cmp_case_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_case_cstr_eq(const struct Vstr_base *s1,
                                        size_t p1, size_t l1, const char *buf)
{ return (vstr_cmp_case_buf_eq(s1, p1, l1, buf, strlen(buf))); }

/* cmp vers */
extern inline int vstr_cmp_vers_eq(const struct Vstr_base *s1,
                                   size_t p1, size_t l1,
                                   const struct Vstr_base *s2,
                                   size_t p2, size_t l2)
{ return ((l1 == l2) && !vstr_cmp_vers(s1, p1, l1, s2, p2, l1)); }
extern inline int vstr_cmp_vers_buf_eq(const struct Vstr_base *s1,
                                       size_t p1, size_t l1,
                                       const char *buf, size_t buf_len)
{ return ((l1 == buf_len) && !vstr_cmp_vers_buf(s1, p1, l1, buf, buf_len)); }
extern inline int vstr_cmp_vers_cstr(const struct Vstr_base *s1,
                                     size_t p1, size_t l1, const char *buf)
{ return (vstr_cmp_vers_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_vers_cstr_eq(const struct Vstr_base *s1,
                                        size_t p1, size_t l1, const char *buf)
{ return (vstr_cmp_vers_buf_eq(s1, p1, l1, buf, strlen(buf))); }

/* add */
extern inline int vstr_add_cstr_buf(struct Vstr_base *s1, size_t pa1,
                                    const char *buf)
{ return (vstr_add_buf(s1, pa1, buf, strlen(buf))); }
extern inline int vstr_add_cstr_ptr(struct Vstr_base *s1, size_t pa1,
                                    const char *ptr)
{ return (vstr_add_ptr(s1, pa1, ptr, strlen(ptr))); }
extern inline int vstr_add_cstr_ref(struct Vstr_base *s1, size_t pa1,
                                    struct Vstr_ref *ref, size_t off)
{ return (vstr_add_ref(s1, pa1, ref, off,
                       strlen((const char *)ref->ptr) + off)); }

/* dup */
extern inline struct Vstr_base *vstr_dup_cstr_buf(struct Vstr_conf *conf,
                                                  const char *buf)
{ return (vstr_dup_buf(conf, buf, strlen(buf))); }
extern inline struct Vstr_base *vstr_dup_cstr_ptr(struct Vstr_conf *conf,
                                                  const char *ptr)
{ return (vstr_dup_ptr(conf, ptr, strlen(ptr))); }
extern inline struct Vstr_base *vstr_dup_cstr_ref(struct Vstr_conf *conf,
                                    struct Vstr_ref *ref, size_t off)
{ return (vstr_dup_ref(conf, ref, off,
                       strlen((const char *)ref->ptr) + off)); }

/* sub */
extern inline int vstr_sub_cstr_buf(struct Vstr_base *s1, size_t p1, size_t l1,
                                    const char *buf)
{ return (vstr_sub_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_sub_cstr_ptr(struct Vstr_base *s1, size_t p1, size_t l1,
                                    const char *ptr)
{ return (vstr_sub_ptr(s1, p1, l1, ptr, strlen(ptr))); }
extern inline int vstr_sub_cstr_ref(struct Vstr_base *s1, size_t p1, size_t l1,
                                    struct Vstr_ref *ref, size_t off)
{ return (vstr_sub_ref(s1, p1, l1, ref, off,
                       strlen((const char *)ref->ptr) + off)); }

/* srch */
extern inline size_t vstr_srch_cstr_buf_fwd(struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *buf)
{ return (vstr_srch_buf_fwd(s1, p1, l1, buf, strlen(buf))); }
extern inline size_t vstr_srch_cstr_buf_rev(struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *buf)
{ return (vstr_srch_buf_rev(s1, p1, l1, buf, strlen(buf))); }

extern inline size_t vstr_srch_cstr_chrs_fwd(struct Vstr_base *s1,
                                             size_t p1, size_t l1,
                                             const char *chrs)
{ return (vstr_srch_chrs_fwd(s1, p1, l1, chrs, strlen(chrs))); }
extern inline size_t vstr_srch_cstr_chrs_rev(struct Vstr_base *s1,
                                             size_t p1, size_t l1,
                                             const char *chrs)
{ return (vstr_srch_chrs_rev(s1, p1, l1, chrs, strlen(chrs))); }

extern inline size_t vstr_csrch_cstr_chrs_fwd(struct Vstr_base *s1,
                                              size_t p1, size_t l1,
                                              const char *chrs)
{ return (vstr_csrch_chrs_fwd(s1, p1, l1, chrs, strlen(chrs))); }
extern inline size_t vstr_csrch_cstr_chrs_rev(struct Vstr_base *s1,
                                              size_t p1, size_t l1,
                                              const char *chrs)
{ return (vstr_csrch_chrs_rev(s1, p1, l1, chrs, strlen(chrs))); }

extern inline size_t vstr_srch_case_cstr_buf_fwd(struct Vstr_base *s1,
                                                 size_t p1, size_t l1,
                                                 const char *buf)
{ return (vstr_srch_case_buf_fwd(s1, p1, l1, buf, strlen(buf))); }
extern inline size_t vstr_srch_case_cstr_buf_rev(struct Vstr_base *s1,
                                                 size_t p1, size_t l1,
                                                 const char *buf)
{ return (vstr_srch_case_buf_rev(s1, p1, l1, buf, strlen(buf))); }

/* spn */
extern inline size_t vstr_spn_cstr_chrs_fwd(struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *chrs)
{ return (vstr_spn_chrs_fwd(s1, p1, l1, chrs, strlen(chrs))); }
extern inline size_t vstr_spn_cstr_chrs_rev(struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *chrs)
{ return (vstr_spn_chrs_rev(s1, p1, l1, chrs, strlen(chrs))); }

extern inline size_t vstr_cspn_cstr_chrs_fwd(struct Vstr_base *s1,
                                             size_t p1, size_t l1,
                                             const char *chrs)
{ return (vstr_cspn_chrs_fwd(s1, p1, l1, chrs, strlen(chrs))); }
extern inline size_t vstr_cspn_cstr_chrs_rev(struct Vstr_base *s1,
                                             size_t p1, size_t l1,
                                             const char *chrs)
{ return (vstr_cspn_chrs_rev(s1, p1, l1, chrs, strlen(chrs))); }

/* split */
extern inline size_t vstr_split_cstr_buf(struct Vstr_base *s1,
                                         size_t p1, size_t l1,
                                         const char *buf,
                                         struct Vstr_sects *sect,
                                         unsigned int lim, unsigned int flags)
{  return (vstr_split_buf(s1, p1, l1, buf, strlen(buf), sect, lim, flags)); }
extern inline size_t vstr_split_cstr_chrs(struct Vstr_base *s1,
                                          size_t p1, size_t l1,
                                          const char *chrs,
                                          struct Vstr_sects *sect,
                                          unsigned int lim, unsigned int flags)
{  return (vstr_split_chrs(s1, p1, l1, chrs, strlen(chrs), sect, lim, flags)); }


/* simple inlines that can't easily be macros... */
/* cmp */
extern inline int vstr_cmp_bod(const struct Vstr_base *s1, size_t p1, size_t l1,
                               const struct Vstr_base *s2, size_t p2, size_t l2)
{
  size_t tmp = l1; if (tmp > l2) tmp = l2;
  return (vstr_cmp(s1, p1, tmp, s2, p2, tmp));
}
extern inline int vstr_cmp_eod(const struct Vstr_base *s1, size_t p1, size_t l1,
                               const struct Vstr_base *s2, size_t p2, size_t l2)
{
  size_t tmp = l1; if (tmp > l2) tmp = l2;
  p1 += (l1 - tmp);
  p2 += (l2 - tmp);
  return (vstr_cmp(s1, p1, tmp, s2, p2, tmp));
}
extern inline int vstr_cmp_bod_eq(const struct Vstr_base *s1,
                                  size_t p1, size_t l1,
                                  const struct Vstr_base *s2,
                                  size_t p2, size_t l2)
{ return (!vstr_cmp_bod(s1, p1, l1, s2, p2, l2)); }
extern inline int vstr_cmp_eod_eq(const struct Vstr_base *s1,
                                  size_t p1, size_t l1,
                                  const struct Vstr_base *s2,
                                  size_t p2, size_t l2)
{ return (!vstr_cmp_eod(s1, p1, l1, s2, p2, l2)); }

/* cmp buf */
extern inline int vstr_cmp_bod_buf(const struct Vstr_base *s1,
                                   size_t p1, size_t l1,
                                   const void *buf, size_t buf_len)
{
  size_t tmp = l1; if (tmp > buf_len) tmp = buf_len;
  return (vstr_cmp_buf(s1, p1, tmp, buf, tmp));
}
extern inline int vstr_cmp_eod_buf(const struct Vstr_base *s1,
                                   size_t p1, size_t l1,
                                   const void *pbuf, size_t buf_len)
{
  const char *buf = (const char *)pbuf;
  size_t tmp = l1; if (tmp > buf_len) tmp = buf_len;
  p1  += (l1 - tmp);
  buf += (buf_len - tmp);
  return (vstr_cmp_buf(s1, p1, tmp, buf, tmp));
}
extern inline int vstr_cmp_bod_buf_eq(const struct Vstr_base *s1,
                                      size_t p1, size_t l1,
                                      const void *pbuf, size_t buf_len)
{ return (!vstr_cmp_bod_buf(s1, p1, l1, pbuf, buf_len)); }
extern inline int vstr_cmp_eod_buf_eq(const struct Vstr_base *s1,
                                      size_t p1, size_t l1,
                                      const void *pbuf, size_t buf_len)
{ return (!vstr_cmp_eod_buf(s1, p1, l1, pbuf, buf_len)); }

/* cmp cstr */
extern inline int vstr_cmp_bod_cstr(const struct Vstr_base *s1,
                                    size_t p1, size_t l1,
                                    const char *buf)
{ return (vstr_cmp_bod_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_eod_cstr(const struct Vstr_base *s1,
                                    size_t p1, size_t l1,
                                    const char *buf)
{ return (vstr_cmp_eod_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_bod_cstr_eq(const struct Vstr_base *s1,
                                       size_t p1, size_t l1,
                                       const char *buf)
{ return (vstr_cmp_bod_buf_eq(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_eod_cstr_eq(const struct Vstr_base *s1,
                                       size_t p1, size_t l1,
                                       const char *buf)
{ return (vstr_cmp_eod_buf_eq(s1, p1, l1, buf, strlen(buf))); }

/* cmp case */
extern inline int vstr_cmp_case_bod(const struct Vstr_base *s1,
                                    size_t p1, size_t l1,
                                    const struct Vstr_base *s2,
                                    size_t p2, size_t l2)
{
  size_t tmp = l1; if (tmp > l2) tmp = l2;
  return (vstr_cmp_case(s1, p1, tmp, s2, p2, tmp));
}
extern inline int vstr_cmp_case_eod(const struct Vstr_base *s1,
                                    size_t p1, size_t l1,
                                    const struct Vstr_base *s2,
                                    size_t p2, size_t l2)
{
  size_t tmp = l1; if (tmp > l2) tmp = l2;
  p1 += (l1 - tmp);
  p2 += (l2 - tmp);
  return (vstr_cmp_case(s1, p1, tmp, s2, p2, tmp));
}
extern inline int vstr_cmp_case_bod_eq(const struct Vstr_base *s1,
                                  size_t p1, size_t l1,
                                  const struct Vstr_base *s2,
                                  size_t p2, size_t l2)
{ return (!vstr_cmp_case_bod(s1, p1, l1, s2, p2, l2)); }
extern inline int vstr_cmp_case_eod_eq(const struct Vstr_base *s1,
                                  size_t p1, size_t l1,
                                  const struct Vstr_base *s2,
                                  size_t p2, size_t l2)
{ return (!vstr_cmp_case_eod(s1, p1, l1, s2, p2, l2)); }

/* cmp case buf */
extern inline int vstr_cmp_case_bod_buf(const struct Vstr_base *s1,
                                        size_t p1, size_t l1,
                                        const char *buf, size_t buf_len)
{
  size_t tmp = l1; if (tmp > buf_len) tmp = buf_len;
  return (vstr_cmp_case_buf(s1, p1, tmp, buf, tmp));
}
extern inline int vstr_cmp_case_eod_buf(const struct Vstr_base *s1,
                                        size_t p1, size_t l1,
                                        const char *buf, size_t buf_len)
{
  size_t tmp = l1; if (tmp > buf_len) tmp = buf_len;
  p1  += (l1 - tmp);
  buf += (buf_len - tmp);
  return (vstr_cmp_case_buf(s1, p1, tmp, buf, tmp));
}
extern inline int vstr_cmp_case_bod_buf_eq(const struct Vstr_base *s1,
                                           size_t p1, size_t l1,
                                           const char *buf, size_t buf_len)
{ return (!vstr_cmp_case_bod_buf(s1, p1, l1, buf, buf_len)); }
extern inline int vstr_cmp_case_eod_buf_eq(const struct Vstr_base *s1,
                                           size_t p1, size_t l1,
                                           const char *buf, size_t buf_len)
{ return (!vstr_cmp_case_eod_buf(s1, p1, l1, buf, buf_len)); }

/* cmp case cstr */
extern inline int vstr_cmp_case_bod_cstr(const struct Vstr_base *s1,
                                         size_t p1, size_t l1,
                                         const char *buf)
{ return (vstr_cmp_case_bod_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_case_eod_cstr(const struct Vstr_base *s1,
                                         size_t p1, size_t l1,
                                         const char *buf)
{ return (vstr_cmp_case_eod_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_case_bod_cstr_eq(const struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *buf)
{ return (vstr_cmp_case_bod_buf_eq(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_case_eod_cstr_eq(const struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *buf)
{ return (vstr_cmp_case_eod_buf_eq(s1, p1, l1, buf, strlen(buf))); }

/* cmp vers */
extern inline int vstr_cmp_vers_bod(const struct Vstr_base *s1,
                                    size_t p1, size_t l1,
                                    const struct Vstr_base *s2,
                                    size_t p2, size_t l2)
{
  size_t tmp = l1; if (tmp > l2) tmp = l2;
  return (vstr_cmp_vers(s1, p1, tmp, s2, p2, tmp));
}
extern inline int vstr_cmp_vers_eod(const struct Vstr_base *s1,
                                    size_t p1, size_t l1,
                                    const struct Vstr_base *s2,
                                    size_t p2, size_t l2)
{
  size_t tmp = l1; if (tmp > l2) tmp = l2;
  p1 += (l1 - tmp);
  p2 += (l2 - tmp);
  return (vstr_cmp_vers(s1, p1, tmp, s2, p2, tmp));
}
extern inline int vstr_cmp_vers_bod_eq(const struct Vstr_base *s1,
                                  size_t p1, size_t l1,
                                  const struct Vstr_base *s2,
                                  size_t p2, size_t l2)
{ return (!vstr_cmp_vers_bod(s1, p1, l1, s2, p2, l2)); }
extern inline int vstr_cmp_vers_eod_eq(const struct Vstr_base *s1,
                                  size_t p1, size_t l1,
                                  const struct Vstr_base *s2,
                                  size_t p2, size_t l2)
{ return (!vstr_cmp_vers_eod(s1, p1, l1, s2, p2, l2)); }

/* cmp vers buf */
extern inline int vstr_cmp_vers_bod_buf(const struct Vstr_base *s1,
                                        size_t p1, size_t l1,
                                        const char *buf, size_t buf_len)
{
  size_t tmp = l1; if (tmp > buf_len) tmp = buf_len;
  return (vstr_cmp_vers_buf(s1, p1, tmp, buf, tmp));
}
extern inline int vstr_cmp_vers_eod_buf(const struct Vstr_base *s1,
                                        size_t p1, size_t l1,
                                        const char *buf, size_t buf_len)
{
  size_t tmp = l1; if (tmp > buf_len) tmp = buf_len;
  p1  += (l1 - tmp);
  buf += (buf_len - tmp);
  return (vstr_cmp_vers_buf(s1, p1, tmp, buf, tmp));
}
extern inline int vstr_cmp_vers_bod_buf_eq(const struct Vstr_base *s1,
                                           size_t p1, size_t l1,
                                           const char *buf, size_t buf_len)
{ return (!vstr_cmp_vers_bod_buf(s1, p1, l1, buf, buf_len)); }
extern inline int vstr_cmp_vers_eod_buf_eq(const struct Vstr_base *s1,
                                           size_t p1, size_t l1,
                                           const char *buf, size_t buf_len)
{ return (!vstr_cmp_vers_eod_buf(s1, p1, l1, buf, buf_len)); }

/* cmp vers cstr */
extern inline int vstr_cmp_vers_bod_cstr(const struct Vstr_base *s1,
                                         size_t p1, size_t l1,
                                         const char *buf)
{ return (vstr_cmp_vers_bod_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_vers_eod_cstr(const struct Vstr_base *s1,
                                         size_t p1, size_t l1,
                                         const char *buf)
{ return (vstr_cmp_vers_eod_buf(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_vers_bod_cstr_eq(const struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *buf)
{ return (vstr_cmp_vers_bod_buf_eq(s1, p1, l1, buf, strlen(buf))); }
extern inline int vstr_cmp_vers_eod_cstr_eq(const struct Vstr_base *s1,
                                            size_t p1, size_t l1,
                                            const char *buf)
{ return (vstr_cmp_vers_eod_buf_eq(s1, p1, l1, buf, strlen(buf))); }
