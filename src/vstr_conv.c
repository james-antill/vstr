#define VSTR_CONV_C
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
/* functions for converting data in vstrs */
#include "main.h"

/* can overestimate number of nodes needed, as you get a new one
 * everytime you get a new node but it's not a big problem */
#define VSTR__BUF_NEEDED(test, val_count) do { \
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len); \
  if (!scan) goto malloc_buf_needed_fail; \
  \
  do \
  { \
    if (scan->type == VSTR_TYPE_NODE_BUF) \
      continue; \
    if (scan->type == VSTR_TYPE_NODE_NON) \
      continue; \
    \
    while (scan_len > 0) \
    { \
      if (test) \
      { \
        size_t start_scan_len = scan_len; \
        \
        ++ (val_count); \
        \
        do \
        { \
          --scan_len; \
          ++scan; \
        } while ((scan_len > 0) && \
                 ((scan_len - start_scan_len) <= base->conf->buf_sz) && \
                 (test)); \
        \
        continue; \
      } \
      \
      --scan_len; \
      ++scan_str; \
    } \
  } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num, \
                                           scan, &scan_str, &scan_len))); \
  \
  len = passed_len; \
} while (FALSE)

int vstr_conv_lowercase(Vstr_base *base, size_t pos, size_t passed_len)
{
  size_t len = passed_len;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  unsigned int extra_nodes = 0;
  
  VSTR__BUF_NEEDED(VSTR__IS_ASCII_UPPER(*scan_str), extra_nodes);
  
  if (!extra_nodes) /* no convertion or all _buf and _non nodes */
  {
    assert(vstr__check_spare_nodes(base->conf));
    assert(vstr__check_real_nodes(base));

    scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
    if (!scan)
      return (FALSE);
    
    do
    {
      if (scan->type == VSTR_TYPE_NODE_NON)
        continue;
      
      while (scan_len > 0)
      {
        if (VSTR__IS_ASCII_UPPER(*scan_str))
          *scan_str = VSTR__TO_ASCII_LOWER(*scan_str);
        
        --scan_len;
        ++scan_str;
      }
    } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                             scan, &scan_str, &scan_len)));
    
    assert(vstr__check_spare_nodes(base->conf));
    assert(vstr__check_real_nodes(base));
  }
  else
  {
    if (base->conf->spare_buf_num < extra_nodes)
    {
      extra_nodes -= base->conf->spare_buf_num;
      
      if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                               extra_nodes) != extra_nodes)
        return (FALSE);
    }
    
    while (len)
    {
      char tmp = vstr_export_chr(base, pos);
      
      if (VSTR__IS_ASCII_UPPER(tmp))
      {
        tmp = VSTR__TO_ASCII_LOWER(tmp);
        
        if (!vstr_sub_buf(base, pos, 1, &tmp, 1))
          return (FALSE);
      }
      
      ++pos;
      --len;
    }
  }
  
  return (TRUE);
  
 malloc_buf_needed_fail:
  return (FALSE);
}

int vstr_conv_uppercase(Vstr_base *base, size_t pos, size_t passed_len)
{
  size_t len = passed_len;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  unsigned int extra_nodes = 0;

  VSTR__BUF_NEEDED(VSTR__IS_ASCII_LOWER(*scan_str), extra_nodes);
  
  if (!extra_nodes) /* no convertion or all _buf and _non nodes */
  {
    assert(vstr__check_spare_nodes(base->conf));
    assert(vstr__check_real_nodes(base));

    scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
    if (!scan)
      return (FALSE);
    
    do
    {
      if (scan->type == VSTR_TYPE_NODE_NON)
        continue;
      
      while (scan_len > 0)
      {
        if (VSTR__IS_ASCII_LOWER(*scan_str))
          *scan_str = VSTR__TO_ASCII_UPPER(*scan_str);
        
        --scan_len;
        ++scan_str;
      }
    } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                             scan, &scan_str, &scan_len)));

    assert(vstr__check_spare_nodes(base->conf));
    assert(vstr__check_real_nodes(base));
  }
  else
  {
    if (base->conf->spare_buf_num < extra_nodes)
    {
      extra_nodes -= base->conf->spare_buf_num;
      
      if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                               extra_nodes) != extra_nodes)
        return (FALSE);
    }
    
    while (len)
    {
      char tmp = vstr_export_chr(base, pos);
      
      if (VSTR__IS_ASCII_LOWER(tmp))
      {
        tmp = VSTR__TO_ASCII_UPPER(tmp);
        
        if (!vstr_sub_buf(base, pos, 1, &tmp, 1))
          return (FALSE);
      }
      
      ++pos;
      --len;
    }
  }
  
  return (TRUE);
  
 malloc_buf_needed_fail:
  return (FALSE);
}

#define VSTR__UCHAR(x) ((unsigned char)(x))
#define VSTR__IS_ASCII_PRINTABLE(x, flags) ( \
 (VSTR__UCHAR(x) == 0x00) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_NUL)  : \
 (VSTR__UCHAR(x) == 0x07) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BEL)  : \
 (VSTR__UCHAR(x) == 0x08) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BS)   : \
 (VSTR__UCHAR(x) == 0x09) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HT)   : \
 (VSTR__UCHAR(x) == 0x0A) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_LF)   : \
 (VSTR__UCHAR(x) == 0x0B) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_VT)   : \
 (VSTR__UCHAR(x) == 0x0C) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_FF)   : \
 (VSTR__UCHAR(x) == 0x0D) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_CR)   : \
 (VSTR__UCHAR(x) == 0x7F) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_DEL)  : \
 (VSTR__UCHAR(x) >  0xA1) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HIGH) : \
 ((VSTR__UCHAR(x) >= 0x20) && (VSTR__UCHAR(x) <= 0x7E)))

int vstr_conv_unprintable(Vstr_base *base, size_t pos, size_t passed_len,
                          unsigned int flags, char swp)
{
  size_t len = passed_len;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  unsigned int extra_nodes = 0;
  
  VSTR__BUF_NEEDED(!VSTR__IS_ASCII_PRINTABLE(*scan_str, flags), extra_nodes);

  if (base->conf->spare_buf_num < extra_nodes)
  {
    extra_nodes -= base->conf->spare_buf_num;
    
    if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                             extra_nodes) != extra_nodes)
      return (FALSE);
  }
   
  while (len)
  {
    char tmp = vstr_export_chr(base, pos);

    if (!VSTR__IS_ASCII_PRINTABLE(tmp, flags) &&
        !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
        
    ++pos;
    --len;
  }

  return (TRUE);

 malloc_buf_needed_fail:
  return (FALSE);
}
#undef VSTR__UCHAR
