#define VSTR_PARSE_C
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
/* functions for parsing data out of the Vstr */
#include "main.h"


/* basically uses: [ ]*[-+](0x|0X|0)[0-9a-z_]+ */
static int vstr__parse_num(const Vstr_base *base,
                           size_t *passed_pos, size_t *passed_len,
                           unsigned int flags, int *is_neg, unsigned int *err)
{
  size_t pos = *passed_pos;
  size_t len = *passed_len;
  unsigned int num_base = flags & VSTR_FLAG_PARSE_NUM_MASK;
  int auto_base = FALSE;
  char num_0 = '0';
  char let_x_low = 'x';
  char let_X_high = 'X';
  char sym_minus = '-';
  char sym_plus = '+';
  char sym_space = ' ';
  size_t tmp = 0;
  
  if (!num_base)
    auto_base = TRUE;
  else if (num_base > 36)
    num_base = 36;
  else if (num_base == 1)
    ++num_base;

  if (!(flags & VSTR_FLAG_PARSE_NUM_LOCAL))
  {
    num_0 = 0x30;
    let_x_low = 0x78;
    let_X_high = 0x58;
    sym_minus = 0x2D;
    sym_plus = 0x2B;
    sym_space = 0x20;
  }
     
  if (flags & VSTR_FLAG_PARSE_NUM_SPACE)
  {
    tmp = vstr_spn_buf_fwd(base, pos, len, &sym_space, 1);
    if (tmp >= len)
    {
      *err = VSTR_TYPE_PARSE_NUM_ERR_ONLY_S;
      return (0);
    }
    
    pos += tmp;
    len -= tmp;
  }
  
  tmp = vstr_spn_buf_fwd(base, pos, len, &sym_minus, 1);
  if (tmp > 1)
  {
    *err = VSTR_TYPE_PARSE_NUM_ERR_PARSE_ERR;
    return (0);
  }
  else if (!tmp)
  {
    tmp = vstr_spn_buf_fwd(base, pos, len, &sym_plus, 1);
    if (tmp > 1)
    {
      *err = VSTR_TYPE_PARSE_NUM_ERR_PARSE_ERR;
      return (0);
    }
  }
  else
    *is_neg = TRUE;
  
  pos += tmp;
  len -= tmp;

  if (!len)
  {
    *err = VSTR_TYPE_PARSE_NUM_ERR_ONLY_SPM;
    return (0);
  }
  
  tmp = vstr_spn_buf_fwd(base, pos, len, &num_0, 1);
  if ((tmp == 1) && (auto_base || (num_base == 16)))
  {
    char xX[2];

    if (tmp >= len)
    {
      *passed_pos = pos + len;
      *passed_len = 0;
      return (num_base);
    }

    pos += tmp;
    len -= tmp;

    xX[0] = let_x_low;
    xX[1] = let_X_high;
    tmp = vstr_spn_buf_fwd(base, pos, len, xX, 2);

    if (tmp > 1)
    { /* It's a 0 */
      *err = VSTR_TYPE_PARSE_NUM_ERR_OOB;
      *passed_pos = pos + 1;
      *passed_len = len - 1;
      return (0);
    }
    if (tmp == 1)
    {
      num_base = 16;
      
      pos += tmp;
      len -= tmp;
      
      if (!len)
      {
        *err = VSTR_TYPE_PARSE_NUM_ERR_ONLY_SPMX;
        return (0);
      }
      
      tmp = vstr_spn_buf_fwd(base, pos, len, &num_0, 1);
    }
    else
      num_base = 8;
  }
  else if (tmp && auto_base)
    num_base = 8;
  else if (auto_base)
    num_base = 10;

  if (tmp >= len)
  { /* It's a 0 */
    *passed_pos = pos + len;
    *passed_len = 0;
    return (num_base);
  }

  pos += tmp;
  len -= tmp;
  
  *passed_pos = pos;
  *passed_len = len;

  return (num_base);
}

#define VSTR__PARSE_NUM_BEG_A(num_type) \
  unsigned int dummy_err; \
  num_type ret = 0; \
  unsigned int num_base = 0; \
  int is_neg = FALSE; \
  size_t orig_len = len; \
  unsigned char sym_sep = '_'; \
  unsigned int ascii_num_end = 0x39; \
  unsigned int ascii_let_low_end = 0x7A; \
  unsigned int ascii_let_high_end = 0x5A; \
  unsigned int local_num_end = '9'; \
  const char *local_let_low = "abcdefghijklmnopqrstuvwxyz"; \
  const char *local_let_high = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; \
  \
  if (ret_len) *ret_len = 0; \
  if (!err) err = &dummy_err; \
  *err = VSTR_TYPE_PARSE_NUM_ERR_NONE; \
  \
  if (!(num_base = vstr__parse_num(base, &pos, &len, flags, &is_neg, err))) \
    return (0)

#define VSTR__PARSE_NUM_BEG_S(num_type) \
  VSTR__PARSE_NUM_BEG_A(num_type)

#define VSTR__PARSE_NUM_BEG_U(num_type) \
  VSTR__PARSE_NUM_BEG_A(num_type); \
  \
  if (is_neg) \
  { \
    *err = VSTR_TYPE_PARSE_NUM_ERR_NEGATIVE; \
    return (0); \
  }

#define VSTR__PARSE_NUM_ASCII() do { \
  if (!(flags & VSTR_FLAG_PARSE_NUM_LOCAL)) \
  { \
    sym_sep = 0x5F; \
    \
    if (num_base <= 10) \
      ascii_num_end = 0x30 + num_base - 1; \
    else if (num_base > 10) \
    { \
      ascii_let_low_end = 0x61 + (num_base - 11); \
      ascii_let_high_end = 0x41 + (num_base - 11); \
    } \
  } \
  else if (num_base <= 10) \
    local_num_end = '0' + num_base - 1; \
} while (FALSE)

#define VSTR__PARSE_NUM_LOOP(num_type) do { \
  while (len) \
  { \
    unsigned char scan = vstr_export_chr(base, pos); \
    const char *end = NULL; \
    unsigned int add_num = 0; \
    num_type old_ret = ret; \
    \
    if (scan == sym_sep) \
    { \
      if (!(flags & VSTR_FLAG_PARSE_NUM_SEP)) break; \
      --len; \
      ++pos; \
      continue; \
    } \
    else if (flags & VSTR_FLAG_PARSE_NUM_LOCAL) \
    { \
      if ((scan >= '0') && (scan <= local_num_end)) \
        add_num = (scan - '0'); \
      else if (num_base <= 10) \
        break; \
      else if ((end = memchr(local_let_low, scan, num_base - 10))) \
        add_num = 10 + (end - local_let_low); \
      else if ((end = memchr(local_let_high, scan, num_base - 10))) \
        add_num = 10 + (end - local_let_high); \
      else \
        break; \
    } \
    else \
    { \
      if (scan < 0x30) break; \
      \
      if (scan <= ascii_num_end) \
        add_num = (scan - 0x30); \
      else if (num_base <= 10) \
        break; \
      else if ((scan >= 0x41) && (scan <= ascii_let_high_end)) \
        add_num = 10 + (scan - 0x41); \
      else if ((scan >= 0x61) && (scan <= ascii_let_low_end)) \
        add_num = 10 + (scan - 0x61); \
      else \
        break; \
    } \
    \
    ret = (ret * num_base) + add_num; \
    if ((flags & VSTR_FLAG_PARSE_NUM_OVERFLOW) && \
        (((ret - add_num) / num_base) != old_ret)) \
    { \
      *err = VSTR_TYPE_PARSE_NUM_ERR_OVERFLOW; \
      break; \
    } \
    \
    --len; \
    ++pos; \
  } \
} while (FALSE)

/* assume negative numbers can be one bigger than signed positive numbers */
#define VSTR__PARSE_NUM_END_S(num_max) do { \
  if ((flags & VSTR_FLAG_PARSE_NUM_OVERFLOW) && \
      ((ret - is_neg) > num_max)) \
  { \
    *err = VSTR_TYPE_PARSE_NUM_ERR_OVERFLOW; \
    ret = num_max + is_neg; \
  } \
  if (len && !*err) *err = VSTR_TYPE_PARSE_NUM_ERR_OOB; \
  \
  if (ret_len) \
    *ret_len = (orig_len - len); \
  \
  if (is_neg) \
    return (-ret); \
  \
  return (ret); \
} while (FALSE)

#define VSTR__PARSE_NUM_END_U() do { \
  if (len && !*err) *err = VSTR_TYPE_PARSE_NUM_ERR_OOB; \
  \
  if (ret_len) \
    *ret_len = (orig_len - len); \
  \
  return (ret); \
} while (FALSE)

#define VSTR__PARSE_NUM_SFUNC(num_type, num_max) do { \
  VSTR__PARSE_NUM_BEG_S(num_type); \
  VSTR__PARSE_NUM_ASCII(); \
  VSTR__PARSE_NUM_LOOP(num_type); \
  VSTR__PARSE_NUM_END_S(num_max); \
} while (FALSE)
    
#define VSTR__PARSE_NUM_UFUNC(num_type) do { \
  VSTR__PARSE_NUM_BEG_U(num_type); \
  VSTR__PARSE_NUM_ASCII(); \
  VSTR__PARSE_NUM_LOOP(num_type); \
  VSTR__PARSE_NUM_END_U(); \
} while (FALSE)
    
short vstr_parse_short(const Vstr_base *base, size_t pos, size_t len,
                       unsigned int flags, size_t *ret_len, unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(unsigned int, INT_MAX); }

unsigned short vstr_parse_ushort(const Vstr_base *base,
                                 size_t pos, size_t len,
                                 unsigned int flags, size_t *ret_len,
                                 unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(unsigned short); }

int vstr_parse_int(const Vstr_base *base, size_t pos, size_t len,
                   unsigned int flags, size_t *ret_len, unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(unsigned short, SHRT_MAX); }

unsigned int vstr_parse_uint(const Vstr_base *base, size_t pos, size_t len,
                             unsigned int flags, size_t *ret_len,
                             unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(unsigned int); }

long vstr_parse_long(const Vstr_base *base, size_t pos, size_t len,
                     unsigned int flags, size_t *ret_len,
                     unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(unsigned long, LONG_MAX); }

unsigned long vstr_parse_ulong(const Vstr_base *base, size_t pos, size_t len,
                               unsigned int flags, size_t *ret_len,
                               unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(unsigned long); }

intmax_t vstr_parse_intmax(const struct Vstr_base *base,
                           size_t pos, size_t len,
                           unsigned int flags, size_t *ret_len,
                           unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(uintmax_t, INTMAX_MAX); }

uintmax_t vstr_parse_uintmax(const struct Vstr_base *base,
                             size_t pos, size_t len,
                             unsigned int flags, size_t *ret_len,
                             unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(uintmax_t); }
