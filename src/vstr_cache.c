#define VSTR_CACHE_C
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
/* Functions to allow things to be cached for the string */
#include "main.h"
/* NOTE: some assert stuff is in vstr.c also vstr_add.c and vstr_del.c
 * know a bit about the internals of this file as well */

void vstr__cache_free_pos(const Vstr_base *base)
{
  Vstr_cache_data_pos *data = NULL;

  if ((data = vstr_cache_get_data(base, base->conf->cache_pos_cb_pos)))
    data->node = NULL;
}

static void vstr__cache_cbs(const Vstr_base *base, size_t pos, size_t len,
                            unsigned int type)
{
  unsigned int scan = 0;

  while (scan < VSTR__CACHE(base)->sz)
  {
    void *(*cb_func)(const struct Vstr_base *, size_t, size_t,
                     unsigned int, void *);
    void *data = VSTR__CACHE(base)->data[scan];

    if (data)
    {
      cb_func = base->conf->cache_cbs_ents[scan].cb_func;
      VSTR__CACHE(base)->data[scan] = (*cb_func)(base, pos, len, type, data);
      assert((type != VSTR_TYPE_CACHE_FREE) || !VSTR__CACHE(base)->data[scan]);
    }

    ++scan;
  }
}

void vstr__cache_del(const Vstr_base *base, size_t pos, size_t len)
{
  if (!base->cache_available || !VSTR__CACHE(base))
    return;
  
  vstr__cache_cbs(base, pos, len, VSTR_TYPE_CACHE_DEL); 
}

void vstr__cache_add(const Vstr_base *base, size_t pos, size_t len)
{
 if (!base->cache_available || !VSTR__CACHE(base))
   return;

 vstr__cache_cbs(base, pos, len, VSTR_TYPE_CACHE_ADD);
}

void vstr__cache_sub(const Vstr_base *base,
                     size_t pos __attribute__ ((unused)),
                     size_t len __attribute__ ((unused)))
{
  if (!base->cache_available || !VSTR__CACHE(base))
    return;

  vstr__cache_cbs(base, pos, len, VSTR_TYPE_CACHE_SUB);
}

void vstr__cache_cstr_cpy(const Vstr_base *base, size_t pos, size_t len,
                          const Vstr_base *from_base, size_t from_pos)
{
  Vstr_cache_data_cstr *data = NULL;
  Vstr_cache_data_cstr *from_data = NULL;
  unsigned int off = base->conf->cache_pos_cb_cstr;
  unsigned int from_off = from_base->conf->cache_pos_cb_cstr;
  
  if (!base->cache_available || !VSTR__CACHE(base))
    return;

  if (!(data = vstr_cache_get_data(base, off)))
    return;

  if (!(from_data = vstr_cache_get_data(from_base, from_off)))
    return;

  if (data->ref || !from_data->ref)
    return;

  if ((from_pos == from_data->pos) && (len == from_data->len))
  {
    data->ref = vstr_ref_add(from_data->ref);
    data->pos = pos;
    data->len = len;
  }
}

static void vstr__cache_iovec_memmove(const Vstr_base *base)
{
 Vstr_cache_data_iovec *vec = VSTR__CACHE(base)->vec;

 memmove(vec->v + base->conf->iov_min_offset, vec->v + vec->off,
         sizeof(struct iovec) * base->num);
 memmove(vec->t + base->conf->iov_min_offset, vec->t + vec->off,
         base->num);
 vec->off = base->conf->iov_min_offset;
}

int vstr__cache_iovec_alloc(const Vstr_base *base, unsigned int sz)
{
  Vstr_cache_data_iovec *vec = NULL;
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
   if (!(vec = malloc(sizeof(Vstr_cache_data_iovec))))
   {
     base->conf->malloc_bad = TRUE;
     return (FALSE);
   }
   if (!vstr_cache_set_data(base, base->conf->cache_pos_cb_iovec, vec))
   {
     free(vec);
     base->conf->malloc_bad = TRUE;
     return (FALSE);
   }
   
   VSTR__CACHE(base)->vec = vec;
   
   vec->v = NULL;
   vec->t = NULL;
   vec->off = 0;
   vec->sz = 0;
 }
 assert(VSTR__CACHE(base)->vec ==
        vstr_cache_get_data(base, base->conf->cache_pos_cb_iovec));
 
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

 if (!base->iovec_upto_date)
   vec->off = base->conf->iov_min_offset;
 else if ((vec->off > base->conf->iov_min_offset) &&
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

int vstr__make_cache(const Vstr_base *base)
{
 Vstr_cache *cache = malloc(sizeof(Vstr_cache));

 if (!cache)
 {
   base->conf->malloc_bad = TRUE;  
   return (FALSE);
 }
 
 cache->vec = NULL;

 cache->sz = 0;
 
 VSTR__CACHE(base) = cache;

 return (TRUE);
}

void vstr__free_cache(const Vstr_base *base)
{
  if (!base->cache_available || !VSTR__CACHE(base))
    return;

  assert(VSTR__CACHE(base)->vec ==
         vstr_cache_get_data(base, base->conf->cache_pos_cb_iovec));

  vstr__cache_cbs(base, 0, 0, VSTR_TYPE_CACHE_FREE);
  
  free(VSTR__CACHE(base));
  
  VSTR__CACHE(base) = NULL;
}

static int vstr__resize_cache(const Vstr_base *base, unsigned int sz)
{
  Vstr_cache *cache = NULL;

  assert(base->cache_available && VSTR__CACHE(base));
  assert(VSTR__CACHE(base)->sz < sz);
  
  if (!(cache = realloc(VSTR__CACHE(base),
                        sizeof(Vstr_cache) + (sizeof(void *) * sz))))
  {
    base->conf->malloc_bad = TRUE;  
    return (FALSE);
  }

  memset(cache->data + cache->sz, 0, sizeof(void *) * (sz - cache->sz));

  cache->sz = sz;
  VSTR__CACHE(base) = cache;

  return (TRUE);
}

static void *vstr__cache_pos_cb(const Vstr_base *base __attribute__((unused)),
                                size_t pos, size_t len __attribute__((unused)),
                                unsigned int type, void *passed_data)
{
  Vstr_cache_data_pos *data = passed_data;
  
  if (type == VSTR_TYPE_CACHE_FREE)
  {
    free(data);
    return (NULL);
  }

  if (!data->node)
    return (data);

  if ((type == VSTR_TYPE_CACHE_ADD) && (pos >= data->pos))
    return (data);

  if ((type == VSTR_TYPE_CACHE_DEL) && (pos > data->pos))
    return (data);
  
  if (type == VSTR_TYPE_CACHE_SUB)
    return (data);
  
  /* can't update data->num properly */
  data->node = NULL;
  return (data);
}

static void *vstr__cache_iovec_cb(const Vstr_base *base
                                  __attribute__((unused)),
                                  size_t pos __attribute__((unused)),
                                  size_t len __attribute__((unused)),
                                  unsigned int type, void *passed_data)
{
  Vstr_cache_data_iovec *data = passed_data;

  assert(VSTR__CACHE(base)->vec == data);
  
  if (type == VSTR_TYPE_CACHE_FREE)
  {
    free(data->v);
    free(data->t);
    free(data);

    return (NULL);
  }

  /* **** handled by hand in the code... :O */
  
  return (data);
}

static void *vstr__cache_cstr_cb(const Vstr_base *base __attribute__((unused)),
                                 size_t pos, size_t len,
                                 unsigned int type, void *passed_data)
{
  Vstr_cache_data_cstr *data = passed_data;
  const size_t end_pos = (pos + len - 1);
  const size_t data_end_pos = (data->pos + data->len - 1);
  
  if (type == VSTR_TYPE_CACHE_FREE)
  {
    free(data);
    return (NULL);
  }

  if (!data->ref)
    return (data);
  
  if (data_end_pos < pos)
    return (data);

  if (type == VSTR_TYPE_CACHE_ADD)
  {
    if (data_end_pos == pos)
      return (data);

    if (data->pos > pos)
    {
      data->pos += len;
      return (data);
    }
  }
  else if (data->pos > end_pos)
  {
    if (type == VSTR_TYPE_CACHE_DEL)
      data->pos -= len;
    
    /* do nothing for (type == VSTR_TYPE_CACHE_SUB) */
    
    return (data);
  }
  
  vstr_ref_del(data->ref);
  data->ref = NULL;
  
  return (data);
}

int vstr__cache_conf_init(Vstr_conf *conf)
{
  if (!(conf->cache_cbs_ents = malloc(sizeof(Vstr_cache_cb) * 3)))
    return (FALSE);
  
  conf->cache_cbs_sz = 3;
  
  conf->cache_cbs_ents[0].name = "/vstr__/pos";
  conf->cache_cbs_ents[0].cb_func = vstr__cache_pos_cb;
  conf->cache_pos_cb_pos = 1;

  conf->cache_cbs_ents[1].name = "/vstr__/iovec";
  conf->cache_cbs_ents[1].cb_func = vstr__cache_iovec_cb;
  conf->cache_pos_cb_iovec = 2;

  conf->cache_cbs_ents[2].name = "/vstr__/cstr";
  conf->cache_cbs_ents[2].cb_func = vstr__cache_cstr_cb;
  conf->cache_pos_cb_cstr = 3;
  
  return (TRUE);
}

/* initial stuff done in vstr.c */
unsigned int vstr_cache_add_cb(Vstr_conf *conf, const char *name,
                               void *(*func)(const Vstr_base *, size_t, size_t,
                                             unsigned int, void *))
{
  unsigned int sz = conf->cache_cbs_sz + 1;
  Vstr_cache_cb *ents = realloc(conf->cache_cbs_ents,
                                sizeof(Vstr_cache_cb) * sz);

  if (!ents)
  {
    conf->malloc_bad = TRUE;
    return (0);
  }

  ents[sz - 1].name = name;
  ents[sz - 1].cb_func = func;
  
  conf->cache_cbs_sz = sz;
  conf->cache_cbs_ents = ents;

  return (sz);
}

unsigned int vstr_cache_srch(Vstr_conf *conf, const char *name)
{
  unsigned int pos = 0;

  while (pos < conf->cache_cbs_sz)
  {
    if (!strcmp(name, conf->cache_cbs_ents[pos++].name))
      return (pos);
  }

  return (0);
}

void *vstr_cache_get_data(const Vstr_base *base, unsigned int pos)
{
  assert(pos);
  
  if (!pos)
    return (NULL);

  if (!base->cache_available || !VSTR__CACHE(base))
    return (NULL);
  
  --pos;
  
  if (pos >= VSTR__CACHE(base)->sz)
    return (NULL);
  
  return (VSTR__CACHE(base)->data[pos]);
}

int vstr_cache_set_data(const Vstr_base *base, unsigned int pos, void *data)
{
  assert(pos);
  
  if (!pos)
    return (FALSE);

  if (!base->cache_available)
    return (FALSE);

  if (!VSTR__CACHE(base))
  {
    if (!vstr__make_cache(base))
      return (FALSE);
  }
  
  --pos;
  if (pos >= VSTR__CACHE(base)->sz)
  {
    if (!vstr__resize_cache(base, pos + 1))
      return (FALSE);
  }

  VSTR__CACHE(base)->data[pos] = data;
  
  return (TRUE);
}
