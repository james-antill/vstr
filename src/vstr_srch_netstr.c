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

static size_t vstr__srch_netstr(const Vstr_base *base, size_t pos, size_t len,
                                size_t *ret_pos, size_t *ret_len, int netstr2)
{
 static char *buf = NULL;

 assert(pos && (pos <= base->len));
 
 if (!pos || (pos > base->len))
   return (0);

 /* setup beg */
 if (!vstr__netstr2_num_len)
 {
  Vstr_base *tmp = vstr_make_base(NULL);
  if (!tmp)
    goto malloc_failed;
  
  vstr__netstr2_num_len = vstr_add_fmt(tmp, 0, "%lu", ULONG_MAX);
  assert(vstr__netstr2_num_len || tmp->conf->malloc_bad);
  
  vstr_free_base(tmp);
  if (!vstr__netstr2_num_len)
    goto malloc_failed;
 }

 if (!buf)
 {
  buf = malloc(vstr__netstr2_num_len + 1);
  if (!buf)
    goto malloc_failed;
 }
 
 /* setup end */

 if ((pos + len - 1) > base->len)
   len = (base->len - pos) + 1;

 while (len > 0)
 {
  size_t tmp = len + vstr__netstr2_num_len;
  size_t first_char_pos = 0;
  unsigned int count = 0;
  
  assert((pos + len - 1) <= base->len);
  if ((pos + tmp - 1) > base->len)
    tmp = (base->len - pos) + 1;

  if (!(tmp = vstr_spn_buf_fwd(base, pos, tmp, "1234567890", 10)))
  {
   tmp = vstr_cspn_buf_fwd(base, pos, len, "1234567890", 10);
   if (!tmp)
     return (0);
   assert(tmp <= len);
   goto next_loop;
  }

  if (tmp > vstr__netstr2_num_len)
  {
   tmp -= vstr__netstr2_num_len;
   /* this recalcs the _spn_ number, but then I can't imagine this is
    * something that most people would want to do -- esp. as it means it's
    * very hard to find out later which number we start from for the netstr */
   goto next_loop;
  }
  
  if (netstr2 && (tmp != vstr__netstr2_num_len))
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

