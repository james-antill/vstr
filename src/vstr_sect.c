#define VSTR_SECT_C
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
/* functions to manage a Vstr section */
#include "main.h"

Vstr_sects *vstr_sects_make(unsigned int beg_num)
{
  Vstr_sects *sects = malloc(sizeof(Vstr_sects));
  Vstr_sect_node *ptr = NULL;
  if (!sects)
    return (NULL);

  if (beg_num && !(ptr = malloc(sizeof(Vstr_sect_node) * beg_num)))
  {
    free(sects);
    return (NULL);
  }

  VSTR_SECTS_INIT(sects, beg_num, ptr, TRUE);

  return (sects);
}

void vstr_sects_free(Vstr_sects *sects)
{
  if (!sects)
    return;

  if (sects->free_ptr)
    free(sects->ptr);

  free(sects);
}

static int vstr__sects_add(Vstr_sects *sects)
{
  Vstr_sect_node *ptr = NULL;
  unsigned int sz = sects->sz;
  
  if (sects->alloc_double)
    sz <<= 1;
  else
    ++sz;
  
  if (!(ptr = realloc(sects->ptr, sizeof(Vstr_sect_node) * sz)))
  {
    sects->malloc_bad = TRUE;
    return (FALSE);
  }

  sects->ptr = ptr;
  sects->sz = sz;

  return (TRUE);
}

static int vstr__sects_del(Vstr_sects *sects)
{
  Vstr_sect_node *ptr = NULL;
  unsigned int sz = sects->sz;
  
  sz >>= 1;
  assert(sz >= sects->num);
  
  if (!(ptr = realloc(sects->ptr, sizeof(Vstr_sect_node) * sz)))
  {
    sects->malloc_bad = TRUE;
    return (FALSE);
  }

  sects->ptr = ptr;
  sects->sz = sz;

  return (TRUE);
}

int vstr_extern_inline_sects_add(Vstr_sects *sects,
                                 size_t pos __attribute__((unused)),
                                 size_t len __attribute__((unused)))
{
  /* see vstr-extern.h for why */
  assert(sizeof(struct Vstr_sects) >= sizeof(struct Vstr_sect_node));

  return (vstr__sects_add(sects));
}

int vstr_sects_del(Vstr_sects *sects, unsigned int num)
{
  assert(sects->sz && num);
  assert(sects->num >= num);
  
  if (!sects->sz || !num)
    return (FALSE);
  
  if (num > sects->num)
    return (FALSE);

  if (!sects->ptr[num - 1].pos)
    return (FALSE);

  sects->ptr[num - 1].pos = 0;

  while (sects->num && !sects->ptr[sects->num - 1].pos)
    --sects->num;

  if (sects->can_del_sz && (sects->num < (sects->sz / 2)))
    vstr__sects_del(sects);
  
  return (TRUE);
}

unsigned int vstr_sects_srch(Vstr_sects *sects, size_t pos, size_t len)
{
  unsigned int count = 0;

  if (!sects->sz)
    return (0);
  
  while (count < sects->num)
  {
    size_t scan_pos = VSTR_SECTS_NUM(sects, count)->pos;
    size_t scan_len = VSTR_SECTS_NUM(sects, count)->len;

    ++count;
    
    if ((scan_pos == pos) && (scan_len == len))
      return (count);
  }
  
  return (0);
}

int vstr_sects_foreach(const Vstr_base *base,
                       Vstr_sects *sects, const unsigned int flags,
                       unsigned int (*foreach_func)(const Vstr_base *,
                                                    size_t, size_t, void *),
                       void *data)
{
  unsigned int count = 0;
  unsigned int scan = 0;

  if (!sects->sz)
    return (0);

  if (flags & VSTR_FLAG_SECTS_FOREACH_BACKWARDS)
    scan = sects->num;
  
  while ((!(flags & VSTR_FLAG_SECTS_FOREACH_BACKWARDS) &&
          (scan < sects->num)) ||
         ((flags & VSTR_FLAG_SECTS_FOREACH_BACKWARDS) && scan))
  {
    size_t pos = 0;
    size_t len = 0;

    if (flags & VSTR_FLAG_SECTS_FOREACH_BACKWARDS)
      --scan;

    pos = sects->ptr[scan].pos;
    len = sects->ptr[scan].len;

    if (pos && (len || (flags & VSTR_FLAG_SECTS_FOREACH_ALLOW_NULL)))
    {
      ++count;

      switch ((*foreach_func)(base, pos, len, data))
      {
        default:
          assert(FALSE);
        case VSTR_TYPE_SECTS_FOREACH_DEF:
          break;
        case VSTR_TYPE_SECTS_FOREACH_DEL:
          sects->ptr[scan].pos = 0;
          break;
          
        case VSTR_TYPE_SECTS_FOREACH_RET:
          goto shorten_and_return;
      }
    }

    if (!(flags & VSTR_FLAG_SECTS_FOREACH_BACKWARDS))
      ++scan;
  }

 shorten_and_return:
  while (sects->num && !sects->ptr[sects->num - 1].pos)
    --sects->num;

  if (sects->can_del_sz && (sects->num < (sects->sz / 2)))
    vstr__sects_del(sects);

  return (count);
}

