#define VSTR_C
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
/* master file, contains all the base functions */
#include "main.h"


Vstr_conf *vstr_make_conf(void)
{
 Vstr_conf *conf = malloc(sizeof(Vstr_conf));

 conf->spare_buf_num = 0;
 conf->spare_buf_beg = NULL;

 conf->spare_non_num = 0;
 conf->spare_non_beg = NULL;

 conf->spare_ptr_num = 0;
 conf->spare_ptr_beg = NULL;

 conf->spare_ref_num = 0;
 conf->spare_ref_beg = NULL;

 conf->iov_min_alloc = 0;
 conf->iov_min_offset = 0;

 conf->buf_sz = 512 - sizeof(Vstr_node_buf);

 conf->free_do = TRUE;
 conf->malloc_bad = FALSE;
 conf->iovec_auto_update = TRUE;
 conf->split_buf_del = FALSE;
 
 conf->ref = 0;
 
 return (conf);
}

void vstr__del_conf(Vstr_conf *conf)
{
 assert(conf->ref > 0);
 
 if (!--conf->ref)
 {
  vstr_del_spare_nodes(conf, VSTR_TYPE_NODE_BUF, conf->spare_buf_num);
  vstr_del_spare_nodes(conf, VSTR_TYPE_NODE_NON, conf->spare_non_num);
  vstr_del_spare_nodes(conf, VSTR_TYPE_NODE_PTR, conf->spare_ptr_num);
  vstr_del_spare_nodes(conf, VSTR_TYPE_NODE_REF, conf->spare_ref_num);
  if (conf->free_do)
    free(conf);
 }
}

void vstr__add_conf(Vstr_conf *conf)
{
 ++conf->ref;
}

void vstr__base_add_conf(Vstr_base *base, Vstr_conf *conf)
{
 base->conf = conf;
 vstr__add_conf(conf);
}

int vstr_init(void)
{
 if (!(vstr__options.def = vstr_make_conf()))
   return (FALSE);

 vstr__options.def->ref = 1;

 return (TRUE);
}

char *vstr__export_node_ptr(Vstr_node *node)
{
 switch (node->type)
 {
  case VSTR_TYPE_NODE_BUF:
    return (((Vstr_node_buf *)node)->buf);
  case VSTR_TYPE_NODE_NON:
    return (NULL);
  case VSTR_TYPE_NODE_PTR:
    return (((Vstr_node_ptr *)node)->ptr);
  case VSTR_TYPE_NODE_REF:
    return (((char *)((Vstr_node_ref *)node)->ref->ptr) +
            ((Vstr_node_ref *)node)->off);
    
  default:
    assert(FALSE);
    return (NULL);
 }
}

static void vstr__base_iovec_memmove(Vstr_base *base)
{
 Vstr_iovec *vec = &base->vec;

 memmove(vec->v + base->conf->iov_min_offset, vec->v + vec->off,
         sizeof(struct iovec) * base->num);
 vec->off = base->conf->iov_min_offset;
}

int vstr__base_iovec_alloc(Vstr_base *base, unsigned int sz)
{
 Vstr_iovec *vec = &base->vec;
 
 if (!sz)
   return (TRUE);
  
 if (!vec->v)
 {
  vec->v = malloc(sizeof(struct iovec) * sz);

  if (!vec->v)
    return (FALSE);

  vec->sz = sz;
  assert(!vec->off);
 }

 if ((vec->off > base->conf->iov_min_offset) &&
     (sz > (vec->sz - vec->off)))
   vstr__base_iovec_memmove(base);

 if (sz > (vec->sz - base->conf->iov_min_offset))
 {
  struct iovec *tmp = NULL;
  size_t alloc_sz = sz + base->conf->iov_min_offset + base->conf->iov_min_alloc;
 
  tmp = realloc(vec->v, sizeof(struct iovec) * alloc_sz);

  if (!tmp)
  {
   base->conf->malloc_bad = TRUE;
   return (FALSE);
  }
  
  vec->v = tmp;
  
  if (vec->off < base->conf->iov_min_offset)
    vstr__base_iovec_memmove(base);
  
  vec->sz = sz;
 }

 return (TRUE);
}

void vstr__base_iovec_reset_node(Vstr_base *base, Vstr_node *node,
                                 unsigned int num)
{
  struct iovec *iovs = NULL;
  
  if (!base->iovec_upto_date)
    return;
  
  iovs = base->vec.v + base->vec.off;
  iovs[num - 1].iov_len = node->len;
  iovs[num - 1].iov_base = vstr__export_node_ptr(node);
  
  if (num == 1)
  {
    char *tmp = NULL;
    
    iovs[num - 1].iov_len -= base->used;
    tmp = iovs[num - 1].iov_base;
    tmp += base->used;
    iovs[num - 1].iov_base = tmp;
  }
}

int vstr__base_iovec_valid(Vstr_base *base)
{
 unsigned int count = base->conf->iov_min_offset;
 Vstr_node *scan = NULL;
 
 if (base->iovec_upto_date || !base->beg)
   return (TRUE);

 if (!vstr__base_iovec_alloc(base, base->num))
   return (FALSE);

 assert(base->vec.off == base->conf->iov_min_offset);

 scan = base->beg;
 
 base->vec.v[count].iov_len = scan->len - base->used;
 if (scan->type == VSTR_TYPE_NODE_NON)
   base->vec.v[count].iov_base = NULL;
 else
   base->vec.v[count].iov_base = vstr__export_node_ptr(scan) + base->used;
 
 scan = scan->next;
 ++count;
 while (scan)
 {
  base->vec.v[count].iov_len = scan->len;
  base->vec.v[count].iov_base = vstr__export_node_ptr(scan);

  ++count;
  scan = scan->next;
 }

 base->iovec_upto_date = TRUE;

 return (TRUE);
}

#ifndef NDEBUG
static int vstr__base_iovec_check(Vstr_base *base)
{
 unsigned int count = base->vec.off;
 Vstr_node *scan = NULL;
 
 if (!base->iovec_upto_date || !base->beg)
   return (TRUE);

 scan = base->beg;

 assert(scan->len > base->used);
 assert(base->vec.v[count].iov_len == (size_t)(scan->len - base->used));
 if (scan->type == VSTR_TYPE_NODE_NON)
   assert(!base->vec.v[count].iov_base);
 else
   assert(base->vec.v[count].iov_base ==
          (vstr__export_node_ptr(scan) + base->used));
 
 scan = scan->next;
 ++count;
 while (scan)
 {
  assert(base->vec.v[count].iov_len == scan->len);
  assert(base->vec.v[count].iov_base == vstr__export_node_ptr(scan));

  ++count;
  scan = scan->next;
 }
 
 return (TRUE);
}
#endif

int vstr_init_base(Vstr_conf *conf, Vstr_base *base)
{
 if (!conf)
   conf = vstr__options.def;
 
 base->beg = NULL;
 base->end = NULL;

 base->len = 0;
 base->num = 0;

 base->vec.v = NULL;
 base->vec.off = 0;
 base->vec.sz = 0;

 base->cache = NULL;
 
 vstr__base_add_conf(base, conf);

 if (!vstr__base_iovec_alloc(base, base->conf->iov_min_alloc))
   return (FALSE);

 base->used = 0;
 base->free_do = FALSE;
 base->iovec_upto_date = TRUE;
 
 return (TRUE);
}

void vstr_free_base(Vstr_base *base)
{
 if (!base)
   return;

 if (base->len)
   vstr_del(base, 1, base->len);
 
 vstr__del_conf(base->conf);
 
 if (base->free_do)
   free(base);
}

Vstr_base *vstr_make_base(Vstr_conf *conf)
{
 Vstr_base *base = malloc(sizeof(Vstr_base));
  
 if (!base)
   return (NULL);
 
 if (!vstr_init_base(conf, base))
 {
  free(base);
  return (NULL);
 }
 base->free_do = TRUE;
 
 return (base);
}
    
Vstr_node *vstr__base_split_node(Vstr_base *base, Vstr_node *node, size_t pos)
{
 Vstr_node *beg = NULL;
 
 assert(base && pos && (pos <= node->len));
 
 switch (node->type)
 {
  case VSTR_TYPE_NODE_BUF:
    if ((base->conf->spare_buf_num < 1) &&
        (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, 1) != 1))
      return (NULL);
    
    --base->conf->spare_buf_num;
    beg = (Vstr_node *)base->conf->spare_buf_beg;
    base->conf->spare_buf_beg = (Vstr_node_buf *)beg->next;
    
    memcpy(((Vstr_node_buf *)beg)->buf,
           ((Vstr_node_buf *)node)->buf + pos, node->len - pos);
    break;
    
  case VSTR_TYPE_NODE_NON:
    if ((base->conf->spare_non_num < 1) &&
        (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_NON, 1) != 1))
      return (NULL);
    
    --base->conf->spare_non_num;
    beg = (Vstr_node *)base->conf->spare_non_beg;
    base->conf->spare_non_beg = (Vstr_node_non *)beg->next;
    break;
    
  case VSTR_TYPE_NODE_PTR:
    if ((base->conf->spare_ptr_num < 1) &&
        (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_PTR, 1) != 1))
      return (NULL);
    
    --base->conf->spare_ptr_num;
    beg = (Vstr_node *)base->conf->spare_ptr_beg;
    base->conf->spare_ptr_beg = (Vstr_node_ptr *)beg->next;
    
    ((Vstr_node_ptr *)beg)->ptr = ((char *)((Vstr_node_ptr *)node)->ptr) + pos;
    break;
    
  case VSTR_TYPE_NODE_REF:
  {
   Vstr_ref *ref = NULL;
   
   if ((base->conf->spare_ref_num < 1) &&
       (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_REF, 1) != 1))
      return (NULL);

   --base->conf->spare_ref_num;
   beg = (Vstr_node *)base->conf->spare_ref_beg;
   base->conf->spare_ref_beg = (Vstr_node_ref *)beg->next;
   
   ref = ((Vstr_node_ref *)node)->ref;
   ((Vstr_node_ref *)beg)->ref = vstr_ref_add_ref(ref);
   ((Vstr_node_ref *)beg)->off = ((Vstr_node_ref *)node)->off + pos;
  }
  break;

  default:
    assert(FALSE);
    return (NULL);
 }

 ++base->num;
 
 base->iovec_upto_date = FALSE;

 beg->len = node->len - pos;

 if (!(beg->next = node->next))
   base->end = beg;
 node->next = beg;
 
 node->len = pos;
 
 return (node);
}

static int vstr_add_spare_node(Vstr_conf *conf, unsigned int type)
{
  Vstr_node *node = NULL;
  
  switch (type)
  {
    case VSTR_TYPE_NODE_BUF:
      node = malloc(sizeof(Vstr_node_buf) + conf->buf_sz);
      break;
    case VSTR_TYPE_NODE_NON:
      node = malloc(sizeof(Vstr_node_non));
      break;
    case VSTR_TYPE_NODE_PTR:
      node = malloc(sizeof(Vstr_node_ptr));
      break;
    case VSTR_TYPE_NODE_REF:
      node = malloc(sizeof(Vstr_node_ref));
      break;
      
    default:
      return (FALSE);
  }
  
  if (!node)
  {
    conf->malloc_bad = TRUE;
    return (FALSE);
  }
  
  node->len = 0;
  node->type = type;
  
  switch (type)
  {
    case VSTR_TYPE_NODE_BUF:
      node->next = (Vstr_node *)conf->spare_buf_beg;
      conf->spare_buf_beg = (Vstr_node_buf *)node;
      ++conf->spare_buf_num;
      break;
      
    case VSTR_TYPE_NODE_NON:
      node->next = (Vstr_node *)conf->spare_non_beg;
      conf->spare_non_beg = (Vstr_node_non *)node;
      ++conf->spare_non_num;
      break;
      
    case VSTR_TYPE_NODE_PTR:
      node->next = (Vstr_node *)conf->spare_ptr_beg;
      conf->spare_ptr_beg = (Vstr_node_ptr *)node;
      ++conf->spare_ptr_num;
      break;
      
    case VSTR_TYPE_NODE_REF:
      node->next = (Vstr_node *)conf->spare_ref_beg;
      conf->spare_ref_beg = (Vstr_node_ref *)node;
      ++conf->spare_ref_num;
      break;
  }
  
  return (TRUE);
}

unsigned int vstr_add_spare_nodes(Vstr_conf *conf, unsigned int type,
                                  unsigned int num)
{
  unsigned int count = 0;
  
  assert(vstr__check_spare_nodes(conf));
  
  while (count < num)
  {
    if (!vstr_add_spare_node(conf, type))
    {
      assert(vstr__check_spare_nodes(conf));
      
      return (count);
    }
    
    ++count;
  }
  assert(count == num);
  
  assert(vstr__check_spare_nodes(conf));
  
  return (count);
}

static int vstr_del_spare_node(Vstr_conf *conf, unsigned int type)
{
 Vstr_node *scan = NULL;

 switch (type)
 {
  case VSTR_TYPE_NODE_BUF:
    if (!conf->spare_buf_beg)
      return (FALSE);

    scan = (Vstr_node *)conf->spare_buf_beg;
    
    assert(scan->type == type);
    
    conf->spare_buf_beg = (Vstr_node_buf *)scan->next;
    --conf->spare_buf_num;
    free(scan);
    break;
    
  case VSTR_TYPE_NODE_NON:
    if (!conf->spare_non_beg)
      return (FALSE);

    scan = (Vstr_node *)conf->spare_non_beg;
    
    assert(scan->type == VSTR_TYPE_NODE_NON);
    
    conf->spare_non_beg = (Vstr_node_non *)scan->next;
    --conf->spare_non_num;
    free(scan);
    break;
    
  case VSTR_TYPE_NODE_PTR:
    if (!conf->spare_ptr_beg)
      return (FALSE);

    scan = (Vstr_node *)conf->spare_ptr_beg;
    
    assert(scan->type == VSTR_TYPE_NODE_PTR);
    
    conf->spare_ptr_beg = (Vstr_node_ptr *)scan->next;
    --conf->spare_ptr_num;
    free(scan);
    break;
    
  case VSTR_TYPE_NODE_REF:
    if (!conf->spare_ref_beg)
      return (FALSE);

    scan = (Vstr_node *)conf->spare_ref_beg;

    assert(scan->type == type);
    
    conf->spare_ref_beg = (Vstr_node_ref *)scan->next;
    --conf->spare_ref_num;
    free(scan);
    break;

  default:
    return (FALSE);
 }

 return (TRUE);
}

unsigned int vstr_del_spare_nodes(Vstr_conf *conf, unsigned int type,
                                  unsigned int num)
{
  unsigned int count = 0;
  
  assert(vstr__check_spare_nodes(conf));
  
  while (count < num)
  {
    if (!vstr_del_spare_node(conf, type))
      return (count);
    
    ++count;
  }
  assert(count == num);

  assert(vstr__check_spare_nodes(conf));
  
  return (count);
}

#ifndef NDEBUG
int vstr__check_real_nodes(Vstr_base *base)
{
 size_t len = 0;
 size_t num = 0;
 Vstr_node *scan = base->beg;
 
 while (scan)
 {
  len += scan->len;
  
  ++num;
  
  scan = scan->next;
 }

 assert(!base->used || (base->used < base->beg->len));
 assert(((len - base->used) == base->len) && (num == base->num));

 vstr__base_iovec_check(base);
 
 return (TRUE);
}

int vstr__check_spare_nodes(Vstr_conf *conf)
{
 unsigned int num = 0;
 Vstr_node *scan = NULL;

 num = 0;
 scan = (Vstr_node *)conf->spare_buf_beg;
 while (scan)
 {
  ++num;

  assert(scan->type == VSTR_TYPE_NODE_BUF);
  
  scan = scan->next;
 }
 assert(conf->spare_buf_num == num);

 num = 0;
 scan = (Vstr_node *)conf->spare_non_beg;
 while (scan)
 {
  ++num;

  assert(scan->type == VSTR_TYPE_NODE_NON);
  
  scan = scan->next;
 }
 assert(conf->spare_non_num == num);
 
 num = 0;
 scan = (Vstr_node *)conf->spare_ptr_beg;
 while (scan)
 {
  ++num;

  assert(scan->type == VSTR_TYPE_NODE_PTR);
  
  scan = scan->next;
 }
 assert(conf->spare_ptr_num == num);
 
 num = 0;
 scan = (Vstr_node *)conf->spare_ref_beg;
 while (scan)
 {
  ++num;

  assert(scan->type == VSTR_TYPE_NODE_REF);
  
  scan = scan->next;
 }
 assert(conf->spare_ref_num == num);
 
 return (TRUE);
}
#endif

static int vstr__base_cache_pos(const Vstr_base *base,
                                Vstr_node *node, size_t pos, unsigned int num)
{
 if (!base->cache)
 {
  if (!vstr__make_cache((Vstr_base *)base))
    return (FALSE);
 }

 base->cache->pos.node = node;
 base->cache->pos.pos = pos;
 base->cache->pos.num = num;
 
 return (TRUE);
}

Vstr_node *vstr__base_pos(const Vstr_base *base, size_t *pos,
                          unsigned int *num, int cache)
{
 size_t orig_pos = *pos;
 Vstr_node *scan = base->beg;

 *pos += base->used;
 *num = 1;
 
 if (base->beg == base->end)
   return (base->beg);

 /* must be more than one node */
 
 if (orig_pos > (base->len - base->end->len))
 {
  *pos = orig_pos - (base->len - base->end->len);
  *num = base->num;
  return (base->end);
 }

 if (base->cache && base->cache->pos.node && (base->cache->pos.pos <= orig_pos))
 {
  scan = base->cache->pos.node;
  *num = base->cache->pos.num;
  *pos = (orig_pos - base->cache->pos.pos) + 1;
 }
 
 while (*pos > scan->len)
 {
  *pos -= scan->len;
  
  assert(scan->next);
  scan = scan->next;
  ++*num;
 }

 if (cache && (*num > 1))
   vstr__base_cache_pos(base, scan, (orig_pos - *pos) + 1, *num);
 
 return (scan);
}

Vstr_node *vstr__base_scan_fwd_beg(const Vstr_base *base,
                                   size_t pos, size_t *len,
                                   unsigned int *num, 
                                   char **scan_str, size_t *scan_len)
{
 Vstr_node *scan = NULL;
 
 assert(base && pos && *len && ((pos + *len - 1) <= base->len));
 
 if ((pos > base->len) || !*len)
   return (NULL);
 
 if ((pos + *len - 1) > base->len)
   *len = base->len - (pos - 1);
 
 scan = vstr__base_pos(base, &pos, num, TRUE);
 assert(scan);

 --pos;
 
 *scan_len = scan->len - pos;
 if (*scan_len > *len)
   *scan_len = *len;
 *len -= *scan_len;

 *scan_str = NULL;
 if (scan->type != VSTR_TYPE_NODE_NON)
   *scan_str = vstr__export_node_ptr(scan) + pos;
 
 return (scan);
}

Vstr_node *vstr__base_scan_fwd_nxt(const Vstr_base *base, size_t *len,
                                   unsigned int *num, Vstr_node *scan,
                                   char **scan_str, size_t *scan_len)
{
 assert(scan);

 (void)base; /* not needed */
 
 if (!*len || !scan || !(scan = scan->next))
   return (NULL); 
 ++*num;
 
 *scan_len = scan->len;
 
 if (*scan_len > *len)
   *scan_len = *len; 
 *len -= *scan_len;

 *scan_str = NULL;
 if (scan->type != VSTR_TYPE_NODE_NON)
   *scan_str = vstr__export_node_ptr(scan);

 return (scan);
}


int vstr__base_scan_rev_beg(const Vstr_base *base,
                            size_t pos, size_t *len,
                            unsigned int *num, 
                            char **scan_str, size_t *scan_len)
{
  Vstr_node *scan = NULL;
  
  assert(base && pos && *len && ((pos + *len - 1) <= base->len));
  
  if ((pos > base->len) || !*len)
    return (FALSE);
  
  if ((pos + *len - 1) > base->len)
    *len = base->len - (pos - 1);
  
  pos += *len - 1; /* pos at end of string */
  scan = vstr__base_pos(base, &pos, num, TRUE);
  assert(scan);
  
  --pos;
  
  *scan_len = scan->len - pos;
  if (*scan_len > *len)
    *scan_len = *len;
  *len -= *scan_len;
  
  *scan_str = NULL;
  if (scan->type != VSTR_TYPE_NODE_NON)
    *scan_str = vstr__export_node_ptr(scan) + pos;
  
  return (TRUE);
}

int vstr__base_scan_rev_nxt(const Vstr_base *base, size_t *len,
                            unsigned int *num, 
                            char **scan_str, size_t *scan_len)
{
  struct iovec *iovs = NULL;
  size_t pos = 0;
  
  assert(num);
  
  if (!*len || !--*num)
    return (FALSE);

  iovs = base->vec.v + base->vec.off;
  *scan_len = iovs[*num - 1].iov_len;
  
  if (*scan_len > *len)
  {
    pos = *scan_len - *len;
    *scan_len = *len;
  }
  *len -= *scan_len;
  
  *scan_str = NULL;
  if (iovs[*num - 1].iov_base)
    *scan_str = ((char *) (iovs[*num - 1].iov_base)) + pos;
  
  return (TRUE);
}
