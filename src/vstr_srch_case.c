#define VSTR_SRCH_CASE_C
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
/* functions for searching within a vstr, in a case independant manner */
#include "main.h"

size_t vstr_nx_srch_case_chr_fwd(const Vstr_base *base, size_t pos, size_t len,
                                 char srch)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  size_t ret = pos;
  char *scan_str = NULL;
  size_t scan_len = 0;
  
  if (!VSTR__IS_ASCII_ALPHA(srch)) /* not searching for a case dependant char */
    return (vstr_nx_srch_chr_fwd(base, pos, len, srch));

  if (VSTR__IS_ASCII_LOWER(srch))
    srch = VSTR__TO_ASCII_UPPER(srch);
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan)
    return (0);
  
  ret = len + scan_len;
  
  do
  {
    if (scan->type != VSTR_TYPE_NODE_NON)
    {
      size_t count = 0;
      
      while (count < scan_len)
      {
        char scan_tmp = scan_str[count];
        if (VSTR__IS_ASCII_LOWER(scan_tmp))
          scan_tmp = VSTR__TO_ASCII_UPPER(scan_tmp);

        if (scan_tmp == srch)
          return (pos + ((ret - len) - scan_len) + count);

        ++count;
      }
    }
  } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));
  
  return (0);
}

static size_t vstr__srch_case_chr_rev_slow(const Vstr_base *base,
                                           size_t pos, size_t len,
                                           char srch)
{
  size_t ret = 0;
  size_t scan_pos = pos;
  size_t scan_len = len;
  
  while ((scan_pos < (pos + len - 1)) &&
         scan_len)
  {
    size_t tmp = vstr_nx_srch_case_chr_fwd(base, scan_pos, scan_len, srch);
    if (!tmp)
      break;

    ret = tmp;
    
    scan_pos = ret + 1;
    scan_len = len - ((ret - pos) + 1);
  }

  return (ret);
}

size_t vstr_nx_srch_case_chr_rev(const Vstr_base *base, size_t pos, size_t len,
                                 char srch)
{
  return (vstr__srch_case_chr_rev_slow(base, pos, len, srch));
}

size_t vstr_nx_srch_case_buf_fwd(const Vstr_base *base, size_t pos, size_t len,
                                 const void *const str, const size_t str_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t orig_len = len;
 char *scan_str = NULL;
 size_t scan_len = 0;
 char tmp = 0;
 
 if (!len || (str_len > len))
   return (0);

 if (!str_len)
   return (pos);

 if (!str) /* search for _NON lengths are no different */
   return (vstr_nx_srch_buf_fwd(base, pos, len, str, str_len));
 
 if (str_len == 1)
   return (vstr_nx_srch_case_chr_fwd(base, pos, len, *(const char *)str));
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 assert(orig_len == (len + scan_len));

 tmp = *(const char *)str;
 if (VSTR__IS_ASCII_LOWER(tmp))
   tmp = VSTR__TO_ASCII_UPPER(tmp);

 do
 {
   assert(str);
   if (scan->type == VSTR_TYPE_NODE_NON)
     goto next_loop;
   
   /* find buf */
   while (scan_len && ((len + scan_len) >= str_len))
   {
     size_t beg_pos = 0;
     char scan_tmp = 0;

     assert(scan_len);

     scan_tmp = *scan_str;
     if (VSTR__IS_ASCII_LOWER(scan_tmp))
       scan_tmp = VSTR__TO_ASCII_UPPER(scan_tmp);
     if (scan_tmp != tmp)
     {
       --scan_len;
       ++scan_str;
       continue;
     }
     
     beg_pos = pos + ((orig_len - len) - scan_len);
     if (!vstr_nx_cmp_case_buf(base, beg_pos, str_len,
                               (const char *)str, str_len))
       return (beg_pos);
     
     ++scan_str;
     --scan_len;
   }
  
  next_loop:
   continue;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)) &&
          ((len + scan_len) >= str_len));
 
 return (0);
}

static size_t vstr__srch_case_buf_rev_slow(const Vstr_base *base,
                                           size_t pos, size_t len,
                                           const void *const str,
                                           const size_t str_len)
{
  size_t ret = 0;
  size_t scan_pos = pos;
  size_t scan_len = len;
  
  while ((scan_pos < (pos + len - 1)) &&
         (scan_len >= str_len))
  {
    size_t tmp = vstr_nx_srch_case_buf_fwd(base, scan_pos, scan_len,
                                           str, str_len);
    if (!tmp)
      break;

    ret = tmp;
    
    scan_pos = ret + 1;
    scan_len = len - ((ret - pos) + 1);
  }

  return (ret);
}

size_t vstr_nx_srch_case_buf_rev(const Vstr_base *base, size_t pos, size_t len,
                                 const void *const str, const size_t str_len)
{
  if (!len || (str_len > len))
    return (0);
  
  if (!str_len)
    return (pos + len - 1);
  
  if (str_len == 1)
    return (vstr_nx_srch_case_chr_rev(base, pos, len, *(const char *)str));
  
  return (vstr__srch_case_buf_rev_slow(base, pos, len, str, str_len));
}

size_t vstr_nx_srch_case_vstr_fwd(const Vstr_base *base, size_t pos, size_t len,
                                  const Vstr_base *ndl_base,
                                  size_t ndl_pos, size_t ndl_len)
{
  size_t scan_pos = pos;
  size_t scan_len = len;
  Vstr_node *beg_ndl_node = NULL;
  unsigned int dummy_num = 0;
  size_t dummy_len = ndl_len;
  char *beg_ndl_str = NULL;
  size_t beg_ndl_len = 0;

  if (ndl_len > len)
    return (0);

  beg_ndl_node = vstr__base_scan_fwd_beg(ndl_base, ndl_pos,
                                         &dummy_len, &dummy_num,
                                         &beg_ndl_str, &beg_ndl_len);
  if (!beg_ndl_node)
    return (0);

  while ((scan_pos < (pos + len - 1)) &&
         (scan_len >= ndl_len))
  {
    if (!vstr_nx_cmp_case(base, scan_pos, ndl_len, ndl_base, ndl_pos, ndl_len))
      return (scan_pos);

    --scan_len;
    ++scan_pos;
    
    if (beg_ndl_node->type != VSTR_TYPE_NODE_NON)
    {
      size_t tmp = 0; 

      if (!(tmp = vstr_nx_srch_case_buf_fwd(base, scan_pos, scan_len,
                                            beg_ndl_str, beg_ndl_len)))
        return (0);
      
      assert(tmp > scan_pos);
      scan_len -= tmp - scan_pos;
      scan_pos = tmp;
    }
  }
  
  return (0);
}

static size_t vstr__srch_case_vstr_rev_slow(const Vstr_base *base,
                                            size_t pos, size_t len,
                                            const Vstr_base *ndl_base,
                                            size_t ndl_pos, size_t ndl_len)
{
  size_t ret = 0;
  size_t scan_pos = pos;
  size_t scan_len = len;
  
  while ((scan_pos < (pos + len - 1)) &&
         (scan_len >= ndl_len))
  {
    size_t tmp = vstr_nx_srch_case_vstr_fwd(base, scan_pos, scan_len,
                                            ndl_base, ndl_pos, ndl_len);
    if (!tmp)
      break;

    ret = tmp;
    
    scan_pos = ret + 1;
    scan_len = len - ((ret - pos) + 1);
  }

  return (ret);
}

size_t vstr_nx_srch_case_vstr_rev(const Vstr_base *base, size_t pos, size_t len,
                                  const Vstr_base *ndl_base,
                                  size_t ndl_pos, size_t ndl_len)
{
  return (vstr__srch_case_vstr_rev_slow(base, pos, len,
                                        ndl_base, ndl_pos, ndl_len));
}
