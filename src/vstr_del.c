#define VSTR_DEL_C
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
/* function to delete data from a vstr */
#include "main.h"


static void vstr__cache_iovec_reset(Vstr_base *base)
{
 assert(!base->num);
 assert(!base->len);

 if (!(base->cache_available && VSTR__CACHE(base) &&
       VSTR__CACHE(base)->vec && VSTR__CACHE(base)->vec->sz))
   return;

 VSTR__CACHE(base)->vec->off = 0;
 base->iovec_upto_date = TRUE;
 
 if (VSTR__CACHE(base)->vec->sz > base->conf->iov_min_offset)
   VSTR__CACHE(base)->vec->off = base->conf->iov_min_offset;
}

static void vstr__cache_iovec_del_node_end(Vstr_base *base, unsigned int num,
                                           unsigned int len)
{
 if (!base->iovec_upto_date)
   return;
 
 VSTR__CACHE(base)->vec->v[num - 1].iov_len -= len;
}

static void vstr__cache_iovec_del_node_beg(Vstr_base *base, Vstr_node *node,
                                           unsigned int num, unsigned int len)
{
 if (!base->iovec_upto_date)
   return;

 num += VSTR__CACHE(base)->vec->off - 1;
 
 if (node->type != VSTR_TYPE_NODE_NON)
 {
  char *tmp = VSTR__CACHE(base)->vec->v[num].iov_base;
  tmp += len;
  VSTR__CACHE(base)->vec->v[num].iov_base = tmp;
 }

 VSTR__CACHE(base)->vec->v[num].iov_len -= len;
 
 assert((node->len == VSTR__CACHE(base)->vec->v[num].iov_len) ||
        ((base->beg == node) &&
         (node->len == (VSTR__CACHE(base)->vec->v[num].iov_len + base->used))));
}

static void vstr__cache_iovec_del_beg(Vstr_base *base, unsigned int num)
{
 if (!base->iovec_upto_date)
   return;

 VSTR__CACHE(base)->vec->off += num;

 assert(VSTR__CACHE(base)->vec->sz > VSTR__CACHE(base)->vec->off);
 assert((VSTR__CACHE(base)->vec->sz - VSTR__CACHE(base)->vec->off) >= base->num);
}

static void vstr__del_beg_cleanup(Vstr_base *base, unsigned int type,
                                  unsigned int num,
                                  Vstr_node **scan, int base_update)
{
 if (num)
 {
  Vstr_node *tmp = NULL;
  
  switch (type)
  {
   case VSTR_TYPE_NODE_BUF:
     tmp = (Vstr_node *)base->conf->spare_buf_beg;
     
     base->conf->spare_buf_num += num;
     
     base->conf->spare_buf_beg = (Vstr_node_buf *)base->beg;
     break;
     
   case VSTR_TYPE_NODE_NON:
     tmp = (Vstr_node *)base->conf->spare_non_beg;
     
     base->conf->spare_non_num += num;
     
     base->conf->spare_non_beg = (Vstr_node_non *)base->beg;
     break;
     
   case VSTR_TYPE_NODE_PTR:
     tmp = (Vstr_node *)base->conf->spare_ptr_beg;
     
     base->conf->spare_ptr_num += num;
     
     base->conf->spare_ptr_beg = (Vstr_node_ptr *)base->beg;
     break;
     
   case VSTR_TYPE_NODE_REF:
     tmp = (Vstr_node *)base->conf->spare_ref_beg;
     
     base->conf->spare_ref_num += num;
     
     base->conf->spare_ref_beg = (Vstr_node_ref *)base->beg;
     break;

   default:
     assert(FALSE);
     return;
  }

  base->beg = *scan;
  *scan = tmp;
  
  if (base_update)
  {
   base->num -= num;
   
   if (!base->beg)
     base->end = NULL;

   vstr__cache_iovec_del_beg(base, num);
  }
 }
}

static void vstr__del_all(Vstr_base *base)
{
  size_t orig_len = base->len;
  unsigned int num = 0;
  unsigned int type;
  Vstr_node **scan = NULL;

  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));
  
  if (!base->len)
    return;
 
 scan = &base->beg;
 type = (*scan)->type;
 
 base->len = 0;
 
 while (*scan)
 {
  if ((*scan)->type != type)
  {
   vstr__del_beg_cleanup(base, type, num, scan, FALSE);
   type = base->beg->type;
   scan = &base->beg;
   num = 0;
  }
  
  ++num;
  
  if ((*scan)->type == VSTR_TYPE_NODE_REF)
    vstr_nx_ref_del(((Vstr_node_ref *)*scan)->ref);
  
  scan = &(*scan)->next;
 }
 vstr__del_beg_cleanup(base, type, num, scan, FALSE);
 
 base->used = 0;
 base->num = 0;
 assert(!base->beg);
 base->end = NULL;
 
 vstr__cache_iovec_reset(base);

 vstr__cache_del(base, 1, orig_len);
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));
}

static void vstr__del_beg(Vstr_base *base, size_t len)
{
  size_t orig_len = len;
  unsigned int num = 0;
  unsigned int type;
  Vstr_node **scan = NULL;
  
  assert(len && base->beg);
  
  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));
  
  if (!len)
    return;

 if (!base->beg)
   return;

 scan = &base->beg;
 type = (*scan)->type;
 
 base->len -= len;
 
 if (base->used)
 {
  assert((*scan)->len > base->used);
  
  if (len < (size_t)((*scan)->len - base->used))
  {
   base->used += len;
   
   vstr__cache_iovec_del_node_beg(base, *scan, 1, len);

   vstr__cache_del(base, 1, orig_len);

   assert(vstr__check_real_nodes(base));
   return;
  }
  
  num = 1;
  
  len -= ((*scan)->len - base->used);
  assert((*scan)->type == VSTR_TYPE_NODE_BUF);
  /*  vstr_ref_del(((Vstr_node_ref *)(*scan))->ref); */
  type = VSTR_TYPE_NODE_BUF;
  
  scan = &(*scan)->next;
  
  base->used = 0;
  
  if (!*scan)
    assert(len == 0);
 }
 
 while (len > 0)
 {
  if ((*scan)->type != type)
  {
   vstr__del_beg_cleanup(base, type, num, scan, TRUE);
   type = base->beg->type;
   scan = &base->beg;
   num = 0;
  }
  
  if (len < (*scan)->len)
  {
    switch ((*scan)->type)
    {
      case VSTR_TYPE_NODE_BUF:
        base->used = len;
        break;
      case VSTR_TYPE_NODE_NON:
        (*scan)->len -= len;
        break;
      case VSTR_TYPE_NODE_PTR:
      {
        char *tmp = ((Vstr_node_ptr *)(*scan))->ptr;
        ((Vstr_node_ptr *)(*scan))->ptr = tmp + len;
        (*scan)->len -= len;
      }
      break;
      case VSTR_TYPE_NODE_REF:
        ((Vstr_node_ref *)(*scan))->off += len;
        (*scan)->len -= len;
        break;
      default:
        assert(FALSE);
    }

   break;
  }
  
  ++num;
  len -= (*scan)->len;
  if ((*scan)->type == VSTR_TYPE_NODE_REF)
    vstr_nx_ref_del(((Vstr_node_ref *)(*scan))->ref);
  
  scan = &(*scan)->next;
 }
 
 vstr__del_beg_cleanup(base, type, num, scan, TRUE);
 
 if (len)
   vstr__cache_iovec_del_node_beg(base, base->beg, 1, len);

 vstr__cache_del(base, 1, orig_len);
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));
}

static Vstr_node *vstr__del_end_cleanup(Vstr_conf *conf, unsigned int type,
                                        unsigned int num,
                                        Vstr_node *beg, Vstr_node **end)
{
 Vstr_node *ret = *end;
 
 switch (type)
 {
  case VSTR_TYPE_NODE_BUF:
    *end = (Vstr_node *)conf->spare_buf_beg;
    
    conf->spare_buf_num += num;
    
    conf->spare_buf_beg = (Vstr_node_buf *)beg;
    break;
    
  case VSTR_TYPE_NODE_NON:
    *end = (Vstr_node *)conf->spare_non_beg;
    
    conf->spare_non_num += num;
    
    conf->spare_non_beg = (Vstr_node_non *)beg;
    break;
    
  case VSTR_TYPE_NODE_PTR:
    *end = (Vstr_node *)conf->spare_ptr_beg;
    
    conf->spare_ptr_num += num;
    
    conf->spare_ptr_beg = (Vstr_node_ptr *)beg;
    break;
    
  case VSTR_TYPE_NODE_REF:
    *end = (Vstr_node *)conf->spare_ref_beg;
    
    conf->spare_ref_num += num;
    
    conf->spare_ref_beg = (Vstr_node_ref *)beg;
    break;
    
  default:
    assert(FALSE);
    return (ret);
 }
 
 return (ret);
}

int vstr_nx_extern_inline_del(Vstr_base *base, size_t pos, size_t len)
{
 unsigned int num = 0;
 size_t orig_pos = pos;
 size_t orig_len = len;
 Vstr_node *scan = NULL;
 Vstr_node **pscan = NULL;
 Vstr_node *beg = NULL;
 int type;
 Vstr_node *saved_beg = NULL;
 unsigned int saved_num = 0;
 unsigned int del_nodes = 0;

 assert(base && pos && ((pos + len - 1) <= base->len));
 
 if (!len && !base->len)
   return (TRUE);
 
 assert(pos && len);

 if (pos > base->len)
   return (FALSE);

 if (pos <= 1)
 {
   if (len >= base->len)
   {
     vstr__del_all(base);
     
     return (TRUE);
   }
   
   vstr__del_beg(base, len);
   return (TRUE);
 }

 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));
 
 --pos; /* pos == ammount of chars ok from begining */

 if ((pos + len) >= base->len)
   len = base->len - pos;
 
 scan = vstr__base_pos(base, &pos, &num, FALSE);
 
 base->len -= len;

 if (pos != scan->len)
 { /* need to del some data from this node ... */
  size_t tmp = scan->len - pos;

  if (len >= tmp)
  {
   vstr__cache_iovec_del_node_end(base, num, tmp);
   scan->len = pos;
   len -= tmp;
  }
  else
  { /* delete from the middle of a single node */
   switch (scan->type)
   {
    case VSTR_TYPE_NODE_NON:
      break;
      
    case VSTR_TYPE_NODE_BUF:
      if (!base->conf->split_buf_del)
      {
       memmove(((Vstr_node_buf *)scan)->buf + pos,
               ((Vstr_node_buf *)scan)->buf + pos + len,
               scan->len - (pos + len));
       break;
      }
      /* else FALLTHROUGH */
      
    default:
      scan = vstr__base_split_node(base, scan, pos + len);
      if (!scan)
      {
       base->len += len; /* make sure nothing changed */
       
       assert(vstr__check_spare_nodes(base->conf));
       assert(vstr__check_real_nodes(base));
       
       return (FALSE);
      }
      break;
   }
   scan->len -= len;

   vstr__cache_del(base, orig_pos, orig_len);
   
   assert(vstr__check_spare_nodes(base->conf));
   assert(vstr__check_real_nodes(base));
   
   return (TRUE);
  }
 }

 saved_beg = scan;
 saved_num = num;
 del_nodes = 0;
 
 if (!len)
 {
  vstr__cache_del(base, orig_pos, orig_len);
  
  assert(vstr__check_spare_nodes(base->conf));
  assert(vstr__check_real_nodes(base));

  return (TRUE);
 }
 
 scan = scan->next;
 assert(scan);
 
 if (len < scan->len)
   goto fix_last_node;

 len -= scan->len;
 type = scan->type;
 beg = scan;
 pscan = &scan->next;
 num = 1;
 while (*pscan && (len >= (*pscan)->len))
 {
  if ((*pscan)->type == VSTR_TYPE_NODE_REF)
    vstr_nx_ref_del(((Vstr_node_ref *)(*pscan))->ref);
  
  len -= (*pscan)->len;
  
  if ((*pscan)->type != type)
  {
   del_nodes += num;
   
   scan = vstr__del_end_cleanup(base->conf, type, num, beg, pscan);
   type = scan->type;
   pscan = &scan->next;
   num = 1;
   beg = scan;
   continue;
  }
  
  ++num;
  pscan = &(*pscan)->next;
 }
 
 scan = vstr__del_end_cleanup(base->conf, type, num, beg, pscan);
 del_nodes += num;
 
 if (base->iovec_upto_date)
 {
  size_t rebeg_num = saved_num + del_nodes;
  
  memmove(VSTR__CACHE(base)->vec->v + VSTR__CACHE(base)->vec->off + saved_num,
          VSTR__CACHE(base)->vec->v + VSTR__CACHE(base)->vec->off + rebeg_num,
          (base->num - rebeg_num) * sizeof(struct iovec));
 }
 base->num -= del_nodes;
 
 saved_beg->next = scan;
 if (!scan)
   base->end = saved_beg;
 else if (len)
 {
 fix_last_node:
  assert(len < scan->len);
  
  switch (scan->type)
  {
   case VSTR_TYPE_NODE_BUF:
     /* if (!base->conf->split_buf_del) */
     /* no point in splitting */
     memmove(((Vstr_node_buf *)scan)->buf,
             ((Vstr_node_buf *)scan)->buf + len,
             scan->len - len);
     break;
   case VSTR_TYPE_NODE_NON:
     break;
   case VSTR_TYPE_NODE_PTR:
     ((Vstr_node_ptr *)scan)->ptr= ((char *)((Vstr_node_ptr *)scan)->ptr) + len;
     break;
   case VSTR_TYPE_NODE_REF:
     ((Vstr_node_ref *)scan)->off += len;
     break;
   default:
     assert(FALSE);
  }
  scan->len -= len;
  
  vstr__cache_iovec_reset_node(base, scan, saved_num + 1);
 }

 vstr__cache_del(base, orig_pos, orig_len);
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));
 
 return (TRUE);
}

