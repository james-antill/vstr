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
  size_t orig_pos = pos;
  size_t orig_buf_len = buf_len;
  const void *orig_buf = buf;
  int ret = TRUE;

  assert(len && buf_len);

  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));

  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  while (scan && (scan->type == VSTR_TYPE_NODE_BUF))
  {
    size_t tmp = scan_len;
    
    if (tmp > real_len)
      tmp = real_len;
    if (tmp > buf_len)
      tmp = buf_len;

    /* can't roll back if we do it now -- memcpy(scan_str, buf, tmp); */
    
    pos += tmp;
    buf = ((char *)buf) + tmp;
    real_len -= tmp;
    buf_len -= tmp;
    if (!buf_len || !real_len)
      break;
    
    scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                   scan, &scan_str, &scan_len);
  }

  if (buf_len)
    ret = vstr_add_buf(base, pos - 1, buf, buf_len);

  if (!ret)
    return (FALSE);
  
  if (real_len)
    ret = vstr_del(base, pos + buf_len, real_len);
  if (!ret)
  {
    ret = vstr_del(base, pos, buf_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }

  /* nothing can go wrong at this point ... */
  pos = orig_pos;
  len = orig_buf_len - buf_len;
  if (!len)
    return (ret);
  
  scan = NULL;
  num = 0;
  scan_str = NULL;
  scan_len = 0;
  buf = orig_buf;
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  while (scan && (scan->type == VSTR_TYPE_NODE_BUF))
  {
    size_t tmp = scan_len; /* scan_len is always right now */

    memcpy(scan_str, buf, tmp);
    pos += tmp;
    buf = ((char *)buf) + tmp;
    
    scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                   scan, &scan_str, &scan_len);
  }  

  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));

  return (ret);
}

int vstr_sub_ptr(Vstr_base *base, size_t pos, size_t len,
                 const void *ptr, size_t ptr_len)
{
  int ret = vstr_add_ptr(base, pos - 1, ptr, ptr_len);
  
  if (!ret)
    return (FALSE);
  
  ret = vstr_del(base, pos + ptr_len, len);
  if (!ret)
  {
    ret = vstr_del(base, pos, ptr_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }
  
  return (ret);
}

int vstr_sub_non(Vstr_base *base, size_t pos, size_t len, size_t non_len)
{
  int ret = vstr_add_non(base, pos - 1, non_len);

  if (!ret)
    return (FALSE);

  ret = vstr_del(base, pos + non_len, len);
  if (!ret)
  {
    ret = vstr_del(base, pos, non_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }

  return (ret);
}

int vstr_sub_ref(Vstr_base *base, size_t pos, size_t len,
                 Vstr_ref *ref, size_t off, size_t ref_len)
{
  int ret = vstr_add_ref(base, pos - 1, ref, off, ref_len);

  if (!ret)
    return (FALSE);
  
  ret = vstr_del(base, pos + ref_len, len);
  if (!ret)
  {
    ret = vstr_del(base, pos, ref_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }

  return (ret);
}

int vstr_sub_vstr(Vstr_base *base, size_t pos, size_t len,
                  const Vstr_base *from_base, size_t from_pos, size_t from_len,
                  unsigned int type)
{
  int ret = vstr_add_vstr(base, pos - 1, from_base, from_pos, from_len, type);

  if (!ret)
    return (FALSE);

  ret = vstr_del(base, pos + from_len, len);
  if (!ret)
  {
    ret = vstr_del(base, pos, from_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  } 

  return (ret);
}
