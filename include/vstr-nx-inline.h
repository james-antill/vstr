/*
 *  Copyright (C) 2002  James Antill
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
extern inline void *vstr_nx_wrap_memrchr(const void *passed_s1, int c, size_t n)
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
      break;
    default: ret = memrchr(s1, c, n);
      break;
  }
  
  return ((void *)ret);
}
#else
# define vstr_nx_wrap_memrchr(x, y, z) memrchr(x, y, z)
#endif

#ifndef NDEBUG
extern inline void *vstr__debug_malloc(size_t sz)
{
  ++vstr__options.malloc_count;
  
  ASSERT(sz);
  return (malloc(sz));
}

extern inline void *vstr__debug_calloc(size_t num, size_t sz)
{
  ++vstr__options.malloc_count;
  
  ASSERT(sz);
  return (calloc(num, sz));
}

extern inline void vstr__debug_free(void *ptr)
{
  ASSERT(vstr__options.malloc_count--);
  
  free(ptr);
}
#else
# define vstr__debug_malloc malloc
# define vstr__debug_calloc calloc
# define vstr__debug_free free
#endif

extern inline int vstr__safe_realloc(void **ptr, size_t sz)
{
  void *tmp = realloc(*ptr, sz);

  if (!tmp)
    return (FALSE);

  ASSERT(*ptr && sz);
  
  *ptr = tmp;
  
  return (TRUE);
}

/* zero ->used and normalise first node */
extern inline void vstr__base_zero_used(Vstr_base *base)
{
  if (base->used)
  {
    ASSERT(base->beg->type == VSTR_TYPE_NODE_BUF);
    base->beg->len -= base->used;
    vstr_nx_wrap_memmove(((Vstr_node_buf *)base->beg)->buf,
                         ((Vstr_node_buf *)base->beg)->buf + base->used,
                         base->beg->len);
    base->used = 0;
  }
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
  
  assert(base && num && len && type);
  
  assert(*len && ((pos + *len - 1) <= base->len));
  
  assert(base->iovec_upto_date);
  
  if ((pos > base->len) || !*len)
    return (FALSE);
  
  if ((pos + *len - 1) > base->len)
    *len = base->len - (pos - 1);

  end_pos = pos;
  end_pos += *len - 1;
  scan_beg = vstr_nx_base__pos(base, &pos, &dummy_num, TRUE);
  --pos;
  
  scan_end = vstr_nx_base__pos(base, &end_pos, num, FALSE);

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
    *scan_str = vstr_nx_export__node_ptr(scan_end) + pos;
  
  return (TRUE);
}

extern inline int vstr__base_scan_rev_nxt(const Vstr_base *base, size_t *len,
                                          unsigned int *num, unsigned int *type,
                                          char **scan_str, size_t *scan_len)
{
  struct iovec *iovs = NULL;
  unsigned char *types = NULL;
  size_t pos = 0;
  
  assert(base && num && len && type);
  
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
