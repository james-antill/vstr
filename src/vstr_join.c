#define VSTR_SPLIT_C
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
/* functions to join sections into a Vstr (ala. perl join()) */
#include "main.h"

unsigned int vstr_join_buf(Vstr_base *base, size_t pos,
                           const void *buf, size_t buf_len,
                           const Vstr_base *from_base, Vstr_sects *sects,
                           unsigned int sect_beg, unsigned int sect_end,
                           unsigned int flags)
{
  size_t start_pos = pos + 1;
  size_t orig_len = base->len;
  unsigned int added = 0;
  unsigned int scan = sect_beg - 1;

  assert(sect_beg && sect_end);
  
  while (scan < sects->num)
  {
    if (!sects->ptr[scan].pos)
    { /* allow flag to do it anyway */
      ++scan;
      continue;
    }

    if (added)
      if (!vstr_add_buf(base, pos, buf, buf_len))
        goto failed_alloc;

    if (sects->ptr[scan].len)
    {
      /* allow flags for pass through */
      if (!vstr_add_vstr(base, pos, from_base,
                         sects->ptr[scan].pos, sects->ptr[scan].len, 0))
        goto failed_alloc;

      ++added;
    }

    ++scan;
  }

  return (added);
  
 failed_alloc:
  if (base->len - orig_len)
    vstr_del(base, start_pos, base->len - orig_len);
  
  return (0);
}
