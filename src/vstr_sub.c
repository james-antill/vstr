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

#define VSTR__SUB_MEMCPY_BUF(obuf, off) do { \
  switch (tmp) \
  { /* Don't do a memcpy() on small values */ \
    case 7:  (obuf)[off + 6] = ((const char *)buf)[6]; \
    case 6:  (obuf)[off + 5] = ((const char *)buf)[5]; \
    case 5:  (obuf)[off + 4] = ((const char *)buf)[4]; \
    case 4:  (obuf)[off + 3] = ((const char *)buf)[3]; \
    case 3:  (obuf)[off + 2] = ((const char *)buf)[2]; \
    case 2:  (obuf)[off + 1] = ((const char *)buf)[1]; \
    case 1:  (obuf)[off + 0] = ((const char *)buf)[0]; \
             break; \
    default: memcpy((obuf) + off, buf, tmp); \
             break; \
  } \
} while (FALSE)

#define VSTR__SUB_BUF() do { \
  if (tmp > buf_len) \
    tmp = buf_len; \
  \
  VSTR__SUB_MEMCPY_BUF(((Vstr_node_buf *)scan)->buf, pos); \
  buf_len -= tmp; \
  buf = ((char *)buf) + tmp; \
} while (FALSE)


static int vstr__sub_buf_fast(Vstr_base *base, size_t pos, size_t len,
                              const void *buf)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  size_t buf_len = len;

  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan)
    return (FALSE);
  
  do
  {
    const size_t tmp = scan_len;

    assert(scan->type == VSTR_TYPE_NODE_BUF);
    
    VSTR__SUB_MEMCPY_BUF(scan_str, 0);
    buf = ((char *)buf) + tmp;
  } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));

  vstr_nx_cache_cb_sub(base, pos, buf_len);

  return (TRUE);
}

static int vstr__sub_buf_slow(Vstr_base *base, size_t pos, size_t len,
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
  size_t real_pos = 0;
  
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
    
    ret = vstr_nx_del(base, pos + buf_len, len + del_len);
    if (!ret)
    {
      ret = vstr_nx_del(base, pos, buf_len);
      assert(ret); /* this must work as a split can't happen */

      assert(vstr__check_spare_nodes(base->conf));
      assert(vstr__check_real_nodes(base));
      return (FALSE);
    }
    
    return (TRUE);
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
  
  real_pos = orig_pos;
  
  if (!sub_add_len) /* length we are replacing is _just_ _BUF nodes */
  {
    vstr__sub_buf_fast(base, real_pos, len, buf);

    real_pos += len;
    buf_len -= len;
    buf = ((char *)buf) + len;
  }
  else
  {
    size_t rm_pos = 0;
    size_t rm_len = 0;
      
    /* loop again removing any non _BUF nodes and substituting on _BUF */
    if ((pos + len - 1) > base->len)
      len = base->len - (pos - 1);
    scan = vstr__base_pos(base, &pos, &num, TRUE);
    assert(scan);
    
    --pos;
    
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
        
        vstr_nx_cache_cb_sub(base, real_pos, tmp);
        
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
            vstr__cache_iovec_reset_node(base, scan,
                                         vstr__num_node(base, scan));
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

    assert(!buf_len || (add_len >= buf_len));
  }
  
  if (del_len)
    vstr_nx_del(base, real_pos, del_len);

  if (buf_len)
    vstr_nx_add_buf(base, real_pos - 1, buf, buf_len);

  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));

  return (TRUE);
}

int vstr_nx_sub_buf(Vstr_base *base, size_t pos, size_t len,
                    const void *buf, size_t buf_len)
{
  if (!len)
    return (vstr_nx_add_buf(base, pos - 1, buf, buf_len));
  
  if ((len == buf_len) &&
      !base->node_non_used &&
      !base->node_ptr_used &&
      !base->node_ref_used)
    return (vstr__sub_buf_fast(base, pos, len, buf));
  
  return (vstr__sub_buf_slow(base, pos, len, buf, buf_len));
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
{ /* TODO: this is inefficient compared to vstr_sub_buf() because of atomic
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

int vstr_nx_sub_rep_chr(Vstr_base *base, size_t pos, size_t len,
                        char chr, size_t rep_len)
{
  int ret = TRUE;

  /* TODO: this is a simple opt. for _netstr */
  if ((len == rep_len) &&
      !base->node_non_used &&
      !base->node_ptr_used &&
      !base->node_ref_used)
  {
    Vstr_node *scan = NULL;
    unsigned int num = 0;
    char *scan_str = NULL;
    size_t scan_len = 0;
    
    scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
    if (!scan)
      return (FALSE);
    do
    {
      const size_t tmp = scan_len;
      
      assert(scan->type == VSTR_TYPE_NODE_BUF);
      
      memset(scan_str, chr, tmp);
    } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                             scan, &scan_str, &scan_len)));

    vstr_nx_cache_cb_sub(base, pos, rep_len);
    
    return (TRUE);
  }

  ret = vstr_nx_add_rep_chr(base, pos - 1, chr, rep_len);

  if (!len)
    return (ret);
  
  if (!ret)
    return (FALSE);

  ret = vstr_nx_del(base, pos + rep_len, len);
  if (!ret)
  {
    ret = vstr_nx_del(base, pos, rep_len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }

  return (ret);  
}
