#define VSTR_ADD_C
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
/* function to add data to a vstr */
#include "main.h"

static int vstr__base_iovec_add_end(Vstr_base *base, Vstr_node *node,
                                    unsigned int len)
{
 char *tmp = NULL;
 
 if (!base->iovec_upto_date)
   return (TRUE);
 
 if (!vstr__base_iovec_alloc(base, base->num))
   return (FALSE);

 tmp = vstr__export_node_ptr(node);
 if (node == base->beg)
   tmp += base->used;
 
 base->vec.v[base->vec.off + base->num - 1].iov_len = len;
 base->vec.v[base->vec.off + base->num - 1].iov_base = tmp;

 return (TRUE);
}

static void vstr__base_iovec_add_node_end(Vstr_base *base, unsigned int num,
                                          unsigned int len)
{
 if (!base->iovec_upto_date)
   return;

 base->vec.v[num - 1].iov_len += len;
}

static int vstr__base_iovec_maybe_add_end(Vstr_base *base, Vstr_node *node,
                                          unsigned int len)
{
 if (!base->conf->iovec_auto_update)
   base->iovec_upto_date = FALSE;

 if (!base->iovec_upto_date)
   return (TRUE);

 if (base->iovec_upto_date)
 {
  if (base->num <= (base->vec.sz - base->vec.off))
    return (vstr__base_iovec_add_end(base, node, len));
  else
    base->iovec_upto_date = FALSE;
 }
 
 return (TRUE);
}

static Vstr_node *vstr__add_setup_pos(Vstr_base *base,
                                      size_t *pos,
                                      unsigned int *num,
                                      size_t *orig_scan_len)
{
 Vstr_node *scan = vstr__base_pos(base, pos, num, TRUE);
 Vstr_node *scan_next = NULL;
 
 if (orig_scan_len)
   *orig_scan_len = scan->len;
 
 if ((*pos != scan->len) &&
     !(scan_next = vstr__base_split_node(base, scan, *pos)))
   return (NULL);
 
 if (base->iovec_upto_date && (scan != base->end))
   base->iovec_upto_date = FALSE; /* doesn't work in the middle, atm. */

 return (scan);
}

static void vstr__add_fail_cleanup(Vstr_base *base,
                                   Vstr_node *pos_scan,
                                   Vstr_node *pos_scan_next,
                                   unsigned int num, size_t len,
                                   size_t orig_pos_scan_len)
{
 base->conf->malloc_bad = TRUE;
 base->num -= num;
 base->len -= len;
 if (pos_scan)
 {
  pos_scan->len = orig_pos_scan_len;
  pos_scan->next = pos_scan_next;
 }
 else
 {
  assert(!base->num);
  base->beg = NULL;
 }
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));
}

int vstr_add_buf(Vstr_base *base, size_t pos, size_t len, const void *buffer)
{
 unsigned int num = 0;
 size_t orig_pos = pos;
 size_t orig_len = len;
 Vstr_node *scan = NULL;
 Vstr_node *pos_scan = NULL;
 Vstr_node *pos_scan_next = NULL;
 size_t orig_pos_scan_len = 0;
 
 assert(!(!base || !buffer || !len || (pos > base->len) ||
          base->conf->malloc_bad));
 if (!base || !buffer || !len || (pos > base->len) || base->conf->malloc_bad)
   return (FALSE);
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 num = (len / base->conf->buf_sz) + 1;
 if (base->conf->spare_buf_num < num)
 {
  num -= base->conf->spare_buf_num;

  if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, num) != num)
    return (FALSE);
 }
 
 if (base->len)
 {
  scan = vstr__add_setup_pos(base, &pos, &num, &orig_pos_scan_len);
  if (!scan)
    return (FALSE);

  pos_scan = scan;
  pos_scan_next = scan->next;
  
  if ((scan->type == VSTR_TYPE_NODE_BUF) && (pos == scan->len) &&
      (base->conf->buf_sz > scan->len))
  {
   size_t tmp = (base->conf->buf_sz - base->end->len);
   
   assert(base->len);
   
   if (tmp > len)
     tmp = len;
   
   memcpy(((Vstr_node_buf *)scan)->buf + scan->len, buffer, tmp);
   scan->len += tmp;
   
   vstr__base_iovec_add_node_end(base, num, tmp);
   
   base->len += tmp;
   buffer = ((char *)buffer) + tmp;
   len -= tmp;
   
   if (!len)
   {
    assert(vstr__check_real_nodes(base));
    return (TRUE);
   }
  }
 }
 assert(!!pos_scan == !!orig_pos_scan_len);
 
 scan = (Vstr_node *)base->conf->spare_buf_beg;
 if (pos_scan)
 {
  assert(base->len);
  pos_scan->next = scan;
 }
 else
   base->beg = scan;

 num = 0;
 base->len += len;
 
 while (len > 0)
 {
  size_t tmp = base->conf->buf_sz;

  if (tmp > len)
    tmp = len;

  ++num;
  ++base->num;
  
  memcpy(((Vstr_node_buf *)scan)->buf, buffer, tmp);
  scan->len = tmp;

  if (!vstr__base_iovec_maybe_add_end(base, scan, tmp))
  {
   vstr__add_fail_cleanup(base, pos_scan, pos_scan_next,
                          num, len, orig_pos_scan_len);
   return (FALSE);
  }
  
  buffer = ((char *)buffer) + tmp;
  len -= tmp;

  if (!len)
    break;
  
  scan = scan->next;
 }
 
 base->conf->spare_buf_beg = (Vstr_node_buf *)scan->next;
 base->conf->spare_buf_num -= num;
 
 assert(!scan || (scan->type == VSTR_TYPE_NODE_BUF));
 
 if (!(scan->next = pos_scan_next))
   base->end = scan;

 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 vstr__cache_add(base, orig_pos, orig_len);
 
 return (TRUE);
}

int vstr_add_ptr(Vstr_base *base, size_t pos, size_t len, const void *pass_ptr)
{
 unsigned int num = 0;
 size_t orig_pos = pos;
 size_t orig_len = len;
 char *ptr = (char *)pass_ptr; /* store as a char *, but _don't_ alter it */
 Vstr_node *scan = NULL;
 Vstr_node *pos_scan = NULL;
 Vstr_node *pos_scan_next = NULL;
 
 assert(!(!base || !len || (pos > base->len) || base->conf->malloc_bad));
 if (!base || !len || (pos > base->len) || base->conf->malloc_bad)
   return (FALSE);
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 if (base->conf->spare_ptr_num < 1)
 {
  if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_PTR, 1) != 1)
    return (FALSE);
 }
 
 if (base->len)
 {
  scan = vstr__add_setup_pos(base, &pos, &num, NULL);
  if (!scan)
    return (FALSE);

  pos_scan = scan;
  pos_scan_next = scan->next;
 }
 
 scan = (Vstr_node *)base->conf->spare_ptr_beg;
 if (pos_scan)
 {
  assert(base->len);
  pos_scan->next = scan;
 }
 else
   base->beg = scan;

 num = 0;
 base->len += len;

 ++num;
 ++base->num;
  
 ((Vstr_node_ptr *)scan)->ptr = ptr;
 scan->len = len;

 if (!vstr__base_iovec_maybe_add_end(base, scan, len))
 {
  vstr__add_fail_cleanup(base, pos_scan, pos_scan_next,
                         num, len, pos_scan->len);
  return (FALSE);
 }

 base->conf->spare_ptr_beg = (Vstr_node_ptr *)scan->next;
 base->conf->spare_ptr_num -= num;
 
 assert(!scan || (scan->type == VSTR_TYPE_NODE_PTR));
 
 if (!(scan->next = pos_scan_next))
   base->end = scan;

 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 vstr__cache_add(base, orig_pos, orig_len);
 
 return (TRUE);
}

int vstr_add_non(Vstr_base *base, size_t pos, size_t len)
{
 unsigned int num = 0;
 size_t orig_pos = pos;
 size_t orig_len = len;
 Vstr_node *scan = NULL;
 Vstr_node *pos_scan = NULL;
 Vstr_node *pos_scan_next = NULL;
 
 assert(!(!base || !len || (pos > base->len) || base->conf->malloc_bad));
 if (!base || !len || (pos > base->len) || base->conf->malloc_bad)
   return (FALSE);
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 if (base->conf->spare_non_num < 1)
 {
  if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_NON, 1) != 1)
    return (FALSE);
 }

 if (base->len)
 {
  scan = vstr__add_setup_pos(base, &pos, &num, NULL);
  if (!scan)
    return (FALSE);

  if (scan->type == VSTR_TYPE_NODE_NON)
  {
   scan->len += len;
   base->len += len;
   vstr__base_iovec_add_node_end(base, num, len);
   return (TRUE);
  }
  
  pos_scan = scan;
  pos_scan_next = scan->next;
 }
 
 scan = (Vstr_node *)base->conf->spare_non_beg;
 if (base->end)
 {
  assert(base->len);
  base->end->next = scan;
 }
 else
   base->beg = scan;

 num = 0;
 base->len += len;

 ++num;
 ++base->num;

 scan->type = VSTR_TYPE_NODE_NON;
 scan->len = len;

 if (!vstr__base_iovec_maybe_add_end(base, scan, len))
 {
  vstr__add_fail_cleanup(base, pos_scan, pos_scan_next,
                         num, len, pos_scan->len);
  return (FALSE);
 }

 base->conf->spare_non_beg = (Vstr_node_non *)scan->next;
 base->conf->spare_non_num -= num;
 
 assert(!scan || (scan->type == VSTR_TYPE_NODE_NON));

 if (!(scan->next = pos_scan_next))
   base->end = scan;

 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 vstr__cache_add(base, orig_pos, orig_len);
 
 return (TRUE);
}

int vstr_add_ref(Vstr_base *base, size_t pos, size_t len,
                 Vstr_ref *ref, size_t off)
{
 unsigned int num = 0;
 size_t orig_pos = pos;
 size_t orig_len = len;
 Vstr_node *scan = NULL;
 Vstr_node *pos_scan = NULL;
 Vstr_node *pos_scan_next = NULL;

 assert(!(!base || !ref || !len || (pos > base->len) ||
          base->conf->malloc_bad));
 if (!base || !ref || !len || (pos > base->len) ||base->conf->malloc_bad)
   return (FALSE);
 
 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 if (base->conf->spare_ref_num < 1)
 {
  if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_REF, 1) != 1)
    return (FALSE);
 }
 
 if (base->len)
 {
  scan = vstr__add_setup_pos(base, &pos, &num, NULL);
  if (!scan)
    return (FALSE);
  
  pos_scan = scan;
  pos_scan_next = scan->next;
 }
  
 scan = (Vstr_node *)base->conf->spare_ref_beg;
 if (pos_scan)
 {
  assert(base->len);
  pos_scan->next = scan;
 }
 else
   base->beg = scan;

 num = 0;
 base->len += len;

 ++num;
 ++base->num;
  
 ((Vstr_node_ref *)scan)->ref = vstr_ref_add_ref(ref);
 ((Vstr_node_ref *)scan)->off = off;
 scan->len = len;

 if (!vstr__base_iovec_maybe_add_end(base, scan, len))
 {
  vstr__add_fail_cleanup(base, pos_scan, pos_scan_next,
                         num, len, pos_scan->len);
  return (FALSE);
 }

 base->conf->spare_ref_beg = (Vstr_node_ref *)scan->next;
 base->conf->spare_ref_num -= num;

 assert(!scan || (scan->type == VSTR_TYPE_NODE_REF));
 
 if (!(scan->next = pos_scan_next))
   base->end = scan;

 assert(vstr__check_spare_nodes(base->conf));
 assert(vstr__check_real_nodes(base));

 vstr__cache_add(base, orig_pos, orig_len);
 
 return (TRUE);
}

/* replace all buf nodes with ref nodes, we don't need to change the
 * vectors if they are there */
static int vstr__convert_buf_ref(Vstr_base *base, size_t pos, size_t len)
{
 Vstr_node **scan = &base->beg;
 Vstr_ref *ref = NULL;
 Vstr_node *ref_node = NULL;
 
 pos += base->used;
 --pos;
 
 while (*scan && (pos > (*scan)->len))
 {
  pos -= (*scan)->len;
  scan = &(*scan)->next;
 }
 
 while (*scan)
 {
  if ((*scan)->type == VSTR_TYPE_NODE_BUF)
  {
   if (base->conf->spare_ref_num < 1)
   {
    if (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_REF, 1) != 1)
      return (FALSE);
   }
   
   if (!(ref = malloc(sizeof(Vstr_ref))))
   {
    base->conf->malloc_bad = TRUE;
    base->conf->malloc_bad = TRUE;
    return (FALSE);
   }
   ref->func = vstr__ref_cb_free_bufnode_ref;
   ref->ptr = ((Vstr_node_buf *)(*scan))->buf;
   ref->ref = 0;
   
    ref_node = (Vstr_node *)base->conf->spare_ref_beg;
    base->conf->spare_ref_beg = (Vstr_node_ref *)ref_node->next;
    
    ref_node->len = (*scan)->len;
    ((Vstr_node_ref *)ref_node)->ref = vstr_ref_add_ref(ref);
    ((Vstr_node_ref *)ref_node)->off = 0;
    
    if (!(ref_node->next = (*scan)->next))
      base->end = ref_node;
    
    *scan = ref_node;
  }
  
  if (len <= (*scan)->len)
    break;
  len -= (*scan)->len;
  
  scan = &(*scan)->next;
 }

 return (TRUE);
}

static void vstr__add_cleanup_cache(Vstr_base *base,
                                    struct Vstr_cache_cstr *cpy)
{
 if (!base->cache)
   return;
 
  if (base->cache->cstr.ref)
    vstr_ref_del_ref(base->cache->cstr.ref);
  
  base->cache->cstr.pos = cpy->pos;
  base->cache->cstr.len = cpy->len;
  base->cache->cstr.ref = cpy->ref;
}

static int vstr__add_all_ref(Vstr_base *base, size_t pos, size_t len,
                             Vstr_base *from_base, size_t from_pos)
{
  struct Vstr_cache_cstr cpy = {0, 0, NULL};
  Vstr_ref *ref = NULL;

  if (from_base->cache && from_base->cache->cstr.ref)
  {
   cpy.pos = from_base->cache->cstr.pos;
   cpy.len = from_base->cache->cstr.len;
   cpy.ref = vstr_ref_add_ref(from_base->cache->cstr.ref);
  }
  
  if (!(ref = vstr_export_cstr_ref(from_base, from_pos, len)))
  {
   base->conf->malloc_bad = TRUE;
   goto add_all_ref_fail;
  }

  if (!vstr_add_ref(base, pos, len, ref, 0))   
    goto add_all_ref_fail;

  if (cpy.ref)
    vstr__add_cleanup_cache(from_base, &cpy);
  
  return (TRUE);

 add_all_ref_fail:

  vstr__add_cleanup_cache(from_base, &cpy);
  
  return (FALSE);
}

int vstr_add_vstr(Vstr_base *base, size_t pos, size_t len,
                  Vstr_base *from_base, size_t from_pos,
                  unsigned int type)
{
 size_t orig_pos = pos;
 size_t orig_from_pos = from_pos;
 size_t orig_len = len;
 size_t orig_base_len = base->len;
 Vstr_node *scan = NULL;
 size_t off = 0;
 unsigned int dummy_num = 0;

 assert(!(!base || !len || (pos > base->len) ||
          !from_base || (from_pos > from_base->len) || base->conf->malloc_bad));
 if (!base || !len || (pos > base->len) ||
     !from_base || (from_pos > from_base->len) || base->conf->malloc_bad)
   return (FALSE);

 /* quick short cut instead of using export_cstr_ref() also doesn't change
  * from_base in certain cases */
 if (type == VSTR_TYPE_ADD_ALL_REF)
 {
  if (!vstr__add_all_ref(base, pos, len, from_base, from_pos))
    return (FALSE);

  return (TRUE);
 }

 /* make sure there are no buf nodes */
 if (type == VSTR_TYPE_ADD_BUF_REF)
 {
  if (!vstr__convert_buf_ref(from_base, from_pos, len))
    return (FALSE);
 }
 
 /* do the real copying */
 if (!(scan = vstr__base_pos(from_base, &from_pos, &dummy_num, TRUE)))
   return (TRUE);

 off = from_pos - 1;
 while (len > 0)
 {
  size_t tmp = scan->len;

  tmp -= off;
  if (tmp > len)
    tmp = len;
  
  switch (scan->type)
  {
   case VSTR_TYPE_NODE_BUF:
    assert(type != VSTR_TYPE_ADD_BUF_REF); /* all bufs should now be refs */
    
    if (type == VSTR_TYPE_ADD_BUF_PTR)
    {
     if (!vstr_add_ptr(base, pos, tmp, ((Vstr_node_buf *)scan)->buf + off))
       goto fail;
     break;
    }
    if (!vstr_add_buf(base, pos, tmp, ((Vstr_node_buf *)scan)->buf + off))
      goto fail;
    break;
  case VSTR_TYPE_NODE_NON:
    if (!vstr_add_non(base, pos, tmp))
      goto fail;
    break;
  case VSTR_TYPE_NODE_PTR:
    if (!vstr_add_ptr(base, pos, tmp,
                      ((char *)((Vstr_node_ptr *)scan)->ptr) + off))
      goto fail;
    break;
  case VSTR_TYPE_NODE_REF:
    if (!vstr_add_ref(base, pos, tmp, ((Vstr_node_ref *)scan)->ref, off))
      goto fail;
    break;
   default:
     assert(FALSE);
     break;
  }

  pos += tmp;
  len -= tmp;
  
  off = 0;
  
  scan = scan->next;
 }

 vstr__cache_cpy(base, orig_pos, orig_len, from_base, orig_from_pos);
 
 return (TRUE);

 fail:
 /* must work as orig_pos must now be at the begining of a node */
 vstr_del(base, orig_pos, base->len - orig_base_len);
 assert(base->len == orig_len);
 return (FALSE);
}

size_t vstr_add_iovec_buf_beg(Vstr_base *base,
                              unsigned int min, unsigned int max,
                              struct iovec **ret_iovs,
                              unsigned int *num)
{
 unsigned int sz = max;
 struct iovec *iovs = NULL;
 size_t bytes = 0;
 Vstr_node *scan = NULL;
 
 if ((base->conf->spare_buf_num < min) &&
     (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, min)) != min)
   return (0);
 
 if (base->conf->spare_buf_num < sz)
   sz = base->conf->spare_buf_num;
 
 if (!vstr__base_iovec_alloc(base, base->num + sz))
   return (0);

 vstr__base_iovec_valid(base);
   
 iovs = base->vec.v + base->vec.off + base->num;
 
 *num = 0;

 if (base->end && (base->end->type == VSTR_TYPE_NODE_BUF) &&
     (base->conf->buf_sz > base->end->len))
 {
  if (sz < max)
    ++sz;
  
  assert(base->len);
  
  --iovs;
  iovs[0].iov_len = (base->conf->buf_sz - base->end->len);
  iovs[0].iov_base = (((Vstr_node_buf *)base->end)->buf + base->end->len);

  base->iovec_upto_date = FALSE;
  
  bytes = iovs[0].iov_len;
  *num = 1;
 }
 
 scan = (Vstr_node *)base->conf->spare_buf_beg;
 assert(scan);

 while (*num < sz)
 {
  assert(scan->type == VSTR_TYPE_NODE_BUF);
  
  iovs[*num].iov_len = base->conf->buf_sz;
  iovs[*num].iov_base = ((Vstr_node_buf *)scan)->buf;

  bytes += iovs[*num].iov_len;
  ++*num;
  
  scan = scan->next;
 }

 *ret_iovs = iovs;
 return (bytes);
}

void vstr_add_iovec_buf_end(Vstr_base *base, size_t bytes)
{
 struct iovec *iovs = NULL;
 unsigned int count = 0;
 Vstr_node *scan = NULL;

 vstr__cache_add(base, base->len, bytes);

 base->len += bytes;

 iovs = base->vec.v + base->vec.off + base->num;
 
 if (base->end && (base->end->type == VSTR_TYPE_NODE_BUF) &&
     (base->conf->buf_sz > base->end->len))
 {
  size_t first_iov_len = 0;

  --iovs;
  
  assert(base->len);
  assert((base->conf->buf_sz - base->end->len) == iovs[0].iov_len);
  assert((((Vstr_node_buf *)base->end)->buf + base->end->len) ==
         iovs[0].iov_base);

  first_iov_len = iovs[0].iov_len;
  if (first_iov_len > bytes)
    first_iov_len = bytes;
    
  assert(!base->iovec_upto_date);
  base->iovec_upto_date = TRUE;
  
  base->end->len += first_iov_len;
  
  vstr__base_iovec_reset_node(base, base->end, base->num);

  bytes -= first_iov_len;

  ++iovs;
 }
 assert(base->iovec_upto_date);

 if (!bytes)
   return;
 
 if (base->end)
   base->end->next = (Vstr_node *)base->conf->spare_buf_beg;
 else
   base->beg = (Vstr_node *)base->conf->spare_buf_beg;
 
 scan = (Vstr_node *)base->conf->spare_buf_beg;
 count = 0;
 while (bytes > 0)
 {
  Vstr_node *scan_next = scan->next;
  size_t tmp = iovs[count].iov_len;
     
  assert(scan);
  
  if (bytes < tmp)
    tmp = bytes;
  
  assert(((Vstr_node_buf *)scan)->buf == iovs[count].iov_base);
  assert((tmp == base->conf->buf_sz) || (tmp == bytes));
     
  scan->len = tmp;
  
  bytes -= tmp;
  
  if (!bytes)
  {
   base->end = scan;
   iovs[count].iov_len = tmp;
  }
     
  scan = scan_next;
  ++count;
 }
 assert(scan || (base->conf->spare_buf_num == count));
 
 base->num += count;
 base->conf->spare_buf_num -= count;
 
 assert(base->end);
 assert(base->len);
 
 base->end->next = NULL;
 
 base->conf->spare_buf_beg = (Vstr_node_buf *)scan;
}

