#define VSTR_SRCH_NETSTR_C
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
/* netstr (http://cr.yp.to/proto/netstrings.txt). This is basically
 * <num ':' data ','>
 * where
 * num is an ascii number (base 10)
 * data is 8 bit binary data (Ie. any value 0 - 255 is allowed).
 */
/* netstr2 like netstr (http://cr.yp.to/proto/netstrings.txt)
 * but allows leading '0' characters */

#include "main.h"

#define VSTR__ASCII_DIGITS() VSTR__ASCII_DIGIT_0(), \
 VSTR__ASCII_DIGIT_0() + 1, \
 VSTR__ASCII_DIGIT_0() + 2, \
 VSTR__ASCII_DIGIT_0() + 3, \
 VSTR__ASCII_DIGIT_0() + 4, \
 VSTR__ASCII_DIGIT_0() + 5, \
 VSTR__ASCII_DIGIT_0() + 6, \
 VSTR__ASCII_DIGIT_0() + 7, \
 VSTR__ASCII_DIGIT_0() + 8, \
 VSTR__ASCII_DIGIT_0() + 9

static size_t vstr__srch_netstr(const Vstr_base *base, size_t pos, size_t len,
                                size_t *ret_pos, size_t *ret_len, int netstr2)
{
 static char *buf = NULL;

 assert(pos && (pos <= base->len));
 
 if (!pos || (pos > base->len))
   return (0);

 /* setup beg */
 if (!VSTR__ULONG_MAX_LEN)
 {
  Vstr_base *str1 = vstr_make_base(NULL);
  int tmp = 0;
  
  if (!str1)
    goto malloc_failed;
  
  tmp = vstr_add_fmt(str1, 0, "%lu", ULONG_MAX);
  vstr_free_base(str1);
  
  if (!VSTR__ULONG_MAX_SET_LEN(tmp))
    goto malloc_failed;
 }

 if (!buf)
 {
  buf = malloc(VSTR__ULONG_MAX_LEN + 1);
  if (!buf)
    goto malloc_failed;
 }
 
 /* setup end */

 if ((pos + len - 1) > base->len)
   len = (base->len - pos) + 1;

 while (len > 0)
 {
  size_t tmp = len + VSTR__ULONG_MAX_LEN;
  size_t first_char_pos = 0;
  unsigned int count = 0;
  static const char digits[10] = {VSTR__ASCII_DIGITS()};
  
  assert((pos + len - 1) <= base->len);
  if ((pos + tmp - 1) > base->len)
    tmp = (base->len - pos) + 1;

  if (!(tmp = vstr_spn_buf_fwd(base, pos, tmp, digits, 10)))
  {
   tmp = vstr_cspn_buf_fwd(base, pos, len, digits, 10);
   if (!tmp)
     return (0);
   assert(tmp <= len);
   goto next_loop;
  }

  if (tmp > VSTR__ULONG_MAX_LEN)
  {
   tmp -= VSTR__ULONG_MAX_LEN;
   /* this recalcs the _spn_ number, but then I can't imagine this is
    * something that most people would want to do -- esp. as it means it's
    * very hard to find out later which number we start from for the netstr */
   goto next_loop;
  }
  
  if (netstr2 && (tmp != VSTR__ULONG_MAX_LEN))
    goto next_loop;
   
  vstr_export_buf(base, pos, tmp + 1, buf);
  if (buf[tmp] != VSTR__ASCII_COLON())
    goto next_loop;
  buf[tmp] = 0;
  assert(strlen(buf) == tmp);
  
  first_char_pos = pos + tmp + 1;
  if (ret_pos)
    *ret_pos = first_char_pos;

  if (tmp > len)
    tmp = len;
  while ((tmp - count) > 0)
  {
   size_t found_len = 0;

   found_len = strtoul(buf + count, NULL, 10);

   if ((first_char_pos + found_len) > base->len)
   { /* > and not >= because of the ',' below */
    if (ret_len)
      *ret_len = found_len;
    return (0);
   }
   
   if (vstr_export_chr(base, first_char_pos + found_len) == VSTR__ASCII_COMMA())
   {
    if (ret_len)
      *ret_len = found_len;
    return (pos);
   }
   
   if (netstr2)
     goto next_loop;

   ++count;
  }
  assert(tmp == count);
  
 next_loop:
  if (tmp > len)
    return (0);

  pos += tmp;
  len -= tmp;  
 }

 return (0);
  
 malloc_failed:
 base->conf->malloc_bad = TRUE;
 return (0);
}

size_t vstr_srch_netstr2_fwd(const Vstr_base *base, size_t pos, size_t len,
                             size_t *ret_pos, size_t *ret_len)
{
 return (vstr__srch_netstr(base, pos, len, ret_pos, ret_len, TRUE));
}

size_t vstr_srch_netstr_fwd(const Vstr_base *base, size_t pos, size_t len,
                            size_t *ret_pos, size_t *ret_len)
{
 return (vstr__srch_netstr(base, pos, len, ret_pos, ret_len, FALSE));
}

