#define VSTR_SRCH_C
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
/* functions for searching within a vstr */
#include "main.h"

size_t vstr_srch_chr_fwd(const Vstr_base *base, size_t pos, size_t len,
                         int srch)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = pos;
 char *scan_str = NULL;
 size_t scan_len = 0;
 char *found = NULL;

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);
 
 ret = len + scan_len;

 do
 {
  if (scan->type != VSTR_TYPE_NODE_NON)
  {
   found = memchr(scan_str, srch, scan_len);
   if (found)
     return (pos + ((ret - len) - scan_len) + (found - scan_str));
  }
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (0);
}

/* inifficient, keep search forward until you don't find one
 * ... then use the last find */
static size_t vstr__srch_chr_rev_slow(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      int srch)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;
 char *found = NULL;
 size_t orig_len = 0;
 size_t ret = 0;
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 orig_len = len + scan_len;

 do
 {
  if (scan->type != VSTR_TYPE_NODE_NON)
  {
   size_t off = 0;
   
   while ((off < scan_len) &&
          (found = memchr(scan_str + off, srch, scan_len - off)))
   {
    ret = (pos + ((orig_len - len) - scan_len) + (found - scan_str));
    off = (found - scan_str) + 1;
   }
  }
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

size_t vstr_srch_chr_rev(const Vstr_base *base, size_t pos, size_t len,
                         int srch)
{ /* FIXME: this needs to use iovec to walk backwards */
 if (!vstr__base_iovec_valid((Vstr_base *)base))
   return (vstr__srch_chr_rev_slow(base, pos, len, srch));

 return (vstr__srch_chr_rev_slow(base, pos, len, srch));
}

size_t vstr_srch_buf_fwd(const Vstr_base *base, size_t pos, size_t len,
                         const void *const str, const size_t str_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;
 size_t non_len = 0;
 
 if (!str_len)
   return (0);

 if (str_len > base->len)
   return (0);

 if ((pos + str_len - 1) > base->len)
   return (0);
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 ret = len + scan_len;

 do
 {
  if ((scan->type == VSTR_TYPE_NODE_NON) && !str)
  {
   if (!vstr_cmp_buf(base, pos + (ret - len), str_len, NULL, 0))
     return (pos + (ret - len));
   goto next_loop;
  }
  if (!str)
  {
   non_len = 0;
   goto next_loop;
  }
  
  if (scan->type == VSTR_TYPE_NODE_NON)
    goto next_loop;
  
  while (scan_len > 0)
  {
   size_t tmp = scan_len;
   
   if (tmp > str_len)
     tmp = str_len;
   
   if (!memcmp(scan_str, str, scan_len) &&
       !vstr_cmp_buf(base, pos + ((ret - len) - scan_len), str_len, str, 0))
     return (pos + ((ret - len) - scan_len));
   
   ++scan_str;
   --scan_len;
  }
  
 next_loop:
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)) &&
          ((base->len - (pos + (ret - len))) >= str_len));
 
 return (0);
}
