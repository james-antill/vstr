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
  if (!cache->cstr.ref)
    return;
  
  assert(((Vstr__cstr_ref *)cache->cstr.ref)->buf == cache->cstr.ref->ptr);
  
  vstr_ref_del(cache->cstr.ref);
  
  cache->cstr.ref = NULL;
}

void vstr__cache_free_pos(Vstr_cache *cache)
{
  cache->pos.node = NULL;
}

void vstr__cache_del(Vstr_base *base, size_t pos, size_t len)
{
 if (!base->cache_available || !VSTR__CACHE(base))
   return;

 vstr__cache_free_cstr(VSTR__CACHE(base));

 if (VSTR__CACHE(base)->cstr.ref)
 {
  struct Vstr_cache_cstr *cstr = &VSTR__CACHE(base)->cstr;

  if ((cstr->pos + cstr->len - 1) < pos)
  { /* do nothing */ }
  else if (cstr->pos > (pos + len - 1))
    cstr->pos -= len;
  else
    vstr__cache_free_cstr(VSTR__CACHE(base));
 }
 
 if (VSTR__CACHE(base)->pos.node && (VSTR__CACHE(base)->pos.pos >= pos))
   vstr__cache_free_pos(VSTR__CACHE(base));
}

void vstr__cache_add(Vstr_base *base, size_t pos, size_t len)
{
 if (!base->cache_available || !VSTR__CACHE(base))
   return;

 if (VSTR__CACHE(base)->cstr.ref)
 {
  struct Vstr_cache_cstr *cstr = &VSTR__CACHE(base)->cstr;

  if ((cstr->pos + cstr->len - 1) <= pos)
  { /* do nothing */ }
  else if (cstr->pos > pos)
    cstr->pos += len;
  else
    vstr__cache_free_cstr(VSTR__CACHE(base));
 }
 
 if (VSTR__CACHE(base)->pos.node && (VSTR__CACHE(base)->pos.pos > pos))
   vstr__cache_free_pos(VSTR__CACHE(base));
}

void vstr__cache_cpy(Vstr_base *base, size_t pos, size_t len,
                     Vstr_base *from_base, size_t from_pos)
{
 if (!base->cache_available || !VSTR__CACHE(base))
   return;
 
 if (VSTR__CACHE(base)->cstr.ref)
   return;

 if (!VSTR__CACHE(from_base))
   return;
 
 if (!VSTR__CACHE(from_base)->cstr.ref)
   return;

 if ((from_pos == VSTR__CACHE(from_base)->cstr.pos) &&
     (len == VSTR__CACHE(from_base)->cstr.len))
 {
   Vstr_ref *ref = VSTR__CACHE(from_base)->cstr.ref;
   
   VSTR__CACHE(base)->cstr.ref = vstr_ref_add(ref);
   VSTR__CACHE(base)->cstr.pos = pos;
   VSTR__CACHE(base)->cstr.len = len;
 }
}

static void vstr__cache_iovec_memmove(Vstr_base *base)
{
 Vstr_iovec *vec = VSTR__CACHE(base)->vec;

 memmove(vec->v + base->conf->iov_min_offset, vec->v + vec->off,
         sizeof(struct iovec) * base->num);
 memmove(vec->t + base->conf->iov_min_offset, vec->t + vec->off,
         base->num);
 vec->off = base->conf->iov_min_offset;
}

int vstr__cache_iovec_alloc(Vstr_base *base, unsigned int sz)
{
  Vstr_iovec *vec = NULL;
  size_t alloc_sz = sz + base->conf->iov_min_alloc + base->conf->iov_min_offset;
 
  if (!sz)
    return (TRUE);

 if (!base->cache_available)
   return (FALSE);
 
 if (!VSTR__CACHE(base))
 {
   if (!vstr__make_cache(base))
     return (FALSE);
 }
 
 vec = VSTR__CACHE(base)->vec;
 if (!vec)
 {
   if (!(vec = malloc(sizeof(Vstr_iovec))))
   {
     base->conf->malloc_bad = TRUE;
     return (FALSE);
   }
   
   VSTR__CACHE(base)->vec = vec;
   
   vec->v = NULL;
   vec->t = NULL;
   vec->off = 0;
   vec->sz = 0;
 }
 
 if (!vec->v)
 {
  vec->v = malloc(sizeof(struct iovec) * alloc_sz);

  if (!vec->v)
  {
    base->conf->malloc_bad = TRUE;
    return (FALSE);
  }

  assert(!vec->t);
  vec->t = malloc(alloc_sz);

  if (!vec->t)
  {
    free(vec->v);
    vec->v = NULL;
    base->conf->malloc_bad = TRUE;
    return (FALSE);
  }
  
  vec->sz = sz;
  assert(!vec->off);
 }

 if ((vec->off > base->conf->iov_min_offset) &&
     (sz > (vec->sz - vec->off)))
   vstr__cache_iovec_memmove(base);

 if (sz > (vec->sz - vec->off))
 {
  struct iovec *tmp_iovec = NULL;
  unsigned char *tmp_types = NULL;
 
  tmp_iovec = realloc(vec->v, sizeof(struct iovec) * alloc_sz);

  if (!tmp_iovec)
  {
   base->conf->malloc_bad = TRUE;
   return (FALSE);
  }
  
  vec->v = tmp_iovec;
  
  tmp_types = realloc(vec->t, alloc_sz);

  if (!tmp_types)
  {
   base->conf->malloc_bad = TRUE;
   return (FALSE);
  }
  
  vec->t = tmp_types;
  
  if (vec->off < base->conf->iov_min_offset)
    vstr__cache_iovec_memmove(base);
  
  vec->sz = sz;
 }

 return (TRUE);
}

void vstr__cache_iovec_free(Vstr_base *base)
{
  assert(VSTR__CACHE(base));
  if (!VSTR__CACHE(base)->vec)
    return;

  free(VSTR__CACHE(base)->vec->v);
  free(VSTR__CACHE(base)->vec->t);
  
  VSTR__CACHE(base)->vec = NULL;
}

void vstr__cache_chg(Vstr_base *base,
                     size_t pos __attribute__ ((unused)),
                     size_t len __attribute__ ((unused)))
{
  if (!base->cache_available || !VSTR__CACHE(base))
    return;
  
  vstr__cache_free_cstr(VSTR__CACHE(base));
}

int vstr__make_cache(Vstr_base *base)
{
 Vstr_cache *cache = malloc(sizeof(Vstr_cache));

 if (!cache)
 {
   base->conf->malloc_bad = TRUE;  
   return (FALSE);
 }
 
 cache->cstr.ref = NULL;
 cache->pos.node = NULL;
 cache->vec = NULL;
 
 VSTR__CACHE(base) = cache;

 return (TRUE);
}

void vstr__free_cache(Vstr_base *base)
{
  if (!base->cache_available || !VSTR__CACHE(base))
    return;
  
  vstr__cache_free_cstr(VSTR__CACHE(base));
  vstr__cache_free_pos(VSTR__CACHE(base));

  vstr__cache_iovec_free(base);
  
  free(VSTR__CACHE(base));
  VSTR__CACHE(base) = NULL;
}
