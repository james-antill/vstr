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
                          Vstr_fmt_spec *spec, size_t *obj_len,
                          unsigned int flags)
{
  size_t space_len = 0;
  
  if (!(flags & VSTR_FLAG_SC_FMT_CB_BEG_OBJ_NUM))
  {
    spec->fmt_zero = FALSE;
    
    if (spec->fmt_precision && spec->fmt_field_width &&
        (spec->obj_field_width > spec->obj_precision))
      spec->obj_field_width = spec->obj_precision;
  
    if (spec->fmt_precision && (*obj_len > spec->obj_precision))
      *obj_len = spec->obj_precision;
  }
  
  if (spec->fmt_field_width && (*obj_len < spec->obj_field_width))
  {
    spec->obj_field_width -= *obj_len;
    space_len = spec->obj_field_width;
  }
  else
    spec->fmt_field_width = FALSE;
  
  if (!spec->fmt_minus && !spec->fmt_zero && space_len)
  {
    if (!vstr_nx_add_rep_chr(base, *pos, ' ', space_len))
      return (FALSE);
    *pos += space_len;
    space_len = 0;
  }

  if (flags & VSTR_FLAG_SC_FMT_CB_BEG_OBJ_NEG)
  {
    if (!vstr_nx_add_rep_chr(base, *pos, '-', 1))
      return (FALSE);
    ++*pos;
  }
  else if ((flags & VSTR_FLAG_SC_FMT_CB_BEG_OBJ_NUM) && spec->fmt_plus)
  {
    if (!vstr_nx_add_rep_chr(base, *pos, '+', 1))
      return (FALSE);
    ++*pos;
  }
  else if ((flags & VSTR_FLAG_SC_FMT_CB_BEG_OBJ_NUM) && spec->fmt_space)
  {
    if (!vstr_nx_add_rep_chr(base, *pos, ' ', 1))
      return (FALSE);
    ++*pos;
  }
  
  if (spec->fmt_zero && space_len)
  {
    if (!vstr_nx_add_rep_chr(base, *pos, '0', space_len))
      return (FALSE);
    *pos += space_len;
    space_len = 0;
  }

  spec->obj_field_width  = space_len;

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

  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);
  
  if (!vstr_nx_add_vstr(base, pos, sf, sf_pos, sf_len, sf_flags))
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
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);
  
  if (!vstr_nx_add_buf(base, pos, buf, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

int vstr_nx_sc_fmt_add_buf(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_buf,
                          VSTR_TYPE_FMT_PTR_CHAR,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
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
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_DEF))
    return (FALSE);
  
  if (!vstr_nx_add_ptr(base, pos, ptr, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

int vstr_nx_sc_fmt_add_ptr(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_ptr,
                          VSTR_TYPE_FMT_PTR_CHAR,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
}

static int vstr__sc_fmt_add_cb_non(Vstr_base *base, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  size_t sf_len   = VSTR_FMT_CB_ARG_VAL(spec, size_t, 0);

  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);
  
  if (!vstr_nx_add_non(base, pos, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

int vstr_nx_sc_fmt_add_non(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_non,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
}

static int vstr__sc_fmt_add_cb_ref(Vstr_base *base, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  Vstr_ref *ref = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_off = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);
  size_t sf_len = VSTR_FMT_CB_ARG_VAL(spec, size_t, 2);

  assert(ref);
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);
  
  if (!vstr_nx_add_ref(base, pos, ref, sf_off, sf_len))
    return (FALSE);
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

int vstr_nx_sc_fmt_add_ref(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_ref,
                          VSTR_TYPE_FMT_PTR_VOID,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_SIZE_T,
                          VSTR_TYPE_FMT_END));
}

static int vstr__sc_fmt_add_cb_bkmg__uint(Vstr_base *base, size_t pos,
                                          Vstr_fmt_spec *spec,
                                          const char *buf_B,
                                          const char *buf_K,
                                          const char *buf_M,
                                          const char *buf_G)
{
  const unsigned int val_K = (1000);
  const unsigned int val_M = (1000 * 1000);
  const unsigned int val_G = (1000 * 1000 * 1000);
  unsigned int bkmg = VSTR_FMT_CB_ARG_VAL(spec, unsigned int, 0);
  unsigned int bkmg_whole = bkmg;
  unsigned int val_len = 0;
  size_t sf_len = SIZE_MAX;
  const char *end_bkmg = buf_B;
  unsigned int num_added = 0;
  unsigned int mov_dot_back = 0;
  unsigned int prec = 0;

  assert(strlen(buf_B) <= strlen(buf_M));
  assert(strlen(buf_K) == strlen(buf_M));
  assert(strlen(buf_K) == strlen(buf_G));
  
  if (0)
  { /* do nothing */ }
  else if (bkmg >= val_G)
  {
    bkmg_whole = bkmg / val_G;
    mov_dot_back = 9;
    end_bkmg = buf_G;
  }
  else if (bkmg >= val_M)
  {
    bkmg_whole = bkmg / val_M;
    mov_dot_back = 6;
    end_bkmg = buf_M;
  }
  else if (bkmg >= val_K)
  {
    bkmg_whole = bkmg / val_K;
    mov_dot_back = 3;
    end_bkmg = buf_K;
  }

  if (bkmg_whole >= 100)
    val_len = 3;
  else if (bkmg_whole >= 10)
    val_len = 2;
  else
    val_len = 1;
  
  if (spec->fmt_precision)
    prec = spec->obj_precision;
  else
    prec = 2;

  if (prec > mov_dot_back)
    prec = mov_dot_back;
  
  /* One of.... 
   * NNN '.' N+ end_bkmg 
   * NNN        end_bkmg */
  sf_len = val_len + !!prec + prec + strlen(end_bkmg);

  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_OBJ_NUM))
    return (FALSE);

  if (TRUE)
  {
    char buf_dot[2] = {0, 0};
    int num_iadded = 0;
    
    if (prec)
      buf_dot[0] = '.';
    
    if (!vstr_nx_add_sysfmt(base, pos, "%u%n%s%s", bkmg, &num_iadded,
                            buf_dot, end_bkmg))
      return (FALSE);
    num_added = num_iadded;
  }
  
  assert(val_len == (num_added - mov_dot_back));
  assert(num_added >= val_len);
  assert(num_added > mov_dot_back);
  
  if (prec && !vstr_nx_mov(base, pos + val_len,
                           base, pos + num_added + 1, 1))
    return (FALSE);
  
  if (TRUE)
  {
    size_t num_keep = val_len + prec;
    if (num_added > num_keep)
      vstr_nx_del(base, pos + 1 + num_keep + !!prec, (num_added - num_keep));
  }
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
  
  return (TRUE);
}

static int vstr__sc_fmt_add_cb_bkmg_Byte_uint(Vstr_base *base, size_t pos,
                                              Vstr_fmt_spec *spec)
{
  return (vstr__sc_fmt_add_cb_bkmg__uint(base, pos, spec,
                                         "B", "KB", "MB", "GB"));
}

int vstr_nx_sc_fmt_add_bkmg_Byte_uint(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_bkmg_Byte_uint,
                          VSTR_TYPE_FMT_UINT,
                          VSTR_TYPE_FMT_END));
}

static int vstr__sc_fmt_add_cb_bkmg_Bytes_uint(Vstr_base *base, size_t pos,
                                               Vstr_fmt_spec *spec)
{
  return (vstr__sc_fmt_add_cb_bkmg__uint(base, pos, spec,
                                         "B/s", "KB/s", "MB/s", "GB/s"));
}

int vstr_nx_sc_fmt_add_bkmg_Bytes_uint(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_bkmg_Bytes_uint,
                          VSTR_TYPE_FMT_UINT,
                          VSTR_TYPE_FMT_END));
}

static int vstr__sc_fmt_add_cb_bkmg_bit_uint(Vstr_base *base, size_t pos,
                                             Vstr_fmt_spec *spec)
{
  return (vstr__sc_fmt_add_cb_bkmg__uint(base, pos, spec,
                                             "b", "Kb", "Mb", "Gb"));
}

int vstr_nx_sc_fmt_add_bkmg_bit_uint(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_bkmg_bit_uint,
                          VSTR_TYPE_FMT_UINT,
                          VSTR_TYPE_FMT_END));
}

static int vstr__sc_fmt_add_cb_bkmg_bits_uint(Vstr_base *base, size_t pos,
                                              Vstr_fmt_spec *spec)
{
  return (vstr__sc_fmt_add_cb_bkmg__uint(base, pos, spec,
                                             "b/s", "Kb/s", "Mb/s", "Gb/s"));
}

int vstr_nx_sc_fmt_add_bkmg_bits_uint(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_bkmg_bits_uint,
                          VSTR_TYPE_FMT_UINT,
                          VSTR_TYPE_FMT_END));
}

void vstr_nx_sc_basename(const Vstr_base *base, size_t pos, size_t len,
                         size_t *ret_pos, size_t *ret_len)
{
  size_t ls = vstr_nx_srch_chr_rev(base, pos, len, '/');
  size_t end_pos = (pos + len) - 1;
  
  if (!ls)
  {
    *ret_pos = pos;
    *ret_len = len;
  }
  else if (ls == pos)
  {
    *ret_pos = pos;
    *ret_len = 0;
  }
  else if (ls == end_pos)
  {
    char buf[1] = {'/'};

    ls = vstr_nx_spn_chrs_rev(base, pos, len, buf, 1);
    vstr_nx_sc_basename(base, pos, len - ls, ret_pos, ret_len);
  }
  else
  {
    ++ls;
    *ret_pos = ls;
    *ret_len = len - (ls - pos);
  }
}

void vstr_nx_sc_dirname(const Vstr_base *base, size_t pos, size_t len,
                        size_t *ret_len)
{
  size_t ls = vstr_nx_srch_chr_rev(base, pos, len, '/');
  size_t end_pos = (pos + len) - 1;
  const char buf[1] = {'/'};

 
  if (!ls)
    *ret_len = 0;
  else if (ls == end_pos)
  {
    ls = vstr_nx_spn_chrs_rev(base, pos, len, buf, 1);
    len -= ls;
    if (!len)
      *ret_len = 1;
    else
      vstr_nx_sc_dirname(base, pos, len, ret_len);
  }
  else
  {
    len = (ls - pos) + 1;
    ls = vstr_nx_spn_chrs_rev(base, pos, len - 1, buf, 1);
    *ret_len = len - ls;
  }
}
