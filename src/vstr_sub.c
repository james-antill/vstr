#define VSTR_SUB_C
/*
 *  Copyright (C) 2001, 2002  James Antill
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

#define VSTR__SUB_BUF() do { \
  if (tmp > buf_len) \
    tmp = buf_len; \
  \
  switch (tmp) \
  { /* Don't do a memcpy() on small values */ \
    case 7:  ((Vstr_node_buf *)scan)->buf[pos + 6] = ((const char *)buf)[6]; \
    case 6:  ((Vstr_node_buf *)scan)->buf[pos + 5] = ((const char *)buf)[5]; \
    case 5:  ((Vstr_node_buf *)scan)->buf[pos + 4] = ((const char *)buf)[4]; \
    case 4:  ((Vstr_node_buf *)scan)->buf[pos + 3] = ((const char *)buf)[3]; \
    case 3:  ((Vstr_node_buf *)scan)->buf[pos + 2] = ((const char *)buf)[2]; \
    case 2:  ((Vstr_node_buf *)scan)->buf[pos + 1] = ((const char *)buf)[1]; \
    case 1:  ((Vstr_node_buf *)scan)->buf[pos + 0] = ((const char *)buf)[0]; \
             break; \
    default: memcpy(((Vstr_node_buf *)scan)->buf + pos, buf, tmp); \
             break; \
  } \
  buf_len -= tmp; \
  buf = ((char *)buf) + tmp; \
} while (FALSE)


int vstr_nx_sub_buf(Vstr_base *base, size_t pos, size_t len,
                    const void *buf, size_t buf_len)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t orig_pos = pos;
  size_t orig_len = 0;
  size_t orig_buf_len = buf_len;
  size_t sub_add_len = 0;
  size_t add_len = 0;
  size_t del_len = 0;
  size_t rm_pos = 0;
  size_t rm_len = 0;
  size_t real_pos = 0;

  if (!len)
    return (vstr_nx_add_buf(base, pos - 1, buf, buf_len));
  
  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));
  
  if (len > buf_len)
  {
    del_len = len - buf_len;
    len = buf_len;
  }
  else
    add_len = buf_len - len;
  
  orig_len = len;
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan)
    return (FALSE);
  do
  {
    size_t tmp = scan_len;
    
    if (tmp > buf_len)
      tmp = buf_len;
    
    if (scan->type != VSTR_TYPE_NODE_BUF)
      sub_add_len += tmp;
    
    buf_len -= tmp;
  } while (buf_len &&
           (scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));

  len = orig_len;
  buf_len = orig_buf_len;
  
  if (sub_add_len == buf_len)
  { /* no _BUF nodes, so we can't optimise it anyway ... */
    int ret = vstr_nx_add_buf(base, pos - 1, buf, buf_len);
    
    if (!ret)
    {
      assert(vstr__check_spare_nodes(base->conf));
      assert(vstr__check_real_nodes(base));
      return (FALSE);
    }
    
    ret = vstr_nx_del(base, pos + buf_len, len);
    if (!ret)
    {
      ret = vstr_nx_del(base, pos, buf_len);
      assert(ret); /* this must work as a split can't happen */

      assert(vstr__check_spare_nodes(base->conf));
      assert(vstr__check_real_nodes(base));
      return (FALSE);
    }
  }

  add_len += sub_add_len;
  /* allocate extra _BUF nodes needed, all other are ok -- no splits will
   * happen on non _BUF nodes */
  num = (add_len / base->conf->buf_sz) + 2;
  if (num > base->conf->spare_buf_num)
  {
    num -= base->conf->spare_buf_num;
    if (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, num) != num)
      return (FALSE);
  }
  
  /* loop again removing any non _BUF nodes and substituting on _BUF */
  if ((pos + len - 1) > base->len)
    len = base->len - (pos - 1);
  scan = vstr__base_pos(base, &pos, &num, TRUE);
  assert(scan);
  
  --pos;

  real_pos = orig_pos;
  while (buf_len && len)
  {
    size_t tmp = 0;
      
    assert(scan);
    
    if (rm_pos)
    {
      assert(rm_pos == real_pos);
      vstr_nx_del(base, rm_pos, rm_len);
    }

    tmp = scan->len - pos;
    if (tmp > len)
      tmp = len;
    
    if (scan->type != VSTR_TYPE_NODE_BUF)
    {
      rm_pos = real_pos;
      rm_len = tmp;
      len -= tmp;
    }
    else
    {
      rm_pos = 0;

      VSTR__SUB_BUF();
      
      vstr_nx_cache_sub(base, real_pos, tmp);
      
      len -= tmp;
      real_pos += tmp;

      if (buf_len && add_len && (((base->beg == scan) && base->used) ||
                                 (scan->len < base->conf->buf_sz)))
      { /* be more aggessive here than in vstr_add_buf() because when you are
         * adding you generally add a lot, but when subbing you often sub
         * just a little bit more or less than you started with */
        pos += tmp;
        if ((base->beg == scan) && base->used)
        {
          pos -= base->used;
          scan->len -= base->used;
          memmove(((Vstr_node_buf *)scan)->buf,
                  ((Vstr_node_buf *)scan)->buf + base->used,
                  scan->len);
          base->used = 0;
        }

        assert(scan->len < base->conf->buf_sz);
        
        tmp = base->conf->buf_sz - scan->len;
        if (tmp > add_len)
          tmp = add_len;
        if (tmp > buf_len)
          tmp = buf_len;

        if (pos < scan->len)
          memmove(((Vstr_node_buf *)scan)->buf + pos + tmp,
                  ((Vstr_node_buf *)scan)->buf + pos,
                  (scan->len - pos));
        
        VSTR__SUB_BUF();

        scan->len += tmp;
        base->len += tmp;
        add_len -= tmp;
        
        vstr__cache_add(base, real_pos, tmp);
        
        real_pos += tmp;
      
        if (base->iovec_upto_date)
          vstr__cache_iovec_reset_node(base, scan, vstr__num_node(base, scan));
      }
    }

    pos = 0;
    scan = scan->next;
  }

  if (rm_pos)
  {
    assert(rm_pos == real_pos);
    vstr_nx_del(base, rm_pos, rm_len);
  }
  
  if (del_len)
    vstr_nx_del(base, real_pos, del_len);

  assert(add_len >= buf_len);
  if (buf_len)
    vstr_nx_add_buf(base, real_pos - 1, buf, buf_len);

  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));

  return (TRUE);
}

int vstr_nx_sub_ptr(Vstr_base *base, size_t pos, size_t len,
                    const void *ptr, size_t ptr_len)
{
  int ret = vstr_nx_add_ptr(base, pos - 1, ptr, ptr_len);

  if (!len)
    return (ret);
  
  if (!ret)
    return (FALSE);
  
  ret = vstr_nx_del(base, pos + ptr_len, len);
  if (!ret)
  {
    ret = vstr_nx_del(base, pos, ptr_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }
  
  return (ret);
}

int vstr_nx_sub_non(Vstr_base *base, size_t pos, size_t len, size_t non_len)
{
  int ret = vstr_nx_add_non(base, pos - 1, non_len);

  if (!len)
    return (ret);
  
  if (!ret)
    return (FALSE);

  ret = vstr_nx_del(base, pos + non_len, len);
  if (!ret)
  {
    ret = vstr_nx_del(base, pos, non_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }

  return (ret);
}

int vstr_nx_sub_ref(Vstr_base *base, size_t pos, size_t len,
                    Vstr_ref *ref, size_t off, size_t ref_len)
{
  int ret = vstr_nx_add_ref(base, pos - 1, ref, off, ref_len);

  if (!len)
    return (ret);
  
  if (!ret)
    return (FALSE);
  
  ret = vstr_nx_del(base, pos + ref_len, len);
  if (!ret)
  {
    ret = vstr_nx_del(base, pos, ref_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }

  return (ret);
}

int vstr_nx_sub_vstr(Vstr_base *base, size_t pos, size_t len,
                     const Vstr_base *from_base,
                     size_t from_pos, size_t from_len, unsigned int type)
{ /* this is inefficient compared to vstr_sub_buf() because of atomic
   * guarantees - could be made to work with _ALL_BUF */
  int ret = TRUE;

  assert(pos && from_pos);
  
  if (from_len)
    ret = vstr_nx_add_vstr(base, pos - 1, from_base, from_pos, from_len,
                           type);

  if (!len)
    return (ret);
  
  if (!ret)
    return (FALSE);

  ret = vstr_nx_del(base, pos + from_len, len);
  if (!ret)
  {
    ret = vstr_nx_del(base, pos, from_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  } 

  return (ret);
}
