#define VSTR_REF_C
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
/* contains all the base functions related to vstr_ref objects
 * including callbacks */
#include "main.h"

void vstr_ref_cb_free_nothing(Vstr_ref *ref __attribute__ ((unused)))
{
}

void vstr_ref_cb_free_ref(Vstr_ref *ref)
{
  free(ref);
}

void vstr_ref_cb_free_ptr(Vstr_ref *ref)
{
  if (ref)
    free(ref->ptr);
}

void vstr_ref_cb_free_ptr_ref(Vstr_ref *ref)
{
  vstr_ref_cb_free_ptr(ref);
  vstr_ref_cb_free_ref(ref);
}

#ifdef NDEBUG
# define vstr__ref_cb_free_ref vstr_ref_cb_free_ref
#else
static void vstr__ref_cb_free_ref(Vstr_ref *ref)
{
  VSTR__F(ref);
}
#endif

Vstr_ref *vstr_ref_make_ptr(void *ptr, void (*func)(struct Vstr_ref *))
{
  Vstr_ref *ref = NULL;

#ifndef NDEBUG
  if (func == vstr_ref_cb_free_ref)
  {
    ref = VSTR__MK(sizeof(Vstr_ref));
    func = vstr__ref_cb_free_ref;
  }
  else
#endif
    ref = malloc(sizeof(Vstr_ref));

  if (!ref)
    return (NULL);

  ref->ref = 1;
  ref->ptr = ptr;
  ref->func = func;

  return (ref);
}

Vstr_ref *vstr_ref_make_malloc(size_t len)
{
  struct Vstr__buf_ref *ref = VSTR__MK(sizeof(Vstr__buf_ref) + len);

  if (!ref)
    return (NULL);

  ref->ref.ref = 1;
  ref->ref.ptr = ref->buf;
  ref->ref.func = vstr__ref_cb_free_ref;

  assert(&ref->ref == (Vstr_ref *)ref);

  return (&ref->ref);
}

Vstr_ref *vstr_ref_make_memdup(void *ptr, size_t len)
{
  Vstr_ref *ref = vstr_ref_make_malloc(len);
  vstr_wrap_memcpy(ref->ptr, ptr, len);

  return (ref);
}

static void vstr__ref_cb_free_vstr_base(Vstr_ref *ref)
{
  vstr_free_base(ref->ptr);
  free(ref);
}

Vstr_ref *vstr_ref_make_vstr_base(Vstr_base *base)
{
  return (vstr_ref_make_ptr(base, vstr__ref_cb_free_vstr_base));
}

static void vstr__ref_cb_free_vstr_conf(Vstr_ref *ref)
{
  vstr_free_conf(ref->ptr);
  free(ref);
}

Vstr_ref *vstr_ref_make_vstr_conf(Vstr_conf *conf)
{
  return (vstr_ref_make_ptr(conf, vstr__ref_cb_free_vstr_conf));
}

static void vstr__ref_cb_free_vstr_sects(Vstr_ref *ref)
{
  vstr_sects_free(ref->ptr);
  free(ref);
}

Vstr_ref *vstr_ref_make_vstr_sects(Vstr_sects *sects)
{
  return (vstr_ref_make_ptr(sects, vstr__ref_cb_free_vstr_sects));
}
