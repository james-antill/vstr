#define VSTR_CSTR_C
/*
 *  Copyright (C) 1999, 2000, 2001, 2002, 2003  James Antill
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
/* Functions to allow export of a "C string" like interface */
#include "main.h"

static Vstr_ref *vstr__export_cstr_ref(const Vstr_base *base,
                                       size_t pos, size_t len)
{
  Vstr_ref *ref = NULL;
  
  if (!(ref = vstr_ref_make_malloc(len + 1)))
  {
    base->conf->malloc_bad = TRUE;
    return (NULL);
  }

  assert(((Vstr__buf_ref *)ref)->buf == ref->ptr);
  
  vstr_export_cstr_buf(base, pos, len, ref->ptr, len + 1);

  return (ref);
}

static Vstr__cache_data_cstr *vstr__export_cstr_cache(const Vstr_base *base,
                                                      size_t pos, size_t len,
                                                      size_t *ret_off)
{
  Vstr_ref *ref = NULL;
  Vstr__cache_data_cstr *data = NULL;
  unsigned int off = base->conf->cache_pos_cb_cstr;
  
  assert(base && pos && ((pos + len - 1) <= base->len) && ret_off);
  assert(len || (!base->len && (pos == 1)));
  
  if (!(data = vstr_cache_get(base, off)))
  {
    if (!vstr_cache_set(base, off, NULL))
      return (NULL);
    
    if (!(data = VSTR__MK(sizeof(Vstr__cache_data_cstr))))
    {
      base->conf->malloc_bad = TRUE;
      return (NULL);
    }
    data->ref = NULL;
    
    vstr_cache_set(base, off, data);
  }
 
  if (data->ref)
  {
    assert(((Vstr__buf_ref *)data->ref)->buf == data->ref->ptr);

    if (pos >= data->pos)
    {
      size_t tmp = (pos - data->pos);
      if ((data->len - tmp) == len)
      {
        *ret_off = tmp;
        return (data);
      }
    }
    
    vstr_ref_del(data->ref);
    data->ref = NULL;
  }

  if (!(ref = vstr__export_cstr_ref(base, pos, len)))
    return (NULL);
  
  data->ref = ref;
  data->pos = pos;
  data->len = len;
  
  *ret_off = 0;
  return (data);
}

char *vstr_export_cstr_ptr(const Vstr_base *base, size_t pos, size_t len)
{
  Vstr__cache_data_cstr *data = NULL;
  size_t off = 0;
  
  if (!(data = vstr__export_cstr_cache(base, pos, len, &off)))
    return (NULL);
  
  return (((char *)data->ref->ptr) + off);
}

char *vstr_export_cstr_malloc(const Vstr_base *base, size_t pos, size_t len)
{
  void *ptr = malloc(len + 1);

  if (!ptr)
  {
    base->conf->malloc_bad = TRUE;
    return (NULL);
  }
  
  vstr_export_cstr_buf(base, pos, len, ptr, len + 1);

  return (ptr);
}

Vstr_ref *vstr_export_cstr_ref(const Vstr_base *base, size_t pos, size_t len,
                               size_t *ret_off)
{
  Vstr__cache_data_cstr *data = NULL;
  
  if (!base->cache_available)
  {
    Vstr_ref *ref = vstr__export_cstr_ref(base, pos, len);
    
    if (!ref)
      return (NULL);

    *ret_off = 0;
    return (ref);
  }
  
  if (!(data = vstr__export_cstr_cache(base, pos, len, ret_off)))
    return (NULL);
  
  return (vstr_ref_add(data->ref));
}

size_t vstr_export_cstr_buf(const Vstr_base *base, size_t pos, size_t len,
                            void *buf, size_t buf_len)
{
  size_t cpy_len = len;

  if (!buf_len)
    return (0);
  
  if (cpy_len >= buf_len)
    cpy_len = (buf_len - 1);

  vstr_export_buf(base, pos, len, buf, cpy_len);
  ((char *)buf)[cpy_len] = 0;

  return (cpy_len + 1);
}
