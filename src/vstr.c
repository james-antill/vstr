#define VSTR_C
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
/* master file, contains most of the base functions */
#include "main.h"


unsigned int vstr__num_node(Vstr_base *base, Vstr_node *node)
{
  unsigned int num = 0;
  Vstr_node *scan = base->beg;
  
  while (scan)
  {
    ++num;
    
    if (scan == node)
      return (num);
    
    scan = scan->next;
  }
  assert(FALSE);

  return (0);
}

size_t vstr__loc_thou_grp_strlen(const char *passed_str)
{
  const unsigned char *str = (const unsigned char *)passed_str;
  size_t len = 0;
  
  while (*str && (*str < SCHAR_MAX))
  {
    ++len;
    ++str;
  }

  if (*str)
    ++len;
  
  return (len);
}

static int vstr__make_conf_loc_def_numeric(Vstr_locale *loc)
{
  if (!(loc->name_lc_numeric_str = calloc(2, 1)))
    goto fail_numeric;
  
  if (!(loc->grouping = calloc(1, 1)))
    goto fail_grp;
  
  if (!(loc->thousands_sep_str = calloc(1, 1)))
    goto fail_thou;
  
  if (!(loc->decimal_point_str = calloc(2, 1)))
    goto fail_deci;
  
  *loc->name_lc_numeric_str = 'C'; 
  loc->name_lc_numeric_len = 1;

  *loc->decimal_point_str = '.'; 
  loc->decimal_point_len = 1;
  
  loc->thousands_sep_len = 0;

  return (TRUE);
  
 fail_deci:
  free(loc->thousands_sep_str);
 fail_thou:
  free(loc->grouping);
 fail_grp:
  free(loc->name_lc_numeric_str);
 fail_numeric:
  
  return (FALSE);
}

#ifdef USE_RESTRICTED_HEADERS /* always use C locale */
# define setlocale(x, y) NULL
# define localeconv()    NULL
# define SYS_LOC(x) ""
#else
# define SYS_LOC(x) ((sys_loc)->x)
#endif

int vstr__make_conf_loc_numeric(Vstr_conf *conf, const char *name)
{
  struct lconv *sys_loc = NULL;
  const char *tmp = NULL;
  Vstr_locale loc[1];

  if (name)
    tmp = setlocale(LC_NUMERIC, name);
  
  if (!(sys_loc = localeconv()))
  {
    if (!vstr__make_conf_loc_def_numeric(loc))
      goto fail_numeric;
  }
  else
  {
    const char *name_numeric = NULL;
    size_t numeric_len = 0;
    size_t grp_len  = vstr__loc_thou_grp_strlen(SYS_LOC(grouping));
    size_t thou_len =                    strlen(SYS_LOC(thousands_sep));
    size_t deci_len =                    strlen(SYS_LOC(decimal_point));
    
    if (!(name_numeric = setlocale(LC_NUMERIC, NULL)))
      name_numeric = "C";
    numeric_len = strlen(name_numeric);
    
    if (!(loc->name_lc_numeric_str = malloc(numeric_len + 1)))
      goto fail_numeric;
    
    if (!(loc->grouping = malloc(grp_len + 1)))
      goto fail_grp;

    if (!(loc->thousands_sep_str = malloc(thou_len + 1)))
      goto fail_thou;

    if (!(loc->decimal_point_str = malloc(deci_len + 1)))
      goto fail_deci;

    memcpy(loc->name_lc_numeric_str, name_numeric, numeric_len + 1);
    loc->name_lc_numeric_len = numeric_len;
    
    memcpy(loc->grouping, SYS_LOC(grouping), grp_len);
    loc->grouping[grp_len] = 0; /* make sure all cstrs are 0 terminated */
    
    memcpy(loc->thousands_sep_str, SYS_LOC(thousands_sep), thou_len + 1);
    loc->thousands_sep_len = thou_len;
    
    memcpy(loc->decimal_point_str, SYS_LOC(decimal_point), deci_len + 1);
    loc->decimal_point_len = deci_len;
  }

  free(conf->loc->name_lc_numeric_str);
  conf->loc->name_lc_numeric_str = loc->name_lc_numeric_str;
  conf->loc->name_lc_numeric_len = loc->name_lc_numeric_len;

  free(conf->loc->grouping);
  conf->loc->grouping = loc->grouping;
  
  free(conf->loc->thousands_sep_str);
  conf->loc->thousands_sep_str = loc->thousands_sep_str;
  conf->loc->thousands_sep_len = loc->thousands_sep_len;
  
  free(conf->loc->decimal_point_str);
  conf->loc->decimal_point_str = loc->decimal_point_str;
  conf->loc->decimal_point_len = loc->decimal_point_len;

  if (tmp)
    setlocale(LC_NUMERIC, tmp);

  return (TRUE);
  
 fail_deci:
  free(loc->thousands_sep_str);
 fail_thou:
  free(loc->grouping);
 fail_grp:
  free(loc->name_lc_numeric_str);
 fail_numeric:

  if (tmp)
    setlocale(LC_NUMERIC, tmp);
  
  return (FALSE);
}

Vstr_conf *vstr_nx_make_conf(void)
{
  Vstr_conf *conf = malloc(sizeof(Vstr_conf));
  
  if (!conf)
    goto fail_conf;

  if (!vstr__cache_conf_init(conf))
    goto fail_cache;
  
  if (!(conf->loc = malloc(sizeof(Vstr_locale))))
    goto fail_loc;

  conf->loc->name_lc_numeric_str = NULL;
  conf->loc->grouping = NULL;
  conf->loc->thousands_sep_str = NULL;
  conf->loc->decimal_point_str = NULL;
  
  if (!vstr__make_conf_loc_def_numeric(conf->loc))
    goto fail_numeric;
  
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
  
  conf->buf_sz = 64 - sizeof(Vstr_node_buf);

  conf->fmt_usr_names = 0;
  conf->fmt_usr_escape = 0;
  
  conf->ref = 1;

  conf->ref_link = NULL;
  
  conf->free_do = TRUE;
  conf->malloc_bad = FALSE;
  conf->iovec_auto_update = TRUE;
  conf->split_buf_del = FALSE;

  conf->user_ref = 1;

  conf->no_cache = FALSE;
  
  return (conf);

 fail_numeric:
  free(conf->loc);
 fail_loc:
  free(conf->cache_cbs_ents);
 fail_cache:
  free(conf);
 fail_conf:

  return (NULL);
}

static void vstr__add_conf(Vstr_conf *conf)
{
  ++conf->ref;
}

void vstr__add_user_conf(Vstr_conf *conf)
{
  assert(conf);
  assert(conf->user_ref <= conf->ref);
  
  ++conf->user_ref;

  vstr__add_conf(conf);
}

/* NOTE: magic also exists in vstr_add..c (vstr__convert_buf_ref_add)
 * for linked references */

void vstr__add_base_conf(Vstr_base *base, Vstr_conf *conf)
{
  assert(conf->user_ref <= conf->ref);
  
  base->conf = conf;
  vstr__add_conf(conf);
}

void vstr__del_conf(Vstr_conf *conf)
{
 assert(conf->ref > 0);
 
 if (!--conf->ref)
 {
   assert(!conf->ref_link);
   
  vstr_nx_free_spare_nodes(conf, VSTR_TYPE_NODE_BUF, conf->spare_buf_num);
  vstr_nx_free_spare_nodes(conf, VSTR_TYPE_NODE_NON, conf->spare_non_num);
  vstr_nx_free_spare_nodes(conf, VSTR_TYPE_NODE_PTR, conf->spare_ptr_num);
  vstr_nx_free_spare_nodes(conf, VSTR_TYPE_NODE_REF, conf->spare_ref_num);
  
  free(conf->loc->name_lc_numeric_str);
  free(conf->loc->grouping);
  free(conf->loc->thousands_sep_str);
  free(conf->loc->decimal_point_str);
  free(conf->loc);

  free(conf->cache_cbs_ents);

  while (conf->fmt_usr_names)
  {
    Vstr__fmt_usr_name_node *tmp = conf->fmt_usr_names;
      
    vstr_nx_fmt_del(conf, tmp->name_str);
  }
  
  if (conf->free_do)
    free(conf);
 }
}

int vstr_nx_swap_conf(Vstr_base *base, Vstr_conf **conf)
{
  assert(conf && *conf);
  assert((*conf)->user_ref <= (*conf)->ref);

  if (base->conf == *conf)
    return (TRUE);
  
  if (base->conf->buf_sz != (*conf)->buf_sz)
  {
    if ((*conf)->user_ref >= (*conf)->ref)
      return (FALSE);
    
    vstr_nx_free_spare_nodes(*conf, VSTR_TYPE_NODE_BUF, (*conf)->spare_buf_num);
    (*conf)->buf_sz = base->conf->buf_sz;
  }
  
  if (!vstr__cache_subset_cbs(*conf, base->conf))
  {
    if ((*conf)->user_ref >= (*conf)->ref)
      return (FALSE);
    
    if (!vstr__cache_dup_cbs(*conf, base->conf))
      return (FALSE);
  }

  --(*conf)->user_ref;
  ++base->conf->user_ref;
  
  SWAP_TYPE(*conf, base->conf, Vstr_conf *);

  return (TRUE);
}

void vstr_nx_free_conf(Vstr_conf *conf)
{
  if (!conf)
    return;
  
  assert(conf->user_ref > 0);
  assert(conf->user_ref <= conf->ref);
  
  --conf->user_ref;
  
  vstr__del_conf(conf);
}

int vstr_nx_init(void)
{
  if (!vstr__options.def)
  {
    if (!(vstr__options.def = vstr_nx_make_conf()))
      return (FALSE);
    vstr__options.mmap_count = 0;
  }
  
  return (TRUE);
}

void vstr_nx_exit(void)
{
  assert(!vstr__options.mmap_count);

  vstr__add_fmt_cleanup_spec();

  assert((vstr__options.def->user_ref == 1) &&
         (vstr__options.def->ref == 1));
  
  vstr_nx_free_conf(vstr__options.def);
  vstr__options.def = NULL;
}

char *vstr__export_node_ptr(const Vstr_node *node)
{
 switch (node->type)
 {
  case VSTR_TYPE_NODE_BUF:
    return (((Vstr_node_buf *)node)->buf);
  case VSTR_TYPE_NODE_NON:
    return (NULL);
  case VSTR_TYPE_NODE_PTR:
    return (((const Vstr_node_ptr *)node)->ptr);
  case VSTR_TYPE_NODE_REF:
    return (((char *)((const Vstr_node_ref *)node)->ref->ptr) +
            ((const Vstr_node_ref *)node)->off);
    
  default:
    assert(FALSE);
    return (NULL);
 }
}

void vstr__cache_iovec_reset_node(const Vstr_base *base, Vstr_node *node,
                                  unsigned int num)
{
  struct iovec *iovs = NULL;
  unsigned char *types = NULL;
  
  if (!base->iovec_upto_date)
    return;
  
  iovs = VSTR__CACHE(base)->vec->v + VSTR__CACHE(base)->vec->off;
  iovs[num - 1].iov_len = node->len;
  iovs[num - 1].iov_base = vstr__export_node_ptr(node);

  types = VSTR__CACHE(base)->vec->t + VSTR__CACHE(base)->vec->off;
  types[num - 1] = node->type;
  
  if (num == 1)
  {
    char *tmp = NULL;
    
    iovs[num - 1].iov_len -= base->used;
    tmp = iovs[num - 1].iov_base;
    tmp += base->used;
    iovs[num - 1].iov_base = tmp;
  }
}

int vstr__cache_iovec_valid(Vstr_base *base)
{
 unsigned int count = 0;
 Vstr_node *scan = NULL;

 if (base->iovec_upto_date)
   return (TRUE);

 if (!base->beg)
 {
   if (base->cache_available && VSTR__CACHE(base) && VSTR__CACHE(base)->vec &&
       VSTR__CACHE(base)->vec->sz)
     base->iovec_upto_date = TRUE;
   
   return (TRUE);
 }
 
 if (!vstr__cache_iovec_alloc(base, base->num))
   return (FALSE);

 assert(VSTR__CACHE(base)->vec->off == base->conf->iov_min_offset);

 count = base->conf->iov_min_offset;
 scan = base->beg;
 
 VSTR__CACHE(base)->vec->v[count].iov_len = scan->len - base->used;
 if (scan->type == VSTR_TYPE_NODE_NON)
   VSTR__CACHE(base)->vec->v[count].iov_base = NULL;
 else
   VSTR__CACHE(base)->vec->v[count].iov_base = (vstr__export_node_ptr(scan) +
                                          base->used);
 VSTR__CACHE(base)->vec->t[count] = scan->type;
   
 scan = scan->next;
 ++count;
 while (scan)
 {
  VSTR__CACHE(base)->vec->t[count] = scan->type;
  VSTR__CACHE(base)->vec->v[count].iov_len = scan->len;
  VSTR__CACHE(base)->vec->v[count].iov_base = vstr__export_node_ptr(scan);
  
  ++count;
  scan = scan->next;
 }

 base->iovec_upto_date = TRUE;

 return (TRUE);
}

#ifndef NDEBUG
static int vstr__cache_iovec_check(Vstr_base *base)
{
 unsigned int count = 0;
 Vstr_node *scan = NULL;
 
 if (!base->iovec_upto_date || !base->beg)
   return (TRUE);

 count = VSTR__CACHE(base)->vec->off;
 scan = base->beg;

 assert(scan->len > base->used);
 assert(VSTR__CACHE(base)->vec->sz >= base->num);
 assert(VSTR__CACHE(base)->vec->v[count].iov_len == (size_t)(scan->len - base->used));
 if (scan->type == VSTR_TYPE_NODE_NON)
   assert(!VSTR__CACHE(base)->vec->v[count].iov_base);
 else
   assert(VSTR__CACHE(base)->vec->v[count].iov_base ==
          (vstr__export_node_ptr(scan) + base->used));
 assert(VSTR__CACHE(base)->vec->t[count] == scan->type);
 
 if (!(scan = scan->next))
   assert(base->beg == base->end);
 
 ++count;
 while (scan)
 {
  assert(VSTR__CACHE(base)->vec->t[count] == scan->type);
  assert(VSTR__CACHE(base)->vec->v[count].iov_len == scan->len);
  assert(VSTR__CACHE(base)->vec->v[count].iov_base == vstr__export_node_ptr(scan));

  ++count;

  if (!scan->next)
    assert(scan == base->end);
  
  scan = scan->next;
 }
 
 return (TRUE);
}

static int vstr__cache_cstr_check(Vstr_base *base)
{
  Vstr__cache_data_cstr *data = NULL;
  
  if (!(data = vstr_nx_cache_get(base, base->conf->cache_pos_cb_cstr)))
    return (TRUE);
  if (!data->ref)
    return (TRUE);

  return (VSTR_CMP_BUF_EQ(base, data->pos, data->len,
                          data->ref->ptr,  data->len));
}
#endif

static int vstr__init_base(Vstr_conf *conf, Vstr_base *base)
{
 if (!conf)
   conf = vstr__options.def;
 
 base->beg = NULL;
 base->end = NULL;

 base->len = 0;
 base->num = 0;

 if (conf->no_cache)
    base->cache_available = FALSE;
 else
 {
   VSTR__CACHE(base) = NULL;
   base->cache_available = TRUE;
 }
 base->cache_internal = TRUE;
 
 vstr__add_base_conf(base, conf);

 base->used = 0;
 base->free_do = FALSE;
 base->iovec_upto_date = FALSE;
 
 base->node_buf_used = FALSE;
 base->node_non_used = FALSE;
 base->node_ptr_used = FALSE;
 base->node_ref_used = FALSE;
 
 if (base->cache_available)
 {  
   if (!vstr__cache_iovec_alloc(base, 0))
     return (FALSE);
   
   if (VSTR__CACHE(base) &&
       VSTR__CACHE(base)->vec && VSTR__CACHE(base)->vec->sz)
     base->iovec_upto_date = TRUE;
 }

 return (TRUE);
}

void vstr_nx_free_base(Vstr_base *base)
{
 if (!base)
   return;

 vstr_nx_del(base, 1, base->len);

 vstr__free_cache(base);
 
 vstr__del_conf(base->conf);
 
 if (base->free_do)
   free(base);
}

Vstr_base *vstr_nx_make_base(Vstr_conf *conf)
{
 Vstr_base *base = NULL;

 if (!conf)
   conf = vstr__options.def;

 base = malloc(conf->no_cache ? sizeof(Vstr_base) : sizeof(Vstr__base_cache));
 
 if (!base)
 {
   conf->malloc_bad = TRUE;
   return (NULL);
 }
 
 if (!vstr__init_base(conf, base))
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
        (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, 1) != 1))
      return (NULL);
    
    --base->conf->spare_buf_num;
    beg = (Vstr_node *)base->conf->spare_buf_beg;
    base->conf->spare_buf_beg = (Vstr_node_buf *)beg->next;
    
    memcpy(((Vstr_node_buf *)beg)->buf,
           ((Vstr_node_buf *)node)->buf + pos, node->len - pos);
    break;
    
  case VSTR_TYPE_NODE_NON:
    if ((base->conf->spare_non_num < 1) &&
        (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_NON, 1) != 1))
      return (NULL);
    
    --base->conf->spare_non_num;
    beg = (Vstr_node *)base->conf->spare_non_beg;
    base->conf->spare_non_beg = (Vstr_node_non *)beg->next;
    break;
    
  case VSTR_TYPE_NODE_PTR:
    if ((base->conf->spare_ptr_num < 1) &&
        (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_PTR, 1) != 1))
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
       (vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_REF, 1) != 1))
      return (NULL);

   --base->conf->spare_ref_num;
   beg = (Vstr_node *)base->conf->spare_ref_beg;
   base->conf->spare_ref_beg = (Vstr_node_ref *)beg->next;
   
   ref = ((Vstr_node_ref *)node)->ref;
   ((Vstr_node_ref *)beg)->ref = vstr_nx_ref_add(ref);
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

static int vstr__make_spare_node(Vstr_conf *conf, unsigned int type)
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

unsigned int vstr_nx_make_spare_nodes(Vstr_conf *conf, unsigned int type,
                                      unsigned int num)
{
  unsigned int count = 0;
  
  assert(vstr__check_spare_nodes(conf));
  
  while (count < num)
  {
    if (!vstr__make_spare_node(conf, type))
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

static int vstr__free_spare_node(Vstr_conf *conf, unsigned int type)
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

unsigned int vstr_nx_free_spare_nodes(Vstr_conf *conf, unsigned int type,
                                      unsigned int num)
{
  unsigned int count = 0;
  
  assert(vstr__check_spare_nodes(conf));
  
  while (count < num)
  {
    if (!vstr__free_spare_node(conf, type))
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
 unsigned int node_buf_used = FALSE;
 unsigned int node_non_used = FALSE;
 unsigned int node_ptr_used = FALSE;
 unsigned int node_ref_used = FALSE; 
 Vstr_node *scan = base->beg;
 
 assert(!base->used || (base->used < base->beg->len));
 assert(!base->used || (base->beg->type == VSTR_TYPE_NODE_BUF));
 
 while (scan)
 {
   switch (scan->type)
   {
     case VSTR_TYPE_NODE_BUF: node_buf_used = TRUE; break;
     case VSTR_TYPE_NODE_NON: node_non_used = TRUE; break;
     case VSTR_TYPE_NODE_PTR: node_ptr_used = TRUE; break;
     case VSTR_TYPE_NODE_REF: node_ref_used = TRUE; break;
     default:
       assert(FALSE);
       break;
   }
   
  len += scan->len;
  
  ++num;
  
  scan = scan->next;
 }

 /* it can be TRUE in the base and FALSE in reallity */
 assert(!node_buf_used || base->node_buf_used);
 assert(!node_non_used || base->node_non_used);
 assert(!node_ptr_used || base->node_ptr_used);
 assert(!node_ref_used || base->node_ref_used);
 
 assert(!!base->beg == !!base->end);
 assert(((len - base->used) == base->len) && (num == base->num));

 vstr__cache_iovec_check(base);
 vstr__cache_cstr_check(base);
 
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
  Vstr__cache_data_pos *data = NULL;
  
  if (!base->cache_available)
    return (FALSE);

  if (!(data = vstr_nx_cache_get(base, base->conf->cache_pos_cb_pos)))
  {
    if (!(data = malloc(sizeof(Vstr__cache_data_pos))))
      return (FALSE);

    if (!vstr_nx_cache_set(base, base->conf->cache_pos_cb_pos, data))
    {
      free(data);
      return (FALSE);
    }
  }
  
  data->node = node;
  data->pos = pos;
  data->num = num;
  
  return (TRUE);
}

/* zero ->used and normalise first node */
void vstr__base_zero_used(Vstr_base *base)
{
  if (base->used)
  {
    assert(base->beg->type == VSTR_TYPE_NODE_BUF);
    base->beg->len -= base->used;
    memmove(((Vstr_node_buf *)base->beg)->buf,
            ((Vstr_node_buf *)base->beg)->buf + base->used,
            base->beg->len);
    base->used = 0;
  }
}

Vstr_node *vstr__base_pos(const Vstr_base *base, size_t *pos,
                          unsigned int *num, int cache)
{
 size_t orig_pos = *pos;
 Vstr_node *scan = base->beg;
 Vstr__cache_data_pos *data = NULL;
 
 *pos += base->used;
 *num = 1;

 if (*pos <= base->beg->len)
 {
   return (base->beg);
 }
 
 /* must be more than one node */
 
 if (orig_pos > (base->len - base->end->len))
 {
  *pos = orig_pos - (base->len - base->end->len);
  *num = base->num;
  return (base->end);
 }

 if ((data = vstr_nx_cache_get(base, base->conf->cache_pos_cb_pos)) &&
     data->node && (data->pos <= orig_pos))
 {
  scan = data->node;
  *num = data->num;
  *pos = (orig_pos - data->pos) + 1;
 }

 while (*pos > scan->len)
 {
  *pos -= scan->len;
  
  assert(scan->next);
  scan = scan->next;
  ++*num;
 }

 if (cache)
   vstr__base_cache_pos(base, scan, (orig_pos - *pos) + 1, *num);
 
 return (scan);
}

Vstr_node *vstr__base_scan_fwd_beg(const Vstr_base *base,
                                   size_t pos, size_t *len,
                                   unsigned int *num, 
                                   char **scan_str, size_t *scan_len)
{
  Vstr_node *scan = NULL;
  
  assert(base && num && len);
  
  assert(pos && (((pos <= base->len) &&
                  ((pos + *len - 1) <= base->len)) || !*len));
  
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
  assert(base && num && len);
  assert(scan);
  
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
                            unsigned int *num, unsigned int *type,
                            char **scan_str, size_t *scan_len)
{
  Vstr_node *scan_beg = NULL;
  Vstr_node *scan_end = NULL;
  unsigned int dummy_num = 0;
  size_t end_pos = 0;
  
  assert(base && num && len && type);
  
  assert(*len && ((pos + *len - 1) <= base->len));
  
  assert(base->iovec_upto_date);
  
  if ((pos > base->len) || !*len)
    return (FALSE);
  
  if ((pos + *len - 1) > base->len)
    *len = base->len - (pos - 1);

  end_pos = pos;
  end_pos += *len - 1;
  scan_beg = vstr__base_pos(base, &pos, &dummy_num, TRUE);
  assert(scan_beg);
  --pos;
  
  scan_end = vstr__base_pos(base, &end_pos, num, FALSE);
  assert(scan_end);

  *type = scan_end->type;
  
  if (scan_beg != scan_end)
  {
    assert(*num != dummy_num);
    assert(scan_end != base->beg);
    
    pos = 0;
    *scan_len = end_pos;
    
    assert(*scan_len < *len);
    *len -= *scan_len;
  }
  else
  {
    assert(scan_end->len >= *len);
    *scan_len = *len;
    *len = 0;
  }
  
  *scan_str = NULL;
  if (scan_end->type != VSTR_TYPE_NODE_NON)
    *scan_str = vstr__export_node_ptr(scan_end) + pos;
  
  return (TRUE);
}

int vstr__base_scan_rev_nxt(const Vstr_base *base, size_t *len,
                            unsigned int *num, unsigned int *type,
                            char **scan_str, size_t *scan_len)
{
  struct iovec *iovs = NULL;
  unsigned char *types = NULL;
  size_t pos = 0;
  
  assert(base && num && len && type);
  
  assert(base->iovec_upto_date);
  
  assert(num);
  
  if (!*len || !--*num)
    return (FALSE);

  iovs = VSTR__CACHE(base)->vec->v + VSTR__CACHE(base)->vec->off;
  types = VSTR__CACHE(base)->vec->t + VSTR__CACHE(base)->vec->off;

  *type = types[*num - 1];
  *scan_len = iovs[*num - 1].iov_len;
  
  if (*scan_len > *len)
  {
    pos = *scan_len - *len;
    *scan_len = *len;
  }
  *len -= *scan_len;
  
  *scan_str = NULL;
  if (*type != VSTR_TYPE_NODE_NON)
    *scan_str = ((char *) (iovs[*num - 1].iov_base)) + pos;
  
  return (TRUE);
}

unsigned int vstr_nx_num(const Vstr_base *base, size_t pos, size_t len)
{
  Vstr_node *scan = NULL;
  unsigned int ret = 0;

  if (pos == 1 && len == base->len)
    return (base->num);

  {
    unsigned int num = 0; /* ignore */
    char *scan_str = NULL; /* ignore */
    size_t scan_len = 0; /* ignore */
    
    scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  }
  if (!scan)
    return (0);
  if (!len)
    return (1);

  ret = 2;
  scan = scan->next;
  while (len > scan->len)
  {
    len -= scan->len;
    ++ret;
    
    scan = scan->next;
  }

  return (ret);
}
