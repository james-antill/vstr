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
  unsigned int num_base = flags & VSTR__MASK_PARSE_NUM_BASE;
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

  assert(!(auto_base && (flags & VSTR_FLAG_PARSE_NUM_NO_BEG_ZERO)));
  
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
    tmp = vstr_nx_spn_buf_fwd(base, pos, len, &sym_space, 1);
    if (tmp >= len)
    {
      *err = VSTR_TYPE_PARSE_NUM_ERR_ONLY_S;
      return (0);
    }
    
    pos += tmp;
    len -= tmp;
  }
  
  if (!(flags & VSTR_FLAG_PARSE_NUM_NO_BEG_PM))
  {
    tmp = vstr_nx_spn_buf_fwd(base, pos, len, &sym_minus, 1);
    if (tmp > 1)
    {
      *err = VSTR_TYPE_PARSE_NUM_ERR_OOB;
      return (0);
    }
    else if (!tmp)
    {
      tmp = vstr_nx_spn_buf_fwd(base, pos, len, &sym_plus, 1);
      if (tmp > 1)
      {
        *err = VSTR_TYPE_PARSE_NUM_ERR_OOB;
        return (0);
      }
    }
    else
      *is_neg = TRUE;
    
    pos += tmp;
    len -= tmp;
  }

  if (!len)
  {
    *err = VSTR_TYPE_PARSE_NUM_ERR_ONLY_SPM;
    return (0);
  }
  
  tmp = vstr_nx_spn_buf_fwd(base, pos, len, &num_0, 1);
  if (tmp && (flags & VSTR_FLAG_PARSE_NUM_NO_BEG_ZERO))
  {
    *passed_len = len - 1;
    if ((tmp != 1) || (len != 1))
      *err = VSTR_TYPE_PARSE_NUM_ERR_BEG_ZERO;
    
    return (1);
  }

  if ((tmp == 1) && (auto_base || (num_base == 16)))
  {
    char xX[2];

    if (tmp == len)
    {
      *passed_len = 0;
      return (1);
    }

    pos += tmp;
    len -= tmp;

    xX[0] = let_x_low;
    xX[1] = let_X_high;
    tmp = vstr_nx_spn_buf_fwd(base, pos, len, xX, 2);

    if (tmp > 1)
    { /* It's a 0 */
      *err = VSTR_TYPE_PARSE_NUM_ERR_OOB;
      *passed_len = len;
      return (1);
    }
    if (tmp == 1)
    {
      if (auto_base)
        num_base = 16;
      
      pos += tmp;
      len -= tmp;
      
      if (!len)
      {
        *passed_pos = pos + len;
        *passed_len = 0;
        *err = VSTR_TYPE_PARSE_NUM_ERR_ONLY_SPMX;
        return (0);
      }
      
      tmp = vstr_nx_spn_buf_fwd(base, pos, len, &num_0, 1);
    }
    else if (auto_base)
      num_base = 8;
  }
  else if (tmp && auto_base)
    num_base = 8;
  else if (auto_base)
    num_base = 10;

  if (tmp == len)
  { /* It's a 0 */
    *passed_len = 0;
    return (1);
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
    return (0); \
  else if (num_base == 1) goto ret_num_len

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
    unsigned char scan = vstr_nx_export_chr(base, pos); \
    const char *end = NULL; \
    unsigned int add_num = 0; \
    num_type old_ret = ret; \
    \
    if (ret && (scan == sym_sep)) \
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
 ret_num_len: \
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
 ret_num_len: \
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
    
short vstr_nx_parse_short(const Vstr_base *base, size_t pos, size_t len,
                          unsigned int flags, size_t *ret_len,
                          unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(unsigned short, SHRT_MAX); }

unsigned short vstr_nx_parse_ushort(const Vstr_base *base,
                                    size_t pos, size_t len,
                                    unsigned int flags, size_t *ret_len,
                                    unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(unsigned short); }

int vstr_nx_parse_int(const Vstr_base *base, size_t pos, size_t len,
                      unsigned int flags, size_t *ret_len, unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(unsigned int, INT_MAX); }

unsigned int vstr_nx_parse_uint(const Vstr_base *base, size_t pos, size_t len,
                                unsigned int flags, size_t *ret_len,
                                unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(unsigned int); }

long vstr_nx_parse_long(const Vstr_base *base, size_t pos, size_t len,
                        unsigned int flags, size_t *ret_len,
                        unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(unsigned long, LONG_MAX); }

unsigned long vstr_nx_parse_ulong(const Vstr_base *base, size_t pos, size_t len,
                                  unsigned int flags, size_t *ret_len,
                                  unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(unsigned long); }

intmax_t vstr_nx_parse_intmax(const struct Vstr_base *base,
                              size_t pos, size_t len,
                              unsigned int flags, size_t *ret_len,
                              unsigned int *err)
{ VSTR__PARSE_NUM_SFUNC(uintmax_t, INTMAX_MAX); }

uintmax_t vstr_nx_parse_uintmax(const struct Vstr_base *base,
                                size_t pos, size_t len,
                                unsigned int flags, size_t *ret_len,
                                unsigned int *err)
{ VSTR__PARSE_NUM_UFUNC(uintmax_t); }

static int vstr__parse_ipv4_netmask(const struct Vstr_base *base,
                                    size_t pos, size_t *passed_len,
                                    unsigned int flags,
                                    unsigned int num_flags,
                                    unsigned char sym_dot,
                                    unsigned int *cidr, unsigned int *err)
{
  int zero_rest = FALSE;
  size_t len = *passed_len;
  unsigned int scan = 0;
  
  while (len)
  {
    unsigned int tmp = 0;
    size_t num_len = 3;
    
    if (num_len > len)
      num_len = len;
    
    tmp = vstr_nx_parse_uint(base, pos, num_len, 10 | num_flags,
                             &num_len, NULL);
    if (!num_len)
    {
      *cidr = scan * 8;
      break;
    }
    
    pos += num_len;
    len -= num_len;
    
    if (zero_rest && tmp)
    {
      *err = VSTR_TYPE_PARSE_IPV4_ERR_NETMASK_OOB;
      return (FALSE);
    }
    else if (!zero_rest)
    {
      if (tmp > 255)
      {
        *err = VSTR_TYPE_PARSE_IPV4_ERR_NETMASK_OOB;
        return (FALSE);
      }
      
      if ((tmp != 255) || (scan == 3))
      {
        *cidr = scan * 8;
        switch (tmp)
        {
          default:
            *err = VSTR_TYPE_PARSE_IPV4_ERR_NETMASK_OOB;
            return (FALSE);
            
          case 255: *cidr += 8; break;
          case 254: *cidr += 7; break;
          case 252: *cidr += 6; break;
          case 248: *cidr += 5; break;
          case 240: *cidr += 4; break;
          case 224: *cidr += 3; break;
          case 192: *cidr += 2; break;
          case 128: *cidr += 1; break;
          case   0:             break;
        }
        zero_rest = TRUE;
      }
    }
    
    ++scan;

    if (scan == 4)
      break;

    if (len && (vstr_nx_export_chr(base, pos) != sym_dot))
      break;
    
    if (len)
    { /* skip the dot */
      ++pos;
      --len;
    }
  }
  if ((flags & VSTR_FLAG_PARSE_IPV4_NETMASK_FULL) && (scan != 4))
  {
    *err = VSTR_TYPE_PARSE_IPV4_ERR_NETMASK_FULL;
    return (FALSE);
  }

  *passed_len = len;

  return (TRUE);
}

static int vstr__parse_ipv4_cidr(const struct Vstr_base *base,
                                 size_t pos, size_t *passed_len,
                                 unsigned int flags,
                                 unsigned int num_flags,
                                 unsigned char sym_dot,
                                 unsigned int *cidr, unsigned int *err)
{
  size_t len = *passed_len;
  size_t num_len = 0;

  if (len)
    *cidr = vstr_nx_parse_uint(base, pos, len, 10 | num_flags,
                               &num_len, NULL);
  if (num_len)
  {
    if ((flags & VSTR_FLAG_PARSE_IPV4_NETMASK) &&
        (*cidr > 32) && (len > num_len) &&
        (vstr_nx_export_chr(base, pos + num_len) == sym_dot))
    {
      if (!vstr__parse_ipv4_netmask(base, pos, &len, flags, num_flags,
                                    sym_dot, cidr, err))
        return (FALSE);
    }
    else if (*cidr <= 32)
    {
      pos += num_len;
      len -= num_len;
    }
    else
    {
      *err = VSTR_TYPE_PARSE_IPV4_ERR_CIDR_OOB;
      return (FALSE);
    }
  }
  else if (flags & VSTR_FLAG_PARSE_IPV4_CIDR_FULL)
  {
    *err = VSTR_TYPE_PARSE_IPV4_ERR_CIDR_FULL;
    return (FALSE);
  }
  else
    *cidr = 32;

  *passed_len = len;

  return (TRUE);
}

int vstr_nx_parse_ipv4(const struct Vstr_base *base,
                       size_t pos, size_t len,
                       unsigned char *ips, unsigned int *cidr,
                       unsigned int flags, size_t *ret_len, unsigned int *err)
{
  size_t orig_len = len;
  unsigned char sym_slash = 0x2F;
  unsigned char sym_dot = 0x2E;
  unsigned int num_flags = VSTR_FLAG_PARSE_NUM_NO_BEG_PM;
  unsigned int scan = 0;
  unsigned int dummy_err = 0;
  
  assert(ips);

  if (ret_len) *ret_len = 0;
  if (!err)
    err = &dummy_err;
  
  *err = 0;
  
  if (flags & VSTR_FLAG_PARSE_IPV4_LOCAL)
  {
    num_flags |= VSTR_FLAG_PARSE_NUM_LOCAL;
    sym_slash = '/';
    sym_dot = '.';
  }
  
  if (!(flags & VSTR_FLAG_PARSE_IPV4_ZEROS))
    num_flags |= VSTR_FLAG_PARSE_NUM_NO_BEG_ZERO;

  while (len)
  {
    unsigned int tmp = 0;
    size_t num_len = 3;
    
    if (num_len > len)
      num_len = len;
    
    tmp = vstr_nx_parse_uint(base, pos, num_len, 10 | num_flags,
                             &num_len, NULL);
    if (!num_len)
      break;

    if (tmp > 255)
    {
      *err = VSTR_TYPE_PARSE_IPV4_ERR_IPV4_OOB;
      return (FALSE);
    }
    
    pos += num_len;
    len -= num_len;

    ips[scan++] = tmp;

    if (scan == 4)
      break;
    
    if (len && (vstr_nx_export_chr(base, pos) == sym_slash))
      break;
    
    if (len && (vstr_nx_export_chr(base, pos) != sym_dot))
      break;
    
    if (len)
    { /* skip the dot */
      ++pos;
      --len;
    }

    if (len && (vstr_nx_export_chr(base, pos) == sym_slash))
      break;
  }

  if (!len && (scan == 4))
    goto ret_len;

  if ((scan != 4) && (flags & VSTR_FLAG_PARSE_IPV4_FULL))
  {
    *err = VSTR_TYPE_PARSE_IPV4_ERR_IPV4_FULL;
    return (FALSE);
  }
  
  while (scan < 4)
    ips[scan++] = 0;

  if (cidr)
    *cidr = 32;
  
  if (!len)
    goto ret_len;

  if (vstr_nx_export_chr(base, pos) == sym_slash)
  {
    if (flags & VSTR_FLAG_PARSE_IPV4_CIDR)
    {
      ++pos;
      --len;
      
      if (!vstr__parse_ipv4_cidr(base, pos, &len, flags, num_flags, sym_dot,
                                 cidr, err))
        return (FALSE);
    }
    else if (flags & VSTR_FLAG_PARSE_IPV4_NETMASK)
    {
      ++pos;
      --len;
      
      if (!vstr__parse_ipv4_netmask(base, pos, &len, flags, num_flags, sym_dot,
                                    cidr, err))
        return (FALSE);
    }
  }
  
  if ((flags & VSTR_FLAG_PARSE_IPV4_ONLY) && len)
    *err = VSTR_TYPE_PARSE_IPV4_ERR_ONLY;
  
 ret_len:
  if (ret_len)
    *ret_len = orig_len - len;

  return (TRUE);
}
