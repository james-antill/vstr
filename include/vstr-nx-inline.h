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
/* not external inline functions */
#ifdef VSTR_AUTOCONF_USE_WRAP_MEMRCHR
extern inline void *vstr_wrap_memrchr(const void *passed_s1, int c, size_t n)
{
  const unsigned char *s1 = passed_s1;
  const void *ret = 0;
  int tmp = 0;

  switch (n)
  {
    case 7:  tmp = s1[6] == c; if (tmp) { ret = s1 + 6; break; }
    case 6:  tmp = s1[5] == c; if (tmp) { ret = s1 + 5; break; }
    case 5:  tmp = s1[4] == c; if (tmp) { ret = s1 + 4; break; }
    case 4:  tmp = s1[3] == c; if (tmp) { ret = s1 + 3; break; }
    case 3:  tmp = s1[2] == c; if (tmp) { ret = s1 + 2; break; }
    case 2:  tmp = s1[1] == c; if (tmp) { ret = s1 + 1; break; }
    case 1:  tmp = s1[0] == c; if (tmp) { ret = s1 + 0; break; }
    case 0:
      break;
    default: ret = memrchr(s1, c, n);
      break;
  }

  return ((void *)ret);
}
#else
# define vstr_wrap_memrchr(x, y, z) memrchr(x, y, z)
#endif

#ifndef NDEBUG /* this is all non threadsafe ... */
static inline void vstr__debug_alloc(void)
{
  size_t sz = vstr__options.mem_sz;
  
  ++vstr__options.mem_num;

  if (!vstr__options.mem_sz)
  {
    sz = 8;
    vstr__options.mem_vals = malloc(sizeof(Vstr__debug_malloc) * sz);
  }
  else if (vstr__options.mem_num > vstr__options.mem_sz)
  {
    sz *= 2;
    vstr__options.mem_vals = realloc(vstr__options.mem_vals,
                                     sizeof(Vstr__debug_malloc) * sz);
  }
  ASSERT(vstr__options.mem_num <= sz);
  ASSERT(vstr__options.mem_vals);

  vstr__options.mem_sz = sz;
}

extern inline void *vstr__debug_malloc(size_t sz,
                                       const char *file, unsigned int line)
{
  void *ret = NULL;

  if (VSTR__DEBUG_MALLOC_DEC())
    return (NULL);

  vstr__debug_alloc();

  ASSERT(sz);

  ret = malloc(sz);
  ASSERT_RET(ret, NULL);
  
  memset(ret, 0xef, sz); /* poison */

  vstr__options.mem_vals[vstr__options.mem_num - 1].ptr  = ret;
  vstr__options.mem_vals[vstr__options.mem_num - 1].file = file;
  vstr__options.mem_vals[vstr__options.mem_num - 1].line = line;

  return (ret);
}

extern inline int vstr__debug_malloc_check_mem(const void *ptr)
{
  unsigned int scan = 0;

  ASSERT(vstr__options.mem_num);
    
  while (vstr__options.mem_vals[scan].ptr &&
         (vstr__options.mem_vals[scan].ptr != ptr))
    ++scan;
  
  ASSERT(vstr__options.mem_vals[scan].ptr);

  return (scan);
}

extern inline void vstr__debug_free(void *ptr)
{
  if (ptr)
  {
    unsigned int scan = vstr__debug_malloc_check_mem(ptr);

    --vstr__options.mem_num;

    if (scan != vstr__options.mem_num)
    {
      SWAP_TYPE(vstr__options.mem_vals[scan].ptr,
                vstr__options.mem_vals[vstr__options.mem_num].ptr,
                void *);
      SWAP_TYPE(vstr__options.mem_vals[scan].file,
                vstr__options.mem_vals[vstr__options.mem_num].file,
                const char *);
      SWAP_TYPE(vstr__options.mem_vals[scan].line,
                vstr__options.mem_vals[vstr__options.mem_num].line,
                unsigned int);
    }
    vstr__options.mem_vals[vstr__options.mem_num].ptr = NULL;
    free(ptr);
  }
}

extern inline void *vstr__debug_realloc(void *ptr, size_t sz,
                                        const char *file, unsigned int line)
{
  void *ret = NULL;

  ASSERT(ptr && sz);

  if (VSTR__DEBUG_MALLOC_DEC())
    return (NULL);

  ret = realloc(ptr, sz);
  ASSERT_RET(ret, NULL);

  if (ptr != ret) /* not ISO C compliant */
  {
    unsigned int scan = vstr__debug_malloc_check_mem(ptr);

    vstr__options.mem_vals[scan].ptr  = ret;
    vstr__options.mem_vals[scan].file = file;
    vstr__options.mem_vals[scan].line = line;
  }

  return (ret);
}
extern inline void vstr__debug_malloc_check_empty(void)
{
  if (0 && vstr__options.mem_num) /* needs to be zero'd for coverage testing */
  {
    unsigned int scan = 0;

    while (vstr__options.mem_vals[scan].ptr)
    {
      fprintf(stderr, " FAILED MEM CHECK EMPTY: ptr %p from %u:%s\n",
              vstr__options.mem_vals[scan].ptr,
              vstr__options.mem_vals[scan].line,
              vstr__options.mem_vals[scan].file);
      ++scan;
    }
  }
  ASSERT(!vstr__options.mem_num);
}
#endif

/* zero ->used and normalise first node */
extern inline void vstr__base_zero_used(Vstr_base *base)
{
  if (base->used)
  {
    ASSERT(base->beg->type == VSTR_TYPE_NODE_BUF);
    base->beg->len -= base->used;
    vstr_wrap_memmove(((Vstr_node_buf *)base->beg)->buf,
                      ((Vstr_node_buf *)base->beg)->buf + base->used,
                      base->beg->len);
    base->used = 0;
  }
}

extern inline void vstr__cache_iovec_reset_node(const Vstr_base *base,
                                                Vstr_node *node,
                                                unsigned int num)
{
  struct iovec *iovs = NULL;
  unsigned char *types = NULL;

  if (!base->iovec_upto_date)
    return;

  iovs = VSTR__CACHE(base)->vec->v + VSTR__CACHE(base)->vec->off;
  iovs[num - 1].iov_len = node->len;
  iovs[num - 1].iov_base = vstr_export__node_ptr(node);

  types = VSTR__CACHE(base)->vec->t + VSTR__CACHE(base)->vec->off;
  types[num - 1] = node->type;

  if (num == 1)
  {
    char *tmp = NULL;

    iovs[num - 1].iov_len -= base->used;
    tmp = iovs[num - 1].iov_base;
    tmp += base->used;
    iovs[num - 1].iov_base = tmp;
  }
}

extern inline void vstr__relink_nodes(Vstr_conf *conf,
                                      Vstr_node *beg, Vstr_node **end_next,
                                      unsigned int num)
{
  Vstr_node *tmp = NULL;

  ASSERT(beg->type == VSTR__CONV_PTR_NEXT_PREV(end_next)->type);

  switch (beg->type)
  {
    case VSTR_TYPE_NODE_BUF:
      tmp = (Vstr_node *)conf->spare_buf_beg;

      conf->spare_buf_num += num;

      conf->spare_buf_beg = (Vstr_node_buf *)beg;
      break;

    case VSTR_TYPE_NODE_NON:
      tmp = (Vstr_node *)conf->spare_non_beg;

      conf->spare_non_num += num;

      conf->spare_non_beg = (Vstr_node_non *)beg;
      break;

    case VSTR_TYPE_NODE_PTR:
      tmp = (Vstr_node *)conf->spare_ptr_beg;

      conf->spare_ptr_num += num;

      conf->spare_ptr_beg = (Vstr_node_ptr *)beg;
      break;

    case VSTR_TYPE_NODE_REF:
      tmp = (Vstr_node *)conf->spare_ref_beg;

      conf->spare_ref_num += num;

      conf->spare_ref_beg = (Vstr_node_ref *)beg;

      ASSERT_NO_SWITCH_DEF();
  }

  *end_next = tmp;
}

/* sequence could get annoying ... too many special cases */
extern inline int vstr__base_scan_rev_beg(const Vstr_base *base,
                                          size_t pos, size_t *len,
                                          unsigned int *num, unsigned int *type,
                                          char **scan_str, size_t *scan_len)
{
  Vstr_node *scan_beg = NULL;
  Vstr_node *scan_end = NULL;
  unsigned int dummy_num = 0;
  size_t end_pos = 0;

  ASSERT(base && num && len && type && scan_str && scan_len);

  if (!*len)
    return (FALSE);
  
  ASSERT_RET(((pos + *len - 1) <= base->len), FALSE);

  assert(base->iovec_upto_date);

  end_pos = pos;
  end_pos += *len - 1;
  scan_beg = vstr_base__pos(base, &pos, &dummy_num, TRUE);
  --pos;

  scan_end = vstr_base__pos(base, &end_pos, num, FALSE);

  *type = scan_end->type;

  if (scan_beg != scan_end)
  {
    assert(*num != dummy_num);
    assert(scan_end != base->beg);

    pos = 0;
    *scan_len = end_pos;

    assert(*scan_len < *len);
    *len -= *scan_len;
  }
  else
  {
    assert(scan_end->len >= *len);
    *scan_len = *len;
    *len = 0;
  }

  *scan_str = NULL;
  if (scan_end->type != VSTR_TYPE_NODE_NON)
    *scan_str = vstr_export__node_ptr(scan_end) + pos;

  return (TRUE);
}

extern inline int vstr__base_scan_rev_nxt(const Vstr_base *base, size_t *len,
                                          unsigned int *num, unsigned int *type,
                                          char **scan_str, size_t *scan_len)
{
  struct iovec *iovs = NULL;
  unsigned char *types = NULL;
  size_t pos = 0;

  ASSERT(base && num && len && type && scan_str && scan_len);

  assert(base->iovec_upto_date);

  assert(num);

  if (!*len || !--*num)
    return (FALSE);

  iovs = VSTR__CACHE(base)->vec->v + VSTR__CACHE(base)->vec->off;
  types = VSTR__CACHE(base)->vec->t + VSTR__CACHE(base)->vec->off;

  *type = types[*num - 1];
  *scan_len = iovs[*num - 1].iov_len;

  if (*scan_len > *len)
  {
    pos = *scan_len - *len;
    *scan_len = *len;
  }
  *len -= *scan_len;

  *scan_str = NULL;
  if (*type != VSTR_TYPE_NODE_NON)
    *scan_str = ((char *) (iovs[*num - 1].iov_base)) + pos;

  return (TRUE);
}

extern inline const char *
vstr__loc_num_grouping(Vstr_locale *loc, unsigned int num_base)
{
  Vstr_locale_num_base *srch = vstr__loc_num_srch(loc, num_base, FALSE);

  return (srch->grouping->ptr);
}

extern inline const char *
vstr__loc_num_sep_ptr(Vstr_locale *loc, unsigned int num_base)
{
  Vstr_locale_num_base *srch = vstr__loc_num_srch(loc, num_base, FALSE);

  return (srch->thousands_sep_ref->ptr);
}

extern inline size_t
vstr__loc_num_sep_len(Vstr_locale *loc, unsigned int num_base)
{
  Vstr_locale_num_base *srch = vstr__loc_num_srch(loc, num_base, FALSE);

  return (srch->thousands_sep_len);
}

extern inline const char *
vstr__loc_num_pnt_ptr(Vstr_locale *loc, unsigned int num_base)
{
  Vstr_locale_num_base *srch = vstr__loc_num_srch(loc, num_base, FALSE);

  return (srch->decimal_point_ref->ptr);
}

extern inline size_t
vstr__loc_num_pnt_len(Vstr_locale *loc, unsigned int num_base)
{
  Vstr_locale_num_base *srch = vstr__loc_num_srch(loc, num_base, FALSE);

  return (srch->decimal_point_len);
}

