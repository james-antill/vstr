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

static int vstr__export_cstr(const Vstr_base *base, size_t pos, size_t len)
{
 struct Vstr__cstr_ref *ref = NULL;
 
 if (!base->cache)
 {
  if (!vstr__make_cache((Vstr_base *)base))
    return (FALSE);
 }
 
 if (base->cache->cstr.ref)
 {
  if ((base->cache->cstr.pos == pos) && (base->cache->cstr.len == len))
    return (TRUE);
  vstr__cache_free_cstr(base->cache);
 }

 if (!(ref = malloc(sizeof(Vstr__cstr_ref) + len + 1)))
   return (FALSE);

 ref->ref.func = vstr_ref_cb_free_ref;
 ref->ref.ptr = ref->buf;
 ref->ref.ref = 1;
 
 vstr_export_buf(base, pos, len, ref->buf);
 ref->buf[len] = 0;

 base->cache->cstr.ref = &ref->ref;
 base->cache->cstr.pos = pos;
 base->cache->cstr.len = len;

 return (TRUE);
}

char *vstr_export_cstr_ptr(const Vstr_base *base, size_t pos, size_t len)
{
 if (!vstr__export_cstr(base, pos, len))
   return (NULL);

 return (base->cache->cstr.ref->ptr);
}

Vstr_ref *vstr_export_cstr_ref(const Vstr_base *base, size_t pos, size_t len)
{
 if (!vstr__export_cstr(base, pos, len))
   return (NULL);

 return (vstr_ref_add_ref(base->cache->cstr.ref));
}
