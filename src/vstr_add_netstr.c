#define VSTR_ADD_NETSTR_C
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
/* netstr2 like netstr but allows leading '0' characters */

#include "main.h"

#ifndef VSTR_AUTOCONF_ULONG_MAX_LEN
/* only used as a variable if can't work i tout at compile time */
size_t vstr__netstr2_ULONG_MAX_len = 0;
#endif

size_t vstr_nx_add_netstr2_beg(Vstr_base *base, size_t pos)
{
 size_t tmp = 0;
 size_t ret = 0;

 assert(pos <= base->len);
 if (pos > base->len)
   return (0);

 ret = pos + 1;
 /* number will be overwritten so it's ok in OS/compile default locale */
 tmp = vstr_nx_add_sysfmt(base, pos, "%lu%c", ULONG_MAX, VSTR__ASCII_COLON());

 if (!tmp)
    return (0);

 --tmp; /* remove comma from len */
 
 assert(!VSTR__ULONG_MAX_LEN || (tmp == VSTR__ULONG_MAX_LEN));
 VSTR__ULONG_MAX_SET_LEN(tmp);

 assert(vstr_nx_export_chr(base, ret + VSTR__ULONG_MAX_LEN) ==
        VSTR__ASCII_COLON());
 
 return (ret);
}

static int vstr__netstr_end_start(Vstr_base *base,
                                  size_t netstr_beg_pos, size_t netstr_end_pos,
                                  size_t *count, char *buf)
{
  const char comma[1] = {VSTR__ASCII_COMMA()};
  size_t netstr_len = 0;
  
  if (!VSTR__ULONG_MAX_LEN)
    return (FALSE);

 if (netstr_beg_pos >= (netstr_end_pos - VSTR__ULONG_MAX_LEN + 1))
   return (FALSE);

 if (netstr_end_pos > base->len)
   return (FALSE);

 assert(vstr_nx_export_chr(base, netstr_beg_pos + VSTR__ULONG_MAX_LEN) ==
        VSTR__ASCII_COLON());
 
 /* + 1 because of the ':' */
 netstr_len = netstr_end_pos - (netstr_beg_pos + VSTR__ULONG_MAX_LEN);

 if (!vstr_nx_add_buf(base, netstr_end_pos, comma, 1))
   return (FALSE);
 
 *count = VSTR__ULONG_MAX_LEN;
 while (netstr_len)
 {
  int off = netstr_len % 10;
  
  buf[--*count] = VSTR__ASCII_DIGIT_0() + off;
  
  netstr_len /= 10;
 }
 
 return (TRUE);
}

int vstr_nx_add_netstr2_end(Vstr_base *base,
                            size_t netstr_beg_pos, size_t netstr_end_pos)
{
  size_t count = 0;
  char buf[BUF_NUM_TYPE_SZ(unsigned long)];
  
  assert(sizeof(buf) >= VSTR__ULONG_MAX_LEN);
  assert(netstr_beg_pos);
  assert(netstr_end_pos);
  
  if (!vstr__netstr_end_start(base, netstr_beg_pos, netstr_end_pos,
                              &count, buf))
    return (FALSE);
  assert(count <= VSTR__ULONG_MAX_LEN);
  
  vstr_nx_sub_rep_chr(base, netstr_beg_pos, count, '0', count);

  if (count != VSTR__ULONG_MAX_LEN)
    vstr_nx_sub_buf(base, netstr_beg_pos + count, VSTR__ULONG_MAX_LEN - count,
                    buf + count, VSTR__ULONG_MAX_LEN - count);
  
  return (TRUE);
}

/* NOTE: might want to use vstr_add_pos_buf()/_ref() eventually */
size_t vstr_nx_add_netstr_beg(Vstr_base *base, size_t pos)
{
  return (vstr_nx_add_netstr2_beg(base, pos));
}

int vstr_nx_add_netstr_end(Vstr_base *base,
                           size_t netstr_beg_pos, size_t netstr_end_pos)
{
  size_t count = 0;
  char buf[BUF_NUM_TYPE_SZ(unsigned long)];
  
  assert(sizeof(buf) >= VSTR__ULONG_MAX_LEN);
  assert(netstr_beg_pos);
  assert(netstr_end_pos);
  
  if (!vstr__netstr_end_start(base, netstr_beg_pos, netstr_end_pos,
                              &count, buf))
    return (FALSE);
  assert(count <= VSTR__ULONG_MAX_LEN);
  
  if (count == VSTR__ULONG_MAX_LEN)
  { /* here we delete, so need to keep something */
    buf[--count] = VSTR__ASCII_DIGIT_0();
  }
  
  if (count && !vstr_nx_del(base, netstr_beg_pos, count))
  {
    vstr_nx_del(base, netstr_end_pos + 1, 1); /* remove comma, always works */
    return (FALSE);
  }
  vstr_nx_sub_buf(base, netstr_beg_pos, VSTR__ULONG_MAX_LEN - count,
                  buf + count, VSTR__ULONG_MAX_LEN - count);
  
  return (TRUE);
}
