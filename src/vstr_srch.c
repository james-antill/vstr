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

size_t vstr_nx_srch_chr_fwd(const Vstr_base *base, size_t pos, size_t len,
                            char srch)
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

static size_t vstr__srch_chr_rev_slow(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      char srch)
{
  size_t ret = 0;
  size_t scan_pos = pos;
  size_t scan_len = len;
  
  while ((scan_pos < (pos + len - 1)) &&
         scan_len)
  {
    size_t tmp = vstr_nx_srch_chr_fwd(base, scan_pos, scan_len, srch);
    if (!tmp)
      break;

    ret = tmp;
    
    scan_pos = ret + 1;
    scan_len = len - ((ret - pos) + 1);
  }

  return (ret);
}

static size_t vstr__srch_chr_rev_fast(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      char srch)
{
  unsigned int num = 0;
  unsigned int type = 0;
  size_t ret = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  char *found = NULL;
  
  if (!vstr__base_scan_rev_beg(base, pos, &len, &num, &type,
                               &scan_str, &scan_len))
    return (0);
  
  do
  {
    if (type != VSTR_TYPE_NODE_NON)
    {
      found = memrchr(scan_str, srch, scan_len);
      if (found)
        return (pos + ((ret - len) - scan_len) + (found - scan_str));
    }
  } while (vstr__base_scan_rev_nxt(base, &len, &num, &type,
                                   &scan_str, &scan_len));
  
  return (ret);
}

size_t vstr_nx_srch_chr_rev(const Vstr_base *base, size_t pos, size_t len,
                            char srch)
{
  if (base->iovec_upto_date)
    return (vstr__srch_chr_rev_fast(base, pos, len, srch));
  
  return (vstr__srch_chr_rev_slow(base, pos, len, srch));
}

size_t vstr_nx_srch_chrs_fwd(const Vstr_base *base, size_t pos, size_t len,
                             const char *srch, size_t chrs_len)
{
  size_t num = vstr_nx_cspn_chrs_fwd(base, pos, len, srch, chrs_len);

  if (num == len)
    return (0);

  return (pos + num);
}

size_t vstr_nx_srch_chrs_rev(const Vstr_base *base, size_t pos, size_t len,
                             const char *srch, size_t chrs_len)
{
  size_t num = vstr_nx_cspn_chrs_rev(base, pos, len, srch, chrs_len);

  if (num == len)
    return (0);
  
  return (pos + ((len - num) - 1));
}

size_t vstr_nx_csrch_chrs_fwd(const Vstr_base *base, size_t pos, size_t len,
                              const char *srch, size_t chrs_len)
{
  size_t num = vstr_nx_spn_chrs_fwd(base, pos, len, srch, chrs_len);

  if (num == len)
    return (0);

  return (pos + num);
}

size_t vstr_nx_csrch_chrs_rev(const Vstr_base *base, size_t pos, size_t len,
                              const char *srch, size_t chrs_len)
{
  size_t num = vstr_nx_spn_chrs_rev(base, pos, len, srch, chrs_len);

  if (num == len)
    return (0);
  
  return (pos + ((len - num) - 1));
}

size_t vstr_nx_srch_buf_fwd(const Vstr_base *base, size_t pos, size_t len,
                            const void *const str, const size_t str_len)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t orig_len = len;
 char *scan_str = NULL;
 size_t scan_len = 0;

 if (!len || (str_len > len))
   return (0);

 if (!str_len)
   return (pos);

 if (!str && !base->node_non_used)
   return (0);
 
 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 assert(orig_len == (len + scan_len));

 do
 {
   if ((scan->type == VSTR_TYPE_NODE_NON) && !str)
   {
     if (!vstr_nx_cmp_buf(base, pos + (orig_len - len), str_len, NULL, str_len))
       return (pos + ((orig_len - len) - scan_len));
     goto next_loop;
   }
   if (!str)
     goto next_loop;
   
   if (scan->type == VSTR_TYPE_NODE_NON)
     goto next_loop;
   
   /* find buf */
   while ((len + scan_len) >= str_len)
   {
     size_t tmp = 0;
     size_t beg_pos = 0;

     assert(scan_len);

     if (*scan_str != *(const char *)str)
     {
       char *som = NULL; /* start of a possible match */
       
       if (!(som = memchr(scan_str, *(const char *)str, scan_len)))
         goto next_loop;
       scan_len -= (som - scan_str);
       scan_str = som;
       continue;
     }
     
     tmp = scan_len;
     if (tmp > str_len)
       tmp = str_len;

     beg_pos = pos + ((orig_len - len) - scan_len);
     if (!memcmp(scan_str, str, tmp) &&
         ((tmp == str_len) ||
          !vstr_nx_cmp_buf(base, beg_pos + tmp, str_len - tmp,
                           (const char *)str + tmp, str_len - tmp)))
       return (beg_pos);
     
     ++scan_str;
     --scan_len;

     if (!scan_len) break;
   }
  
  next_loop:
   continue;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)) &&
          ((len + scan_len) >= str_len));
 
 return (0);
}

static size_t vstr__srch_buf_rev_slow(const Vstr_base *base,
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
    size_t tmp = vstr_nx_srch_buf_fwd(base, scan_pos, scan_len, str, str_len);
    if (!tmp)
      break;

    ret = tmp;
    
    scan_pos = ret + 1;
    scan_len = len - ((ret - pos) + 1);
  }

  return (ret);
}

static size_t vstr__srch_buf_rev_fast(const Vstr_base *base,
                                      size_t pos, size_t len,
                                      const void *const str,
                                      size_t str_len)
{
  unsigned int num = 0;
  unsigned int type = 0;
  size_t ret = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t non_len = 0;
  size_t orig_len = len;
  size_t scanned = 0;
  
  if (!len || (str_len > len))
    return (0);
  
  if (!str_len)
    return (pos + len - 1);
  
  if (!vstr__base_scan_rev_beg(base, pos, &len, &num, &type,
                               &scan_str, &scan_len))
    return (0);
  
  assert(orig_len == (len + scan_len));
  
  do
  {
    size_t back_len = 0;
    
    if ((type == VSTR_TYPE_NODE_NON) && !str)
    {
      non_len += scan_len;
      if (non_len >= str_len)
        return (pos + ((orig_len - len) - scan_len + (non_len - str_len)));
      goto next_loop;
    }
    non_len = 0;
    if (!str)
      goto next_loop;
   
    if (type == VSTR_TYPE_NODE_NON)
      goto next_loop;

    /* ignore everything smaller than what we are looking for */
    if ((scanned + scan_len) < str_len)
      goto next_loop;

    /* find buf */
    while (back_len < scan_len)
    {
      size_t tmp = 0;
      size_t beg_pos = 0;
      size_t rest = 0;

      ++back_len;

      rest = scan_len - back_len;
      if (scan_str[scan_len - back_len] != *(const char *)str)
      {
        char *som = NULL; /* start of a possible match */
        
        if (!rest || !(som = memrchr(scan_str, *(const char *)str, rest)))
          goto next_loop;
        back_len += ((scan_str + rest) - som);
        continue;
      }
      
      tmp = back_len;
      if (tmp > str_len)
        tmp = str_len;
      
      beg_pos = pos + (orig_len - scanned) + rest;
      if (!memcmp(scan_str + rest, str, tmp) &&
          ((tmp == str_len) ||
           !vstr_nx_cmp_buf(base, beg_pos + tmp, str_len - tmp,
                            (const char *)str + tmp, str_len - tmp)))
        return (beg_pos);
      
      ++back_len;
    }
    
   next_loop:
    scanned += scan_len;
    continue;
  } while (vstr__base_scan_rev_nxt(base, &len, &num, &type,
                                   &scan_str, &scan_len));
  
  return (ret);
}

size_t vstr_nx_srch_buf_rev(const Vstr_base *base, size_t pos, size_t len,
                            const void *const str, const size_t str_len)
{
  if (base->iovec_upto_date)
    return (vstr__srch_buf_rev_fast(base, pos, len, str, str_len));
  
  return (vstr__srch_buf_rev_slow(base, pos, len, str, str_len));
}

size_t vstr_nx_srch_vstr_fwd(const Vstr_base *base, size_t pos, size_t len,
                             const Vstr_base *ndl_base,
                             size_t ndl_pos, size_t ndl_len)
{ /* TODO: this could be faster, esp. with NON nodes */
  size_t scan_pos = pos;
  size_t scan_len = len;
  
  if (ndl_len > len)
    return (0);

  while ((scan_pos < (pos + len - 1)) &&
         (scan_len >= ndl_len))
  {
    size_t tmp = 0;
    char cspn[1];
    
    if (!vstr_nx_cmp(base, scan_pos, ndl_len, ndl_base, ndl_pos, ndl_len))
      return (scan_pos);

    tmp = 0;
    if ((cspn[0] = vstr_nx_export_chr(ndl_base, ndl_pos))) /* non nodes */
      tmp = vstr_nx_cspn_chrs_fwd(base, scan_pos, scan_len - ndl_len + 1,
                                  cspn, 1);
    
    if (!tmp) tmp = 1;
    
    scan_len -= tmp;
    scan_pos += tmp;
  }
  
  return (0);
}

static size_t vstr__srch_vstr_rev_slow(const Vstr_base *base,
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
    size_t tmp = vstr_nx_srch_vstr_fwd(base, scan_pos, scan_len,
                                       ndl_base, ndl_pos, ndl_len);
    if (!tmp)
      break;

    ret = tmp;
    
    scan_pos = ret + 1;
    scan_len = len - ((ret - pos) + 1);
  }

  return (ret);
}

size_t vstr_nx_srch_vstr_rev(const Vstr_base *base, size_t pos, size_t len,
                             const Vstr_base *ndl_base,
                             size_t ndl_pos, size_t ndl_len)
{
  return (vstr__srch_vstr_rev_slow(base, pos, len, ndl_base, ndl_pos, ndl_len));
}

unsigned int vstr_nx_srch_sects_add_buf_fwd(const Vstr_base *base,
                                            size_t pos, size_t len,
                                            Vstr_sects *sects,
                                            const void *const str,
                                            const size_t str_len)    
{
  unsigned int orig_num = sects->num;
  size_t srch_pos = 0;

  while ((srch_pos = vstr_nx_srch_buf_fwd(base, pos, len, str, str_len)))
  {
    if (!vstr_nx_sects_add(sects, srch_pos, str_len))
    {
      if (sects->malloc_bad)
      {
        assert(sects->num >= orig_num);
        sects->num = orig_num;
        return (0);
      }
      assert(!sects->can_add_sz);
      assert(sects->num == sects->sz);
    }

    len -= srch_pos - pos;
    len -= str_len;
    if (!len)
      break;
    pos = srch_pos + str_len;
  }

  return (sects->num - orig_num);
}
