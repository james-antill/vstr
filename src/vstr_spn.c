#define VSTR_SPN_C
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
/* functionss for counting a "spanning" of chars within a vstr */
#include "main.h"

size_t vstr_nx_spn_chrs_fwd(const Vstr_base *base, size_t pos, size_t len,
                            const char *spn_chrs, size_t spn_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 if (!spn_chrs && !base->node_non_used)
   return (0);
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
  size_t count = 0;
  
  if ((scan->type == VSTR_TYPE_NODE_NON) && spn_chrs)
    return (ret);

  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   assert(!spn_chrs);
   goto next_loop;
  }
  
  if (!spn_chrs)
    return (ret);
  
  while (count < scan_len)
  {
   if (!vstr_nx_wrap_memchr(spn_chrs, scan_str[count], spn_len))
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

/* go through fwd, reset everytime it fails then start doing it again */
#if 0
static size_t vstr__spn_chrs_rev_slow(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      const char *spn_chrs, size_t spn_len)
    VSTR__ATTR_I();
#endif
static size_t vstr__spn_chrs_rev_slow(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      const char *spn_chrs, size_t spn_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 if (!spn_chrs && !base->node_non_used)
   return (0);
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
  size_t count = 0;
  
  if ((scan->type == VSTR_TYPE_NODE_NON) && spn_chrs)
  {
   ret = 0;
   continue;
  }
    
  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   assert(!spn_chrs);
   goto next_loop_all_good;
  }
  
  if (!spn_chrs)
  {
   ret = 0;
   continue;
  }

  count = scan_len;
  while (count > 0)
  {
   --count;
   if (!vstr_nx_wrap_memchr(spn_chrs, scan_str[count], spn_len))
   {
    ret = ((scan_len - count) - 1);
    goto next_loop_memchr_fail;
   }
  }
  
 next_loop_all_good:
  ret += scan_len;
  
 next_loop_memchr_fail:
  continue;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

static size_t vstr__spn_chrs_rev_fast(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      const char *spn_chrs, size_t spn_len)
{
  unsigned int num = 0;
  unsigned int type = 0;
  size_t ret = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  
  if (!spn_chrs && !base->node_non_used)
    return (0);
 
  if (!vstr__base_scan_rev_beg(base, pos, &len, &num, &type,
                               &scan_str, &scan_len))
    return (0);
  
  do
  {
    size_t count = 0;
    
    if ((type == VSTR_TYPE_NODE_NON) && spn_chrs)
      return (ret);
    
    if (type == VSTR_TYPE_NODE_NON)
    {
      assert(!spn_chrs);
      goto next_loop;
    }
    
    if (!spn_chrs)
      return (ret);
    
    while (count < scan_len)
    {
      ++count;
      if (!vstr_nx_wrap_memchr(spn_chrs, scan_str[scan_len - count], spn_len))
        return (ret + (count - 1));
    }
    assert(count == scan_len);
    
   next_loop:
    ret += scan_len;
  } while (vstr__base_scan_rev_nxt(base, &len, &num, &type,
                                   &scan_str, &scan_len));
  
  return (ret);
}

size_t vstr_nx_spn_chrs_rev(const Vstr_base *base, size_t pos, size_t len,
                            const char *spn_chrs, size_t spn_len)
{
  if (base->iovec_upto_date)
    return (vstr__spn_chrs_rev_fast(base, pos, len, spn_chrs, spn_len));
  
  return (vstr__spn_chrs_rev_slow(base, pos, len, spn_chrs, spn_len));
}

size_t vstr_nx_cspn_chrs_fwd(const Vstr_base *base, size_t pos, size_t len,
                             const char *cspn_chrs, size_t cspn_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 if (!cspn_chrs && !base->node_non_used)
   return (len);
 
 if (cspn_len == 1)
 {
   size_t f_pos = vstr_nx_srch_chr_fwd(base, pos, len, cspn_chrs[0]);
   
   if (!f_pos)
     return (len);
   
   return (f_pos - pos);
 }

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
   size_t count = 0;
   
   if ((scan->type == VSTR_TYPE_NODE_NON) && cspn_chrs)
     goto next_loop;
   
   if (scan->type == VSTR_TYPE_NODE_NON)
   {
     assert(!cspn_chrs);
     return (ret);
   }
   
   if (!cspn_chrs)
     goto next_loop;
   
   while (count < scan_len)
   {
     if (vstr_nx_wrap_memchr(cspn_chrs, scan_str[count], cspn_len))
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

/* go through fwd, reset everytime it fails then start doing it again */
static size_t vstr__cspn_chrs_rev_slow(const Vstr_base *base,
                                       size_t pos, size_t len,
                                       const char *cspn_chrs, size_t cspn_len)
{  
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;
 
 if (!cspn_chrs && !base->node_non_used)
   return (len);
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 do
 {
  size_t count = 0;
  
  if ((scan->type == VSTR_TYPE_NODE_NON) && cspn_chrs)
    goto next_loop_all_good;
    
  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   assert(!cspn_chrs);
   ret = 0;
   continue;
  }
  
  if (!cspn_chrs)
    goto next_loop_all_good;

  count = scan_len;
  while (count > 0)
  {
   --count;
   if (vstr_nx_wrap_memchr(cspn_chrs, scan_str[count], cspn_len))
   {
    ret = ((scan_len - count) - 1);
    goto next_loop_memchr_fail;
   }
  }
  
 next_loop_all_good:
  ret += scan_len;
  
 next_loop_memchr_fail:
  continue;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

static size_t vstr__cspn_chrs_rev_fast(const Vstr_base *base,
                                       size_t pos, size_t len,
                                       const char *cspn_chrs, size_t cspn_len)
{
  unsigned int num = 0;
  unsigned int type = 0;
  size_t ret = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  
  if (!vstr__base_scan_rev_beg(base, pos, &len, &num, &type,
                               &scan_str, &scan_len))
    return (0);
  
  do
  {
    size_t count = 0;
    
    if ((type == VSTR_TYPE_NODE_NON) && cspn_chrs)
      return (ret);
    
    if (type == VSTR_TYPE_NODE_NON)
    {
      assert(!cspn_chrs);
      goto next_loop;
    }
    
    if (!cspn_chrs)
      return (ret);
    
    while (count < scan_len)
    {
      ++count;
      if (vstr_nx_wrap_memchr(cspn_chrs, scan_str[scan_len - count], cspn_len))
        return (ret + (count - 1));
    }
    assert(count == scan_len);
  
   next_loop:
    ret += scan_len;
  } while (vstr__base_scan_rev_nxt(base, &len, &num, &type,
                                   &scan_str, &scan_len));
  
  return (ret);
}

size_t vstr_nx_cspn_chrs_rev(const Vstr_base *base, size_t pos, size_t len,
                            const char *cspn_chrs, size_t cspn_len)
{  
  if (!cspn_chrs && !base->node_non_used)
    return (len);
  
  if (cspn_len == 1)
  {
    size_t f_pos = vstr_nx_srch_chr_rev(base, pos, len, cspn_chrs[0]);

    if (!f_pos)
      return (len);
    
    return ((pos + (len - 1)) - f_pos);
  }
 
  if (base->iovec_upto_date)
    return (vstr__cspn_chrs_rev_fast(base, pos, len, cspn_chrs, cspn_len));
  
  return (vstr__cspn_chrs_rev_slow(base, pos, len, cspn_chrs, cspn_len));
}
