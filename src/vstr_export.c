#define VSTR_EXPORT_C
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
/* functions for exporting data out of the Vstr -- see also vstr_cstr.c */
#include "main.h"


size_t vstr_nx_export_iovec_ptr_all(const Vstr_base *base,
                                    struct iovec **iovs, unsigned int *ret_num)
{
 if (!base->num)
   return (0);

 if (!vstr__cache_iovec_valid((Vstr_base *)base))
   return (0);

 if (iovs)
   *iovs = VSTR__CACHE(base)->vec->v + VSTR__CACHE(base)->vec->off;

 if (ret_num)
   *ret_num = base->num;
 
 return (base->len);
}

size_t vstr_nx_export_iovec_cpy_buf(const Vstr_base *base,
                                    size_t pos, size_t len,
                                    struct iovec *iovs, unsigned int num_max,
                                    unsigned int *real_ret_num)
{
  Vstr_node *scan = NULL;
  unsigned int scan_num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t ret_len = 0;
  unsigned int dummy_ret_num = 0;
  unsigned int ret_num = 0;
  size_t used = 0;

  assert(iovs && num_max);

  if (!num_max)
    return (0);

  if (!real_ret_num)
    real_ret_num = &dummy_ret_num;
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &scan_num,
                                 &scan_str, &scan_len);
  if (!scan)
    return (0);
  
  do
  {
    size_t tmp = scan_len;

    assert(tmp);
    assert(iovs[ret_num].iov_len);
    assert(iovs[ret_num].iov_len > used);
    
    if (tmp > (iovs[ret_num].iov_len - used))
      tmp = (iovs[ret_num].iov_len - used);
    
    if (scan->type != VSTR_TYPE_NODE_NON)
      vstr_nx_wrap_memcpy(((char *)iovs[ret_num].iov_base) + used, scan_str,
                          tmp);

    used += tmp;
    scan_str += tmp;
    scan_len -= tmp;
    ret_len += tmp;

    if (iovs[ret_num].iov_len == used)
    {
      used = 0;
      
      if (++ret_num >= num_max)
        break;
    }
    
    if (!scan_len)
      scan = vstr__base_scan_fwd_nxt(base, &len, &scan_num,
                                     scan, &scan_str, &scan_len);
  } while (scan);

  if (used)
  {
    iovs[ret_num].iov_len = used;
    ++ret_num;
  }

  *real_ret_num = ret_num;
  
  return (ret_len);
}

size_t vstr_nx_export_iovec_cpy_ptr(const Vstr_base *base,
                                    size_t pos, size_t len,
                                    struct iovec *iovs, unsigned int num_max,
                                    unsigned int *real_ret_num)
{
  size_t orig_len = len;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t ret_len = 0;
  unsigned int dummy_ret_num = 0;
  unsigned int ret_num = 0;

  assert(iovs && num_max && real_ret_num);

  if (!num_max)
    return (0);
  
  if (!real_ret_num)
    real_ret_num = &dummy_ret_num;
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan)
    return (0);
  
  do
  {    
    iovs[ret_num].iov_base = scan_str;
    iovs[ret_num].iov_len = scan_len;
    ret_len += scan_len;
    
  } while ((++ret_num < num_max) &&
           (scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));
  assert((ret_len == orig_len) || (ret_num == num_max));
  
  *real_ret_num = ret_num;

  return (ret_len);
}

size_t vstr_nx_export_buf(const Vstr_base *base, size_t pos, size_t len,
                          void *buf, size_t buf_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 if (!buf_len)
   return (0);
 
 if (len > buf_len)
   len = buf_len;

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 ret = len + scan_len;
 
 do
 {
  if (scan->type != VSTR_TYPE_NODE_NON)
    vstr_nx_wrap_memcpy(buf, scan_str, scan_len);
   
  buf = ((char *)buf) + scan_len;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

char vstr_nx_export_chr(const Vstr_base *base, size_t pos)
{
 char buf[1] = {0};

 /* errors, requests for data from NON nodes and real data are all == 0 */
 vstr_nx_export_buf(base, pos, 1, buf, 1);
 
 return (buf[0]);
}

static Vstr_ref *vstr__export_buf_ref(const Vstr_base *base,
                                      size_t pos, size_t len)
{
  Vstr_ref *ref = NULL;

  if (!len)
    return (NULL);
  
  if (!(ref = vstr_nx_ref_make_malloc(len)))
  {
    base->conf->malloc_bad = TRUE;
    return (NULL);
  }

  assert(((Vstr__buf_ref *)ref)->buf == ref->ptr);
  
  vstr_nx_export_buf(base, pos, len, ref->ptr, len);
  
  return (ref);
}

Vstr_ref *vstr_nx_export_ref(const Vstr_base *base, size_t pos, size_t len,
                             size_t *ret_off)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  Vstr_ref *ref = NULL;
  size_t orig_pos = pos;
  
  assert(base && pos && len && ((pos + len - 1) <= base->len));
  assert(ret_off);
  
  scan = vstr__base_pos(base, &pos, &num, TRUE);
  if (!scan)
    return (0);
  --pos;
  
  if (0)
  { /* do nothing */; }
  else if (scan->type == VSTR_TYPE_NODE_REF)
  {
    if ((scan->len - pos) >= len)
    {
      *ret_off = pos + ((Vstr_node_ref *)scan)->off;
      return (vstr_nx_ref_add(((Vstr_node_ref *)scan)->ref));
    }
  }
  else if (scan->type == VSTR_TYPE_NODE_PTR)
  {
    if ((scan->len - pos) >= len)
    {
      void *ptr = ((char *)((Vstr_node_ptr *)scan)->ptr) + pos;
      
      if (!(ref = vstr_nx_ref_make_ptr(ptr, vstr_nx_ref_cb_free_ref)))
      {
        base->conf->malloc_bad = TRUE;
        return (NULL);
      }
      
      *ret_off = 0;
      
      return (ref);
    }
  }

  if (base->cache_available)
  { /* use cstr cache if available */
    Vstr__cache_data_cstr *data = NULL;
    unsigned int off = base->conf->cache_pos_cb_cstr;
    
    if ((data = vstr_nx_cache_get(base, off)) && data->ref)
    {
      if (pos >= data->pos)
      {
        size_t tmp = (pos - data->pos);
        if (data->len <= (len - tmp))
        {
          *ret_off = tmp;
          return (vstr_nx_ref_add(data->ref));
        }
      }
    }
  }

  *ret_off = 0;
  if (!(ref = vstr__export_buf_ref(base, orig_pos, len)))
    return (NULL);

  return (ref);
}
