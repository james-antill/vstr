#define VSTR_SUB_C
/*
 *  Copyright (C) 2001  James Antill
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
/* functions to substitute data in a vstr */
#include "main.h"

int vstr_sub_buf(Vstr_base *base, size_t pos, size_t len,
                 const void *buf, size_t buf_len)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t real_len = len;
  int ret = TRUE;

  assert(len && buf_len);
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  while (scan && (scan->type == VSTR_TYPE_NODE_BUF))
  {
    size_t tmp = scan_len;
    
    if (tmp > real_len)
      tmp = real_len;
    if (tmp > buf_len)
      tmp = buf_len;

    memcpy(scan_str, buf, tmp);
    pos += tmp;
    real_len -= tmp;
    buf = ((char *)buf) + tmp;
    buf_len -= tmp;
    if (!buf_len || !real_len)
      break;
    
    scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                   scan, &scan_str, &scan_len);
  }

  if (real_len)
    ret = vstr_del(base, pos, real_len);

  if (ret && buf_len)
    vstr_add_buf(base, pos - 1, buf, buf_len);

  return (ret);
}

int vstr_sub_ptr(Vstr_base *base, size_t pos, size_t len,
                 const void *ptr, size_t ptr_len)
{
  int ret = vstr_del(base, pos, len);

  if (ret)
    vstr_add_ptr(base, pos - 1, ptr, ptr_len);

  return (ret);
}

int vstr_sub_non(Vstr_base *base, size_t pos, size_t len, size_t non_len)
{
  int ret = vstr_del(base, pos, len);

  if (ret)
    vstr_add_non(base, pos - 1, non_len);

  return (ret);
}

int vstr_sub_ref(Vstr_base *base, size_t pos, size_t len,
                 Vstr_ref *ref, size_t off, size_t ref_len)
{
  int ret = vstr_del(base, pos, len);

  if (ret)
    vstr_add_ref(base, pos - 1, ref, off, ref_len);

  return (ret);
}

int vstr_sub_vstr(Vstr_base *base, size_t pos, size_t len,
                  Vstr_base *from_base, size_t from_pos, size_t from_len,
                  unsigned int type)
{
  int ret = vstr_del(base, pos, len);

  if (ret)
    ret = vstr_add_vstr(base, pos - 1, from_base, from_pos, from_len, type);

  return (ret);
}
