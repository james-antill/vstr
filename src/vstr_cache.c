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
  Vstr__cache_data_pos *data = NULL;

  if ((data = vstr_nx_cache_get(base, base->conf->cache_pos_cb_pos)))
    data->node = NULL;
}

static void vstr__cache_cbs(const Vstr_base *base, size_t pos, size_t len,
                            unsigned int type)
{
  unsigned int scan = 0;
  unsigned int last = 0;
  
  while (scan < VSTR__CACHE(base)->sz)
  {
    void *data = VSTR__CACHE(base)->data[scan];

    if (data)
    {
      if (type != VSTR_TYPE_CACHE_NOTHING)
      {
        void *(*cb_func)(const struct Vstr_base *, size_t, size_t,
                         unsigned int, void *);
        cb_func = base->conf->cache_cbs_ents[scan].cb_func;
        VSTR__CACHE(base)->data[scan] = (*cb_func)(base, pos, len, type, data);
      }
      
      assert((type != VSTR_TYPE_CACHE_FREE) || !VSTR__CACHE(base)->data[scan]);

      if (VSTR__CACHE(base)->data[scan])
        last = scan;
    }

    ++scan;
  }

  if (last < VSTR__CACHE_INTERNAL_POS_MAX) /* last is one less than the pos */
    ((Vstr_base *)base)->cache_internal = TRUE;
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

void vstr_nx_cache_cb_sub(const Vstr_base *base, size_t pos, size_t len)
{
  if (!base->cache_available || !VSTR__CACHE(base))
    return;

  vstr__cache_cbs(base, pos, len, VSTR_TYPE_CACHE_SUB);
}

void vstr_nx_cache_cb_free(const Vstr_base *base, unsigned int num)
{
  if (!base->cache_available || !VSTR__CACHE(base))
    return;

  if (num && (--num < VSTR__CACHE(base)->sz))
  {
    void *data = VSTR__CACHE(base)->data[num];
    
    if (data)
    {
      void *(*cb_func)(const struct Vstr_base *, size_t, size_t,
                       unsigned int, void *);
      
      cb_func = base->conf->cache_cbs_ents[num].cb_func;
      VSTR__CACHE(base)->data[num] = (*cb_func)(base, 0, 0,
                                                VSTR_TYPE_CACHE_FREE, data);
      vstr__cache_cbs(base, 0, 0, VSTR_TYPE_CACHE_NOTHING);
    }
    
    return;
  }
  
  vstr__cache_cbs(base, 0, 0, VSTR_TYPE_CACHE_FREE);
}

void vstr__cache_cstr_cpy(const Vstr_base *base, size_t pos, size_t len,
                          const Vstr_base *from_base, size_t from_pos)
{
  Vstr__cache_data_cstr *data = NULL;
  Vstr__cache_data_cstr *from_data = NULL;
  unsigned int off = base->conf->cache_pos_cb_cstr;
  unsigned int from_off = from_base->conf->cache_pos_cb_cstr;
  
  if (!base->cache_available || !VSTR__CACHE(base))
    return;

  if (!(data = vstr_nx_cache_get(base, off)))
    return;

  if (!(from_data = vstr_nx_cache_get(from_base, from_off)))
    return;

  if (data->ref || !from_data->ref)
    return;

  if ((from_pos == from_data->pos) && (len == from_data->len))
  {
    data->ref = vstr_nx_ref_add(from_data->ref);
    data->pos = pos;
    data->len = len;
  }
}

static void vstr__cache_iovec_memmove(const Vstr_base *base)
{
 Vstr__cache_data_iovec *vec = VSTR__CACHE(base)->vec;

 vstr_nx_wrap_memmove(vec->v + base->conf->iov_min_offset, vec->v + vec->off,
                      sizeof(struct iovec) * base->num);
 vstr_nx_wrap_memmove(vec->t + base->conf->iov_min_offset, vec->t + vec->off,
                      base->num);
 vec->off = base->conf->iov_min_offset;
}

int vstr__cache_iovec_alloc(const Vstr_base *base, unsigned int sz)
{
  Vstr__cache_data_iovec *vec = NULL;
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
   if (!(vec = malloc(sizeof(Vstr__cache_data_iovec))))
   {
     base->conf->malloc_bad = TRUE;
     return (FALSE);
   }
   if (!vstr_nx_cache_set(base, base->conf->cache_pos_cb_iovec, vec))
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
        vstr_nx_cache_get(base, base->conf->cache_pos_cb_iovec));
 
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
  
  vec->sz = alloc_sz;
  assert(!vec->off);
 }

 if (!base->iovec_upto_date)
 {
   vec->off = 0;
 }
 else if ((vec->off > base->conf->iov_min_offset) &&
          (sz > (vec->sz - vec->off)))
   vstr__cache_iovec_memmove(base);

 if (sz > (vec->sz - vec->off))
 {
  struct iovec *tmp_iovec = NULL;
  unsigned char *tmp_types = NULL;

  alloc_sz = sz + base->conf->iov_min_offset;
 
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
  vec->sz = alloc_sz;
  
  if (vec->off < base->conf->iov_min_offset)
    vstr__cache_iovec_memmove(base);
 }

 return (TRUE);
}

int vstr__make_cache(const Vstr_base *base)
{
 Vstr__cache *cache = malloc(sizeof(Vstr__cache));

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
         vstr_nx_cache_get(base, base->conf->cache_pos_cb_iovec));

  vstr__cache_cbs(base, 0, 0, VSTR_TYPE_CACHE_FREE);
  
  free(VSTR__CACHE(base));

  base->iovec_upto_date = FALSE;
  
  VSTR__CACHE(base) = NULL;
}

static int vstr__resize_cache(const Vstr_base *base, unsigned int sz)
{
  Vstr__cache *cache = NULL;

  assert(base->cache_available && VSTR__CACHE(base));
  assert(VSTR__CACHE(base)->sz < sz);
  
  if (!(cache = realloc(VSTR__CACHE(base),
                        sizeof(Vstr__cache) + (sizeof(void *) * sz))))
  {
    base->conf->malloc_bad = TRUE;  
    return (FALSE);
  }

  vstr_nx_wrap_memset(cache->data + cache->sz, 0,
                      sizeof(void *) * (sz - cache->sz));

  cache->sz = sz;
  VSTR__CACHE(base) = cache;

  return (TRUE);
}

static void *vstr__cache_pos_cb(const Vstr_base *base __attribute__((unused)),
                                size_t pos, size_t len __attribute__((unused)),
                                unsigned int type, void *passed_data)
{
  Vstr__cache_data_pos *data = passed_data;
  
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
  Vstr__cache_data_iovec *data = passed_data;

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
  Vstr__cache_data_cstr *data = passed_data;
  const size_t end_pos = (pos + len - 1);
  const size_t data_end_pos = (data->pos + data->len - 1);
  
  if (type == VSTR_TYPE_CACHE_FREE)
  {
    if (data)
      vstr_nx_ref_del(data->ref);
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
  
  vstr_nx_ref_del(data->ref);
  data->ref = NULL;
  
  return (data);
}

int vstr__cache_conf_init(Vstr_conf *conf)
{
  if (!(conf->cache_cbs_ents = malloc(sizeof(Vstr_cache_cb) * 3)))
    return (FALSE);
  
  conf->cache_cbs_sz = 3;

  conf->cache_pos_cb_sects = 0; /* NOTE: should really be in vstr_sects.c ...
                                 * but it's not a big problem */
  
  conf->cache_cbs_ents[0].name = "/vstr__/pos";
  conf->cache_cbs_ents[0].cb_func = vstr__cache_pos_cb;
  conf->cache_pos_cb_pos = 1;

  conf->cache_cbs_ents[1].name = "/vstr__/iovec";
  conf->cache_cbs_ents[1].cb_func = vstr__cache_iovec_cb;
  conf->cache_pos_cb_iovec = 2;

  conf->cache_cbs_ents[2].name = "/vstr__/cstr";
  conf->cache_cbs_ents[2].cb_func = vstr__cache_cstr_cb;
  conf->cache_pos_cb_cstr = 3;

  assert(VSTR__CACHE_INTERNAL_POS_MAX == 2);
  
  return (TRUE);
}

/* initial stuff done in vstr.c */
unsigned int vstr_nx_cache_add(Vstr_conf *conf, const char *name,
                               void *(*func)(const Vstr_base *,
                                             size_t, size_t,
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
  conf->cache_cbs_sz = sz;
  conf->cache_cbs_ents = ents;

  ents[sz - 1].name = name;
  ents[sz - 1].cb_func = func;
  
  return (sz);
}

unsigned int vstr_nx_cache_srch(Vstr_conf *conf, const char *name)
{
  unsigned int pos = 0;

  while (pos < conf->cache_cbs_sz)
  {
    if (!strcmp(name, conf->cache_cbs_ents[pos++].name))
      return (pos);
  }

  return (0);
}

int vstr_nx_cache_set(const Vstr_base *base, unsigned int pos, void *data)
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

  /* we've set data for a user cache */
  if ((pos > VSTR__CACHE_INTERNAL_POS_MAX) && data)
    ((Vstr_base *)base)->cache_internal = FALSE;

  VSTR__CACHE(base)->data[pos] = data;
  
  return (TRUE);
}

int vstr__cache_subset_cbs(Vstr_conf *ful_conf, Vstr_conf *sub_conf)
{
  unsigned int scan = 0;
  
  if (sub_conf->cache_cbs_sz > ful_conf->cache_cbs_sz)
    return (FALSE);

  while (scan < ful_conf->cache_cbs_sz)
  {
    if (strcmp(ful_conf->cache_cbs_ents[scan].name,
               sub_conf->cache_cbs_ents[scan].name))
      return (FALSE); /* different ... so bad */

    ++scan;
  }

  return (TRUE);
}

int vstr__cache_dup_cbs(Vstr_conf *conf, Vstr_conf *dupconf)
{
  Vstr_cache_cb *ents = conf->cache_cbs_ents;
  unsigned int scan = 0;

  if ((conf->cache_cbs_sz != dupconf->cache_cbs_sz) &&
      !(ents = realloc(ents, sizeof(Vstr_cache_cb) * conf->cache_cbs_sz)))
  {
    conf->malloc_bad = TRUE;
    return (FALSE);
  }
  conf->cache_cbs_sz = dupconf->cache_cbs_sz;
  conf->cache_cbs_ents = ents;  

  while (scan < dupconf->cache_cbs_sz)
  {
    ents[scan] = dupconf->cache_cbs_ents[scan];
    ++scan;
  }

  return (TRUE);
}
