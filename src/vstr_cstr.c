#define VSTR_CSTR_C
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
/* Functions to allow export of a "C string" like interface */
#include "main.h"

static Vstr__cstr_ref *vstr__export_cstr_ref(const Vstr_base *base,
                                             size_t pos, size_t len)
{
  struct Vstr__cstr_ref *ref = NULL;
  
  if (!(ref = malloc(sizeof(Vstr__cstr_ref) + len + 1)))
    return (NULL);
  
  ref->ref.func = vstr_ref_cb_free_ref;
  ref->ref.ptr = ref->buf;
  ref->ref.ref = 1;
  
  if (len)
    vstr_export_buf(base, pos, len, ref->buf);
  ref->buf[len] = 0;

  return (ref);
}

static int vstr__export_cstr(const Vstr_base *base, size_t pos, size_t len)
{
 struct Vstr__cstr_ref *ref = NULL;

 assert(base && pos && ((pos + len - 1) <= base->len));
 assert(len || (!base->len && (pos == 1)));

 if (!base->cache_available)
   return (FALSE);
 
 if (!VSTR__CACHE(base))
 {
  if (!vstr__make_cache((Vstr_base *)base))
    return (FALSE);
 }
 
 if (VSTR__CACHE(base)->cstr.ref)
 {
  if ((VSTR__CACHE(base)->cstr.pos == pos) &&
      (VSTR__CACHE(base)->cstr.len == len))
    return (TRUE);
  vstr__cache_free_cstr(VSTR__CACHE(base));
 }

 if (!(ref = vstr__export_cstr_ref(base, pos, len)))
   return (FALSE);

 VSTR__CACHE(base)->cstr.ref = &ref->ref;
 VSTR__CACHE(base)->cstr.pos = pos;
 VSTR__CACHE(base)->cstr.len = len;

 return (TRUE);
}

char *vstr_export_cstr_ptr(const Vstr_base *base, size_t pos, size_t len)
{
 if (!vstr__export_cstr(base, pos, len))
   return (NULL);

 return (VSTR__CACHE(base)->cstr.ref->ptr);
}

Vstr_ref *vstr_export_cstr_ref(const Vstr_base *base, size_t pos, size_t len)
{
  if (!base->cache_available)
  {
    struct Vstr__cstr_ref *ref = vstr__export_cstr_ref(base, pos, len);
    
    if (!ref)
      return (NULL);
    
    return (&ref->ref);
  }
  
  if (!vstr__export_cstr(base, pos, len))
    return (NULL);
  
  return (vstr_ref_add(VSTR__CACHE(base)->cstr.ref));
}

void vstr_export_cstr_buf(const Vstr_base *base, size_t pos, size_t len,
                          void *buf, size_t buf_len)
{
  size_t cpy_len = len;

  if (!buf_len)
    return;
  
  if (cpy_len >= buf_len)
    cpy_len = (buf_len - 1);

  vstr_export_buf(base, pos, cpy_len, buf);
  ((char *)buf)[cpy_len] = 0;
}
