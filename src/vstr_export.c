#define VSTR_EXPORT_C
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
/* functions for exporting a vstr to other types */
#include "main.h"


size_t vstr_export_iovec_ptr_all(const Vstr_base *base,
                                 struct iovec **iovs, unsigned int *ret_num)
{ 
 if (!base->num || !iovs || !ret_num)
   return (0);

 if (!vstr__cache_iovec_valid((Vstr_base *)base))
   return (0);

 *iovs = base->cache->vec->v + base->cache->vec->off;

 *ret_num = base->num;
 
 return (base->len);
}

size_t vstr_export_iovec_cpy_buf(const Vstr_base *base, size_t pos, size_t len,
                                 struct iovec *iovs, unsigned int num_max,
                                 unsigned int *real_ret_num)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t ret_len = 0;
  unsigned int ret_num = 0;
  size_t used = 0;

  assert(num_max && real_ret_num);

  if (!num_max)
    return (0);
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan)
    return (0);
  
  do
  {
    size_t tmp = scan_len;
    
    if (tmp > (iovs[num].iov_len - used))
      tmp = (iovs[num].iov_len - used);
    
    if (scan->type != VSTR_TYPE_NODE_NON)
      memcpy(((char *)iovs[ret_num].iov_base) + used, scan_str, tmp);

    used += tmp;
    scan_str += tmp;
    scan_len -= tmp;
    ret_len += scan_len;

    if (iovs[ret_num].iov_len == used)
    {
      used = 0;
      
      if (++ret_num >= num_max)
        break;
    }
    
    if (!scan_len)
      scan = vstr__base_scan_fwd_nxt(base, &len, &num,
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

size_t vstr_export_iovec_cpy_ptr(const Vstr_base *base, size_t pos, size_t len,
                                 struct iovec *iovs, unsigned int num_max,
                                 unsigned int *real_ret_num)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t ret_len = 0;
  unsigned int ret_num = 0;

  assert(num_max && real_ret_num);

  if (!num_max)
    return (0);
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan)
    return (0);
  
  do
  {    
    iovs[ret_num].iov_base = scan_str;
    iovs[ret_num].iov_len = scan_len;
    ret_len += scan_len;
    
    if (++ret_num >= num_max)
      break;
    
  } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));
  
  *real_ret_num = ret_num;
  
  return (ret_len);
}

size_t vstr_export_buf(const Vstr_base *base, size_t pos, size_t len, void *buf)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 ret = len + scan_len;
 
 do
 {
  if (scan->type != VSTR_TYPE_NODE_NON)
    memcpy(buf, scan_str, scan_len);
   
  buf = ((char *)buf) + scan_len;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

char vstr_export_chr(const Vstr_base *base, size_t pos)
{
 char buf[1] = {0};

 /* errors, requests for data from NON nodes and real data are all == 0 */
 vstr_export_buf(base, pos, 1, buf);
 
 return (buf[0]);
}

