#define VSTR_CACHE_C
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
/* Functions to allow things to be cached for the string */
#include "main.h"

void vstr__cache_free_cstr(Vstr_cache *cache)
{
 assert(((Vstr__cstr_ref *)cache->cstr.ref)->buf == cache->cstr.ref->ptr);
 
 vstr_ref_del_ref(cache->cstr.ref);
 
 cache->cstr.ref = NULL;
}

void vstr__cache_free_pos(Vstr_cache *cache)
{
 cache->pos.node = NULL;
}

void vstr__cache_del(Vstr_base *base, size_t pos, size_t len)
{
 if (!base->cache)
   return;

 if (base->cache->cstr.ref)
   vstr__cache_free_cstr(base->cache);

 if (base->cache->cstr.ref)
 {
  struct Vstr_cache_cstr *cstr = &base->cache->cstr;

  if ((cstr->pos + cstr->len - 1) < pos)
  { /* do nothing */ }
  else if (cstr->pos > (pos + len - 1))
    cstr->pos -= len;
  else
    vstr__cache_free_cstr(base->cache);
 }
 
 if (base->cache->pos.node && (base->cache->pos.pos >= pos))
   vstr__cache_free_pos(base->cache);
}

void vstr__cache_add(Vstr_base *base, size_t pos, size_t len)
{
 if (!base->cache)
   return;

 if (base->cache->cstr.ref)
 {
  struct Vstr_cache_cstr *cstr = &base->cache->cstr;

  if ((cstr->pos + cstr->len - 1) <= pos)
  { /* do nothing */ }
  else if (cstr->pos > pos)
    cstr->pos += len;
  else
    vstr__cache_free_cstr(base->cache);
 }
 
 if (base->cache->pos.node && (base->cache->pos.pos > pos))
   vstr__cache_free_pos(base->cache);
}

void vstr__cache_cpy(Vstr_base *base, size_t pos, size_t len,
                     Vstr_base *from_base, size_t from_pos)
{
 if (!base->cache)
   return;
 
 if (base->cache->cstr.ref)
   return;

 if (!from_base->cache)
   return;
 
 if (!from_base->cache->cstr.ref)
   return;

 if ((from_pos == from_base->cache->cstr.pos) &&
     (len == from_base->cache->cstr.len))
 {
  base->cache->cstr.ref = vstr_ref_add_ref(from_base->cache->cstr.ref);
  base->cache->cstr.pos = pos;
  base->cache->cstr.len = len;
 }
}

void vstr__cache_chg(Vstr_base *base,
                     size_t pos __attribute__ ((unused)),
                     size_t len __attribute__ ((unused)))
{
 if (!base->cache)
   return;
 
 if (base->cache->cstr.ref)
   vstr__cache_free_cstr(base->cache);
}

int vstr__make_cache(Vstr_base *base)
{
 Vstr_cache *cache = malloc(sizeof(Vstr_cache));
 if (!cache)
   return (FALSE);

 cache->cstr.ref = NULL;
 cache->pos.node = NULL;
 
 base->cache = cache;

 return (TRUE);
}

