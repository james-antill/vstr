#define VSTR_REF_C
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
/* contains all the base functions related to vstr_ref objects
 * including callbacks */
#include "main.h"

void vstr_nx_ref_cb_free_nothing(Vstr_ref *ref __attribute__ ((unused)))
{
}

void vstr_nx_ref_cb_free_ref(Vstr_ref *ref)
{
 free(ref);
}

#ifndef NDEBUG
void vstr__ref_cb_free_buf_ref(Vstr_ref *ref)
{
  assert(!ref || (((Vstr__buf_ref *)ref)->buf == ref->ptr));
  vstr_nx_ref_cb_free_ref(ref);
}
#endif

void vstr_nx_ref_cb_free_ptr(Vstr_ref *ref)
{
  if (ref)
    free(ref->ptr);
}

void vstr_nx_ref_cb_free_ptr_ref(Vstr_ref *ref)
{
 vstr_nx_ref_cb_free_ptr(ref);
 free(ref);
}

Vstr_ref *vstr_nx_ref_make_ptr(void *ptr, void (*func)(struct Vstr_ref *))
{
  Vstr_ref *ref = malloc(sizeof(Vstr_ref));
  
  if (!ref)
    return (NULL);

  ref->ref = 1;
  ref->ptr = ptr;
  ref->func = func;

  return (ref);
}

Vstr_ref *vstr_nx_ref_make_malloc(size_t len)
{
  struct Vstr__buf_ref *ref = malloc(sizeof(Vstr__buf_ref) + len);
  
  if (!ref)
    return (NULL);

  ref->ref.ref = 1;
  ref->ref.ptr = ref->buf;
  ref->ref.func = vstr_nx_ref_cb_free_ref;

  assert(&ref->ref == (Vstr_ref *)ref);
  
  return (&ref->ref);
}
