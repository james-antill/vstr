#define VSTR_SPN_C
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
/* functionss for counting a "spanning" of chars within a vstr */
#include "main.h"

size_t vstr_spn_buf_fwd(const Vstr_base *base, size_t pos, size_t len,
                        const char *spn_buf, size_t spn_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
  size_t count = 0;
  
  if ((scan->type == VSTR_TYPE_NODE_NON) && spn_buf)
    return (ret);

  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   assert(!spn_buf);
   goto next_loop;
  }
  
  if (!spn_buf)
    return (ret);
  
  while (count < scan_len)
  {
   if (!memchr(spn_buf, scan_str[count], spn_len))
     return (ret + count);
   ++count; 
  }
  assert(count == scan_len);

 next_loop:
  ret += scan_len;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

static size_t vstr__spn_buf_rev_slow(const Vstr_base *base,
                                     size_t pos, size_t len,
                                     const char *spn_buf, size_t spn_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
  size_t count = 0;
  
  if ((scan->type == VSTR_TYPE_NODE_NON) && spn_buf)
  {
   ret = 0;
   continue;
  }
    
  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   assert(!spn_buf);
   goto next_loop_all_good;
  }
  
  if (!spn_buf)
  {
   ret = 0;
   continue;
  }

  count = scan_len;
  while (count > 0)
  {
   --count;
   if (!memchr(spn_buf, scan_str[count], spn_len))
   {
    ret = ((scan_len - count) - 1);
    if (!len)
      return (ret);
    goto next_loop_memchr_fail;
   }
  }
  
 next_loop_all_good:
  ret += scan_len;
  
 next_loop_memchr_fail:
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

size_t vstr_spn_buf_rev(const Vstr_base *base, size_t pos, size_t len,
                        const char *spn_buf, size_t spn_len)
{ /* FIXME: this needs to use iovec to walk backwards */
 if (!vstr__base_iovec_valid((Vstr_base *)base))
   return (vstr__spn_buf_rev_slow(base, pos, len, spn_buf, spn_len));

 return (vstr__spn_buf_rev_slow(base, pos, len, spn_buf, spn_len));
}

size_t vstr_cspn_buf_fwd(const Vstr_base *base, size_t pos, size_t len,
                         const char *cspn_buf, size_t cspn_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
  if ((scan->type == VSTR_TYPE_NODE_NON) && cspn_buf)
    goto next_loop;

  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   assert(!cspn_buf);
   return (ret);
  }
  
  if (!cspn_buf)
    goto next_loop;
  
  if (scan->type != VSTR_TYPE_NODE_NON)
  {
   size_t count = 0;
   while (count < scan_len)
   {
    if (memchr(cspn_buf, scan_str[count], cspn_len))
      return (ret + count);
    ++count;
   }
  }

 next_loop:
  ret += scan_len;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

static size_t vstr__cspn_buf_rev_slow(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      const char *cspn_buf, size_t cspn_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
  size_t count = 0;
  
  if ((scan->type == VSTR_TYPE_NODE_NON) && cspn_buf)
    goto next_loop_all_good;
    
  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   assert(!cspn_buf);
   ret = 0;
   continue;
  }
  
  if (!cspn_buf)
    goto next_loop_all_good;

  count = scan_len;
  while (count > 0)
  {
   --count;
   if (memchr(cspn_buf, scan_str[count], cspn_len))
   {
    ret = ((scan_len - count) - 1);
    if (!len)
      return (ret);
    goto next_loop_memchr_fail;
   }
  }
  
 next_loop_all_good:
  ret += scan_len;
  
 next_loop_memchr_fail:
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

size_t vstr_cspn_buf_rev(const Vstr_base *base, size_t pos, size_t len,
                         const char *cspn_buf, size_t cspn_len)
{ /* FIXME: this needs to use iovec to walk backwards */
 if (!vstr__base_iovec_valid((Vstr_base *)base))
   return (vstr__cspn_buf_rev_slow(base, pos, len, cspn_buf, cspn_len));

 return (vstr__cspn_buf_rev_slow(base, pos, len, cspn_buf, cspn_len));
}
