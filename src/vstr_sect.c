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

Vstr_sects *vstr_nx_sects_make(unsigned int beg_num)
{
  Vstr_sects *sects = VSTR__MK(sizeof(Vstr_sects));
  Vstr_sect_node *ptr = NULL;
  if (!sects)
    return (NULL);

  if (beg_num && !(ptr = VSTR__MK(sizeof(Vstr_sect_node) * beg_num)))
  {
    VSTR__F(sects);
    return (NULL);
  }

  VSTR_SECTS_INIT(sects, beg_num, ptr, TRUE);

  return (sects);
}

void vstr_nx_sects_free(Vstr_sects *sects)
{
  if (!sects)
    return;

  if (sects->free_ptr)
    VSTR__F(sects->ptr);

  VSTR__F(sects);
}

static int vstr__sects_add(Vstr_sects *sects)
{
  unsigned int sz = sects->sz;
  
  if (sects->alloc_double)
    sz <<= 1;
  else
    ++sz;
  
  if (!VSTR__MV(sects->ptr, sizeof(Vstr_sect_node) * sz))
  {
    sects->malloc_bad = TRUE;
    return (FALSE);
  }
  sects->sz = sz;

  return (TRUE);
}

static int vstr__sects_del(Vstr_sects *sects)
{
  unsigned int sz = sects->sz;
  
  sz >>= 1;
  assert(sz >= sects->num);
  
  if (!VSTR__MV(sects->ptr, sizeof(Vstr_sect_node) * sz))
  {
    sects->malloc_bad = TRUE;
    return (FALSE);
  }
  sects->sz = sz;

  return (TRUE);
}

int vstr_nx_extern_inline_sects_add(Vstr_sects *sects,
                                    size_t pos __attribute__((unused)),
                                    size_t len __attribute__((unused)))
{
  /* see vstr-extern.h for why */
  assert(sizeof(struct Vstr_sects) >= sizeof(struct Vstr_sect_node));

  return (vstr__sects_add(sects));
}

int vstr_nx_sects_del(Vstr_sects *sects, unsigned int num)
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

unsigned int vstr_nx_sects_srch(Vstr_sects *sects, size_t pos, size_t len)
{
  unsigned int count = 0;

  if (!sects->sz)
    return (0);
  
  while (count++ < sects->num)
  {
    size_t scan_pos = VSTR_SECTS_NUM(sects, count)->pos;
    size_t scan_len = VSTR_SECTS_NUM(sects, count)->len;
    
    if ((scan_pos == pos) && (scan_len == len))
      return (count);
  }
  
  return (0);
}

int vstr_nx_sects_foreach(const Vstr_base *base,
                          Vstr_sects *sects, const unsigned int flags,
                          unsigned int (*foreach_func)(const Vstr_base *,
                                                       size_t, size_t, void *),
                          void *data)
{
  unsigned int count = 0;
  unsigned int scan = 0;

  if (!sects->sz)
    return (0);

  if (flags & VSTR_FLAG_SECTS_FOREACH_BACKWARD)
    scan = sects->num;
  
  while ((!(flags & VSTR_FLAG_SECTS_FOREACH_BACKWARD) &&
          (scan < sects->num)) ||
         ((flags & VSTR_FLAG_SECTS_FOREACH_BACKWARD) && scan))
  {
    size_t pos = 0;
    size_t len = 0;

    if (flags & VSTR_FLAG_SECTS_FOREACH_BACKWARD)
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

    if (!(flags & VSTR_FLAG_SECTS_FOREACH_BACKWARD))
      ++scan;
  }

 shorten_and_return:
  while (sects->num && !sects->ptr[sects->num - 1].pos)
    --sects->num;

  if (sects->can_del_sz && (sects->num < (sects->sz / 2)))
    vstr__sects_del(sects);

  return (count);
}

typedef struct Vstr__sects_cache_data
{
 unsigned int sz;
 unsigned int len;
 Vstr_sects *VSTR__STRUCT_HACK_ARRAY(updates);
} Vstr__sects_cache_data;

static void *vstr__sects_update_cb(const Vstr_base *base,size_t pos, size_t len,
                                   unsigned int type, void *passed_data)
{
  Vstr__sects_cache_data *data = passed_data;
  unsigned int count = 0;
  
  ASSERT(base->conf->cache_pos_cb_sects);
  ASSERT(data == vstr_nx_cache_get(base, base->conf->cache_pos_cb_sects));
  
  if (type == VSTR_TYPE_CACHE_FREE)
  {
    VSTR__F(data);
    return (NULL);
  }

  if (type == VSTR_TYPE_CACHE_SUB) /* do nothing for substitutions ... */
    return (data);
  
  while (count < data->len)
  {
    Vstr_sects *sects = data->updates[count];

    switch (type)
    {
      case VSTR_TYPE_CACHE_ADD:
      {
        unsigned int scan = 0;
        while (scan < sects->num)
        {
          if (sects->ptr[scan].pos && sects->ptr[scan].len)
          {
            if (pos < sects->ptr[scan].pos)
              sects->ptr[scan].pos += len;
            if ((pos >= sects->ptr[scan].pos) &&
                (pos < (sects->ptr[scan].pos + sects->ptr[scan].len - 1)))
              sects->ptr[scan].len += len;
          }
          
          ++scan;
        }
      }
      break;
      
      case VSTR_TYPE_CACHE_DEL:
      {
        unsigned int scan = 0;

        while (scan < sects->num)
        {
          if (sects->ptr[scan].pos && sects->ptr[scan].len)
          {
            if (pos <= sects->ptr[scan].pos)
            {
              size_t tmp = sects->ptr[scan].pos - pos;

              if (tmp >= len)
                sects->ptr[scan].pos -= len;
              else
              {
                len -= tmp;
                if (len >= sects->ptr[scan].len)
                  sects->ptr[scan].pos = 0;
                else
                {
                  sects->ptr[scan].pos -= tmp;
                  sects->ptr[scan].len -= len;
                }
              }
            }
            else if ((pos >= sects->ptr[scan].pos) &&
                     (pos < (sects->ptr[scan].pos + sects->ptr[scan].len - 1)))
            {
              size_t tmp = pos - sects->ptr[scan].pos;

              if (!tmp && (len >= sects->ptr[scan].len))
                sects->ptr[scan].pos = 0;
              else if (len >= (sects->ptr[scan].len - tmp))
                sects->ptr[scan].len = tmp;
              else
                sects->ptr[scan].len -= len;
            }
          }
          
          ++scan;
        }
      }
      break;
        
      default:
        ASSERT(FALSE);
    }
    
    ++count;
  }

  return (data);
}

static Vstr_sects **vstr__sects_update_srch(Vstr__sects_cache_data *data,
                                            Vstr_sects *sects)
{
  unsigned int count = 0;

  while (count < data->len)
  {
    if (data->updates[count] == sects)
      return (&data->updates[count]);
    
    ++count;
  }

  return (NULL);
}

static void vstr__sects_update_del(Vstr__sects_cache_data *data,
                                   Vstr_sects **sects)
{
  Vstr_sects **end = (data->updates + (data->len - 1));
  
  --data->len;
  
  if (sects != end)
    vstr_nx_wrap_memmove(sects, sects + 1,
                         (end - sects) * sizeof(Vstr_sects *));
}

int vstr_nx_sects_update_add(const Vstr_base *base,
                             Vstr_sects *sects)
{
  Vstr__sects_cache_data *data = NULL;
  unsigned int sz = 1;
  
  if (!base->conf->cache_pos_cb_sects)
  {
    unsigned int tmp = 0;
    
    tmp = vstr_nx_cache_add(base->conf, "/vstr__/sects/update",
                            vstr__sects_update_cb);

    if (!tmp)
      return (FALSE);
    
    base->conf->cache_pos_cb_sects = tmp;
  }

  if (!(data = vstr_nx_cache_get(base, base->conf->cache_pos_cb_sects)))
  {
    if (!vstr_nx_cache_set(base, base->conf->cache_pos_cb_sects, NULL))
      return (FALSE);

    data = VSTR__MK(sizeof(Vstr__sects_cache_data) +
                    (sz * sizeof(Vstr_sects *)));
    if (!data)
    {
      base->conf->malloc_bad = TRUE;
      return (FALSE);
    }

    data->sz = 1;
    data->len = 0;
    
    vstr_nx_cache_set(base, base->conf->cache_pos_cb_sects, data);
  }

  sz = data->len + 1;
  ASSERT(data->sz);
  ASSERT(data->len <= data->sz);
  if (data->len >= data->sz)
  {
    if (!(VSTR__MV(data, sizeof(Vstr__sects_cache_data) +
                   (sz * sizeof(Vstr_sects *)))))
    {
      base->conf->malloc_bad = TRUE;
      return (FALSE);
    }
    data->sz = data->len + 1;
    
    vstr_nx_cache_set(base, base->conf->cache_pos_cb_sects, data);
  }

  data->updates[data->len] = sects;
  ++data->len;
  
  return (TRUE);
}

int vstr_nx_sects_update_del(const Vstr_base *base,
                             Vstr_sects *sects)
{
  Vstr__sects_cache_data *data = NULL;
  Vstr_sects **srch = NULL;
  
  if (!sects || !base->conf->cache_pos_cb_sects)
    return (FALSE);
  
  data = vstr_nx_cache_get(base, base->conf->cache_pos_cb_sects);
  if (!data)
    return (FALSE);

  srch = vstr__sects_update_srch(data, sects);
  if (!srch)
  {
    ASSERT(FALSE);
    return (FALSE);
  }
  
  vstr__sects_update_del(data, srch);

  if (!data->len)
  {
    VSTR__F(data);
    vstr_nx_cache_set(base, base->conf->cache_pos_cb_sects, NULL);
  }
  
  return (TRUE);
}
