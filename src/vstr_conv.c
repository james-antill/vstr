#define VSTR_CONV_C
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
          ++scan_str; \
        } while ((scan_len > 0) && \
                 ((start_scan_len - scan_len) <= base->conf->buf_sz) && \
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

int vstr_nx_conv_lowercase(Vstr_base *base, size_t pos, size_t passed_len)
{
  size_t len = passed_len;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  unsigned int extra_nodes = 0;
  
  VSTR__BUF_NEEDED(VSTR__IS_ASCII_UPPER(*scan_str), extra_nodes);  
  
  if (base->conf->spare_buf_num < extra_nodes)
  {
    extra_nodes -= base->conf->spare_buf_num;
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                                 extra_nodes) != extra_nodes)
      return (FALSE);
  }
  
  while (len)
  {
    char tmp = vstr_nx_export_chr(base, pos);
    
    if (VSTR__IS_ASCII_UPPER(tmp))
    {
      tmp = VSTR__TO_ASCII_LOWER(tmp);
      
      if (!vstr_nx_sub_buf(base, pos, 1, &tmp, 1))
        return (FALSE);
    }
    
    ++pos;
    --len;
  }
  
  return (TRUE);
  
 malloc_buf_needed_fail:
  return (FALSE);
}

int vstr_nx_conv_uppercase(Vstr_base *base, size_t pos, size_t passed_len)
{
  size_t len = passed_len;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  unsigned int extra_nodes = 0;

  VSTR__BUF_NEEDED(VSTR__IS_ASCII_LOWER(*scan_str), extra_nodes);
  
  if (base->conf->spare_buf_num < extra_nodes)
  {
    extra_nodes -= base->conf->spare_buf_num;
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                                 extra_nodes) != extra_nodes)
      return (FALSE);
  }
  
  while (len)
  {
    char tmp = vstr_nx_export_chr(base, pos);
    
    if (VSTR__IS_ASCII_LOWER(tmp))
    {
      tmp = VSTR__TO_ASCII_UPPER(tmp);
      
      if (!vstr_nx_sub_buf(base, pos, 1, &tmp, 1))
      {
        assert(FALSE);
        return (FALSE);
      }
    }
    
    ++pos;
    --len;
  }
  
  return (TRUE);
  
 malloc_buf_needed_fail:
  return (FALSE);
}

/* is it a printable ASCII character */
#define VSTR__IS_ASCII_PRINTABLE(x, flags) ( \
 (VSTR__UC(x) == 0x00) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_NUL)  : \
 (VSTR__UC(x) == 0x07) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BEL)  : \
 (VSTR__UC(x) == 0x08) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BS)   : \
 (VSTR__UC(x) == 0x09) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HT)   : \
 (VSTR__UC(x) == 0x0A) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_LF)   : \
 (VSTR__UC(x) == 0x0B) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_VT)   : \
 (VSTR__UC(x) == 0x0C) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_FF)   : \
 (VSTR__UC(x) == 0x0D) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_CR)   : \
 (VSTR__UC(x) == 0x1B) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_ESC)  : \
 (VSTR__UC(x) == 0x20) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_SP)   : \
 (VSTR__UC(x) == 0x2C) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_COMMA): \
 (VSTR__UC(x) == 0x2E) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_DOT)  : \
 (VSTR__UC(x) == 0x5F) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW__)    : \
 (VSTR__UC(x) == 0x7F) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_DEL)  : \
 (VSTR__UC(x) == 0xA0) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HSP)  : \
 (VSTR__UC(x) >= 0xA1) ? (flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HIGH) : \
 (unsigned int)((VSTR__UC(x) >= 0x21) && (VSTR__UC(x) <= 0x7E)))

int vstr_nx_conv_unprintable_chr(Vstr_base *base, size_t pos, size_t passed_len,
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
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                                 extra_nodes) != extra_nodes)
      return (FALSE);
  }
   
  while (len)
  {
    char tmp = vstr_nx_export_chr(base, pos);

    if (!VSTR__IS_ASCII_PRINTABLE(tmp, flags) &&
        !vstr_nx_sub_buf(base, pos, 1, &swp, 1))
    {
      assert(FALSE);
      return (FALSE);
    }
    
    ++pos;
    --len;
  }

  return (TRUE);

 malloc_buf_needed_fail:
  return (FALSE);
}

int vstr_nx_conv_unprintable_del(Vstr_base *base, size_t pos, size_t passed_len,
                                 unsigned int flags)
{
  size_t len = passed_len;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  unsigned int extra_nodes[4] = {0};
  size_t del_pos = 0;
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan) goto malloc_buf_needed_fail;
  
  do
  {
    int done = FALSE;
    
    if ((scan->type == VSTR_TYPE_NODE_BUF) && !base->conf->split_buf_del)
      continue;
    if (scan->type == VSTR_TYPE_NODE_NON)
      continue;
    
    while (scan_len > 0)
    {
      if (VSTR__IS_ASCII_PRINTABLE(*scan_str, flags))
        done = TRUE;
      else if (done) /* deleteing from the begining of a node is always ok */
      {
        assert((scan->type > 0) && (scan->type <= 4));
        
        ++extra_nodes[scan->type - 1];
        
        do
        {
          --scan_len;
          ++scan_str;
        } while ((scan_len > 0) && !VSTR__IS_ASCII_PRINTABLE(*scan_str, flags));
        
        continue;
      }
      
      --scan_len;
      ++scan_str;
    }

  } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));

  assert(!extra_nodes[VSTR_TYPE_NODE_NON - 1]);
  assert(!extra_nodes[VSTR_TYPE_NODE_BUF - 1] || base->conf->split_buf_del);
  
  len = passed_len;

  if (base->conf->spare_buf_num < extra_nodes[VSTR_TYPE_NODE_BUF - 1])
  {
    size_t tmp = extra_nodes[VSTR_TYPE_NODE_BUF - 1];
    
    tmp -= base->conf->spare_buf_num;
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, tmp) != tmp)
      return (FALSE);
  }
   
  if (base->conf->spare_ptr_num < extra_nodes[VSTR_TYPE_NODE_PTR - 1])
  {
    size_t tmp = extra_nodes[VSTR_TYPE_NODE_PTR - 1];
    
    tmp -= base->conf->spare_ptr_num;
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_PTR, tmp) != tmp)
      return (FALSE);
  }
   
  if (base->conf->spare_ref_num < extra_nodes[VSTR_TYPE_NODE_REF - 1])
  {
    size_t tmp = extra_nodes[VSTR_TYPE_NODE_REF - 1];
    
    tmp -= base->conf->spare_ref_num;
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_REF, tmp) != tmp)
      return (FALSE);
  }
   
  while (len)
  {
    char tmp = vstr_nx_export_chr(base, pos);

    if (!VSTR__IS_ASCII_PRINTABLE(tmp, flags))
    {
      if (!del_pos)
        del_pos = pos;
      
      assert(pos >= del_pos);
    }
    else if (del_pos)
    {
      vstr_nx_del(base, del_pos, pos - del_pos);
      pos = del_pos;
      del_pos = 0;
    }
    
    ++pos;
    --len;
  }

  if (del_pos)
    vstr_nx_del(base, del_pos, pos - del_pos);

  return (TRUE);

 malloc_buf_needed_fail:
  return (FALSE);
}

#if 0
/* from section 6.7. of rfc2045 */
int vstr_conv_encode_qp(Vstr_base *base, size_t pos, size_t passed_len)
{
  
}

int vstr_conv_decode_qp(Vstr_base *base, size_t pos, size_t passed_len)
{
}
#endif

int vstr_nx_conv_encode_uri(Vstr_base *base, size_t pos, size_t len)
{
  Vstr_sects *sects = vstr_nx_sects_make(8);
  size_t count = 0;
  /* from section 2.4.3. of rfc2396 */
  char chrs_disallowed[] = {
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
   0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
   0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, /* control */
   0x20, /* space */
   0x22, 0x23, 0x25, 0x3C, 0x3E, /* delims */
   0x5B, 0x5C, 0x5D, 0x5E, 0x60, 0x7B, 0x7C, 0x7D, /* unwise */
   0x7F, /* control */
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
   0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
   0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
   0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
   0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
   0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
   0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
   0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
   0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9,
   0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
   0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
   0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF /* high ascii */
  };
  unsigned int extra_nodes = 0;

  if (!sects)
  {
    base->conf->malloc_bad = TRUE;
    return (FALSE);
  }
  
  while (len)
  {
    count = vstr_nx_cspn_chrs_fwd(base, pos, len,
                                  chrs_disallowed, sizeof(chrs_disallowed));
    pos += count;
    len -= count;

    if (!len)
      break;
    
    if (!vstr_nx_sects_add(sects, pos, 1))
    {
      vstr_nx_sects_free(sects);
      base->conf->malloc_bad = TRUE;
      return (FALSE);
    }

    ++pos;
    --len;
  }

  extra_nodes = sects->num * 3;
  if (base->conf->spare_buf_num < extra_nodes)
  {
    extra_nodes -= base->conf->spare_buf_num;
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                                 extra_nodes) != extra_nodes)
      return (FALSE);
  }
  
  while (sects->num--)
  {
    unsigned int bad = 0;
    char sub[3];
    const char *digits = "0123456789abcdef";
    
    assert(sects->ptr[sects->num].len == 1);

    bad = vstr_nx_export_chr(base, sects->ptr[sects->num].pos);
    sub[0] = '%';
    sub[1] = digits[((bad >> 4) & 0x0F)];
    sub[2] = digits[(bad & 0x0F)];

    vstr_nx_sub_buf(base, sects->ptr[sects->num].pos, 1, sub, 3);
  }
  
  vstr_nx_sects_free(sects);
  
  return (TRUE);
}

int vstr_nx_conv_decode_uri(Vstr_base *base, size_t pos, size_t len)
{
  Vstr_sects *sects = vstr_nx_sects_make(8);
  size_t srch_pos = 0;
  char buf_percent[1] = {0x25};
  unsigned int err = 0;
  size_t hex_len = 0;
  unsigned int extra_nodes = 0;
  
  if (!sects)
  {
    base->conf->malloc_bad = TRUE;
    return (FALSE);
  }
  
  while ((srch_pos = vstr_nx_srch_buf_fwd(base, pos, len, buf_percent, 1)))
  {
    size_t left = len - (srch_pos - pos);
    unsigned char sub = 0;
    
    if (left < 3)
      break;
    pos = srch_pos + 1;
    len = left - 1;

    sub = vstr_nx_parse_ushort(base, srch_pos + 1, 2, 16 |
                               VSTR_FLAG_PARSE_NUM_NO_BEG_PM,
                               &hex_len, &err);
    if (err)
      continue;
    assert(hex_len == 2);
    if (!vstr_nx_sects_add(sects, srch_pos, 3))
    {
      vstr_nx_sects_free(sects);
      base->conf->malloc_bad = TRUE;
      return (FALSE);
    }

    pos += 2;
    len -= 2;
  }

  extra_nodes = sects->num + 2;
  if (base->conf->spare_buf_num < extra_nodes)
  {
    extra_nodes -= base->conf->spare_buf_num;
    
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF,
                                 extra_nodes) != extra_nodes)
      return (FALSE);
  }

  while (sects->num--)
  {
    unsigned char sub = 0;

    assert(sects->ptr[sects->num].len == 3);
    sub = vstr_nx_parse_ushort(base, sects->ptr[sects->num].pos + 1, 2,
                               16 | VSTR_FLAG_PARSE_NUM_NO_BEG_PM,
                               &hex_len, &err);
    assert(!err);
    assert(hex_len == 2);

    vstr_nx_sub_buf(base, sects->ptr[sects->num].pos, 3, &sub, 1);
  }  
  
  vstr_nx_sects_free(sects);
  
  return (TRUE);
}
