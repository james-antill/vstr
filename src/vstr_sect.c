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

  if (beg_num && !(ptr = malloc(beg_num)))
  {
    free(sects);
    return (NULL);
  }

  VSTR_SECTS_INIT(sects, beg_num, ptr, TRUE);
  
  sects->free_ptr = TRUE;
  sects->can_add_sz = TRUE;

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

/* FIXME: inline */
int vstr_sects_add(Vstr_sects *sects, size_t pos, size_t len)
{
  /* see vstr-extern.h for why */
  assert(sizeof(struct Vstr_sects) >= sizeof(struct Vstr_sect_node));
  
  if (!sects->sz || (sects->num >= sects->sz))
  {
    if (!sects->can_add_sz)
      return (FALSE);
    
    if (!vstr__sects_add(sects))
      return (FALSE);
  }
  
  sects->ptr[sects->num].pos = pos;
  sects->ptr[sects->num].len = len;
  ++sects->num;
  
  return (TRUE);
}

int vstr_sects_del(Vstr_sects *sects, unsigned int num)
{
  assert(sects->sz && num);
  assert(sects->num >= num);
  
  if (!sects->sz || !num)
    return (FALSE);
  
  if (sects->num < num)
    return (FALSE);

  assert(sects->ptr[num - 1].pos);
  
  sects->ptr[num - 1].pos = 0;
  sects->ptr[num - 1].len = 0;
  
  return (TRUE);
}

unsigned int vstr_sects_srch(Vstr_sects *sects, size_t pos, size_t len)
{
  unsigned int count = 0;

  if (!sects->sz)
    return (0);
  
  while (count < sects->num)
  {
    size_t scan_pos = VSTR_SECTS_NUM(sects, count).pos;
    size_t scan_len = VSTR_SECTS_NUM(sects, count).len;

    ++count;
    
    if ((scan_pos == pos) && (scan_len == len))
      return (count);
  }
  
  return (0);
}

int vstr_sects_foreach(Vstr_base *base, Vstr_sects *sects, unsigned int flags,
                       int (*foreach_func)(const Vstr_base *, size_t, size_t,
                                           void *),
                       void *data)
{
  unsigned int count = 0;

  if (!sects->sz)
    return (0);
  
  while (count < sects->num)
  {
    size_t pos = sects->ptr[count].pos;
    size_t len = sects->ptr[count].len;

    if (pos && (len || (flags & VSTR_FLAG_SECT_FOREACH_ALLOW_NULL)))
    {
      int ret = (*foreach_func)(base, pos, len, data);

      switch (ret)
      {
        default:
          assert(FALSE);
        case VSTR_TYPE_SECT_FOREACH_DEF:
          break;
        case VSTR_TYPE_SECT_FOREACH_DEL:
          sects->ptr[count].pos = 0;
          sects->ptr[count].len = 0;
          break;
          
        case VSTR_TYPE_SECT_FOREACH_RET:
          return (count + 1);
      }
    }

    ++count;
  }

  return (sects->num);
}

