#define VSTR_SC_C
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
/* functions which are shortcuts */
#include "main.h"

int vstr_nx_sc_fmt_cb_beg(Vstr_base *base, size_t *pos,
                          Vstr_fmt_spec *spec, size_t *obj_len)
{
  size_t space_len = 0;
  
  if (spec->fmt_precision && spec->fmt_field_width &&
      (spec->obj_field_width > spec->obj_precision))
    spec->obj_field_width = spec->obj_precision;
  
  if (spec->fmt_precision && (*obj_len > spec->obj_precision))
    *obj_len = spec->obj_precision;
  
  if (spec->fmt_field_width && (*obj_len < spec->obj_field_width))
  {
    spec->obj_field_width -= *obj_len;
    space_len = spec->obj_field_width;
  }
  else
    spec->fmt_field_width = FALSE;

  if (!spec->fmt_minus)
  {
    if (!vstr_nx_add_rep_chr(base, *pos, ' ', space_len))
      return (FALSE);
    spec->obj_field_width = 0;
    *pos += space_len;
  }

  return (TRUE);
}

int vstr_nx_sc_fmt_cb_end(Vstr_base *base, size_t pos,
                          Vstr_fmt_spec *spec, size_t obj_len)
{
  size_t space_len = 0;
  
  if (spec->fmt_field_width)
    space_len = spec->obj_field_width;
  
  if (spec->fmt_minus)
    if (!vstr_nx_add_rep_chr(base, pos + obj_len, ' ', space_len))
      return (FALSE);

  return (TRUE);
}

static int vstr__sc_fmt_add_cb_vstr(Vstr_base *base, size_t pos,
                                    Vstr_fmt_spec *spec)
{
  Vstr_base *sf          = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_pos          = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);
  size_t sf_len          = VSTR_FMT_CB_ARG_VAL(spec, size_t, 2);
  unsigned int sf_flags  = VSTR_FMT_CB_ARG_VAL(spec, unsigned int, 3);

  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len))
    return (FALSE);
  
  if (!vstr_nx_add_vstr(base, pos, sf, sf_pos, sf_len, sf_flags))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

static int vstr__sc_fmt_add_cb_buf(Vstr_base *base, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  const char *buf = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_len   = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);

  if (!buf)
  {
    buf = "(null)";
    if (sf_len > strlen("(null)"))
      sf_len = strlen("(null)");
  }
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len))
    return (FALSE);
  
  if (!vstr_nx_add_buf(base, pos, buf, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

static int vstr__sc_fmt_add_cb_ptr(Vstr_base *base, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  const char *ptr = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_len   = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);

  if (!ptr)
  {
    ptr = "(null)";
    if (sf_len > strlen("(null)"))
      sf_len = strlen("(null)");
  }
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len))
    return (FALSE);
  
  if (!vstr_nx_add_ptr(base, pos, ptr, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

static int vstr__sc_fmt_add_cb_non(Vstr_base *base, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  size_t sf_len   = VSTR_FMT_CB_ARG_VAL(spec, size_t, 0);

  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len))
    return (FALSE);
  
  if (!vstr_nx_add_non(base, pos, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

static int vstr__sc_fmt_add_cb_ref(Vstr_base *base, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  Vstr_ref *ref = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_off = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);
  size_t sf_len = VSTR_FMT_CB_ARG_VAL(spec, size_t, 2);

  assert(ref);
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len))
    return (FALSE);
  
  if (!vstr_nx_add_ref(base, pos, ref, sf_off, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

int vstr_nx_sc_fmt_add_vstr(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_vstr,
                          VSTR_TYPE_FMT_PTR_VOID,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_UINT,
                          VSTR_TYPE_FMT_END));
}

int vstr_nx_sc_fmt_add_buf(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_buf,
                          VSTR_TYPE_FMT_PTR_CHAR,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
}

int vstr_nx_sc_fmt_add_ptr(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_ptr,
                          VSTR_TYPE_FMT_PTR_CHAR,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
}

int vstr_nx_sc_fmt_add_non(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_non,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
}

int vstr_nx_sc_fmt_add_ref(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_ref,
                          VSTR_TYPE_FMT_PTR_VOID,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
}
