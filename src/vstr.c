#define VSTR_C
/*
 *  Copyright (C) 1999, 2000, 2001, 2002, 2003  James Antill
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

static int vstr__make_conf_loc_def_numeric(Vstr_conf *conf, Vstr_locale *loc)
{
  (void)conf;

  if (!(loc->name_lc_numeric_str = VSTR__M0(2)))
    goto fail_numeric;

  if (!(loc->grouping = VSTR__M0(1)))
    goto fail_grp;

  if (!(loc->thousands_sep_str = VSTR__M0(1)))
    goto fail_thou;

  if (!(loc->decimal_point_str = VSTR__M0(2)))
    goto fail_deci;

  *loc->name_lc_numeric_str = 'C';
  loc->name_lc_numeric_len = 1;

  *loc->decimal_point_str = '.';
  loc->decimal_point_len = 1;

  loc->thousands_sep_len = 0;

  return (TRUE);

 fail_deci:
  VSTR__F(loc->thousands_sep_str);
 fail_thou:
  VSTR__F(loc->grouping);
 fail_grp:
  VSTR__F(loc->name_lc_numeric_str);
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
    if (!vstr__make_conf_loc_def_numeric(conf, loc))
      goto fail_numeric;
  }
  else
  {
    const char *name_numeric = NULL;
    size_t numeric_len = 0;
    size_t grp_len  = vstr__loc_thou_grp_strlen(SYS_LOC(grouping));
    size_t thou_len =                    strlen(SYS_LOC(thousands_sep));
    size_t deci_len =                    strlen(SYS_LOC(decimal_point));

    if (VSTR__DEBUG_MALLOC_TST() ||
        !(name_numeric = setlocale(LC_NUMERIC, NULL))) /* name for "name" */
      name_numeric = "C";
    numeric_len = strlen(name_numeric);

    if (!(loc->name_lc_numeric_str = VSTR__MK(numeric_len + 1)))
      goto fail_numeric;

    if (!(loc->grouping = VSTR__MK(grp_len + 1)))
      goto fail_grp;

    if (!(loc->thousands_sep_str = VSTR__MK(thou_len + 1)))
      goto fail_thou;

    if (!(loc->decimal_point_str = VSTR__MK(deci_len + 1)))
      goto fail_deci;

    vstr_wrap_memcpy(loc->name_lc_numeric_str, name_numeric,
                     numeric_len + 1);
    loc->name_lc_numeric_len = numeric_len;

    vstr_wrap_memcpy(loc->grouping, SYS_LOC(grouping), grp_len);
    loc->grouping[grp_len] = 0; /* make sure all cstrs are 0 terminated */

    vstr_wrap_memcpy(loc->thousands_sep_str, SYS_LOC(thousands_sep),
                     thou_len + 1);
    loc->thousands_sep_len = thou_len;

    vstr_wrap_memcpy(loc->decimal_point_str, SYS_LOC(decimal_point),
                     deci_len + 1);
    loc->decimal_point_len = deci_len;
  }

  VSTR__F(conf->loc->name_lc_numeric_str);
  conf->loc->name_lc_numeric_str = loc->name_lc_numeric_str;
  conf->loc->name_lc_numeric_len = loc->name_lc_numeric_len;

  VSTR__F(conf->loc->grouping);
  conf->loc->grouping = loc->grouping;

  VSTR__F(conf->loc->thousands_sep_str);
  conf->loc->thousands_sep_str = loc->thousands_sep_str;
  conf->loc->thousands_sep_len = loc->thousands_sep_len;

  VSTR__F(conf->loc->decimal_point_str);
  conf->loc->decimal_point_str = loc->decimal_point_str;
  conf->loc->decimal_point_len = loc->decimal_point_len;

  if (tmp)
    setlocale(LC_NUMERIC, tmp);

  return (TRUE);

 fail_deci:
  VSTR__F(loc->thousands_sep_str);
 fail_thou:
  VSTR__F(loc->grouping);
 fail_grp:
  VSTR__F(loc->name_lc_numeric_str);
 fail_numeric:

  if (tmp)
    setlocale(LC_NUMERIC, tmp);

  return (FALSE);
}

Vstr_conf *vstr_make_conf(void)
{
  Vstr_conf *conf = VSTR__MK(sizeof(Vstr_conf));

  if (!conf)
    goto fail_conf;

  if (!vstr__cache_conf_init(conf))
    goto fail_cache;

  if (!(conf->loc = VSTR__MK(sizeof(Vstr_locale))))
    goto fail_loc;

  conf->loc->name_lc_numeric_str = NULL;
  conf->loc->grouping = NULL;
  conf->loc->thousands_sep_str = NULL;
  conf->loc->decimal_point_str = NULL;

  if (!vstr__make_conf_loc_def_numeric(conf, conf->loc))
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

  conf->buf_sz = 64 - (sizeof(Vstr_node_buf) + 8);

  conf->fmt_usr_names = 0;
  conf->fmt_usr_escape = 0;

  conf->vstr__fmt_spec_make = NULL;
  conf->vstr__fmt_spec_list_beg = NULL;
  conf->vstr__fmt_spec_list_end = NULL;

  conf->ref = 1;

  conf->ref_link = NULL;

  conf->free_do = TRUE;
  conf->malloc_bad = FALSE;
  conf->iovec_auto_update = TRUE;
  conf->split_buf_del = FALSE;

  conf->user_ref = 1;

  conf->no_cache = FALSE;
  conf->fmt_usr_curly_braces = TRUE;
  conf->atomic_ops = TRUE;

  return (conf);

 fail_numeric:
  VSTR__F(conf->loc);
 fail_loc:
  VSTR__F(conf->cache_cbs_ents);
 fail_cache:
  VSTR__F(conf);
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

    vstr_free_spare_nodes(conf, VSTR_TYPE_NODE_BUF, conf->spare_buf_num);
    vstr_free_spare_nodes(conf, VSTR_TYPE_NODE_NON, conf->spare_non_num);
    vstr_free_spare_nodes(conf, VSTR_TYPE_NODE_PTR, conf->spare_ptr_num);
    vstr_free_spare_nodes(conf, VSTR_TYPE_NODE_REF, conf->spare_ref_num);

    VSTR__F(conf->loc->name_lc_numeric_str);
    VSTR__F(conf->loc->grouping);
    VSTR__F(conf->loc->thousands_sep_str);
    VSTR__F(conf->loc->decimal_point_str);
    VSTR__F(conf->loc);

    VSTR__F(conf->cache_cbs_ents);

    vstr__add_fmt_free_conf(conf);

    if (conf->free_do)
      VSTR__F(conf);
  }
}

int vstr_swap_conf(Vstr_base *base, Vstr_conf **conf)
{
  assert(conf && *conf);
  assert((*conf)->user_ref > 0);
  assert((*conf)->user_ref <= (*conf)->ref);

  if (base->conf == *conf)
    return (TRUE);

  if (base->conf->buf_sz != (*conf)->buf_sz)
  {
    if ((*conf)->user_ref != (*conf)->ref)
      return (FALSE);

    vstr_free_spare_nodes(*conf, VSTR_TYPE_NODE_BUF, (*conf)->spare_buf_num);
    (*conf)->buf_sz = base->conf->buf_sz;
  }

  if (!vstr__cache_subset_cbs(base->conf, *conf))
  {
    if ((*conf)->user_ref != (*conf)->ref)
      return (FALSE); /* not used in any base string objects */

    if (!vstr__cache_dup_cbs(*conf, base->conf))
      return (FALSE);
  }

  --(*conf)->user_ref;
  ++base->conf->user_ref;

  SWAP_TYPE(*conf, base->conf, Vstr_conf *);

  return (TRUE);
}

void vstr_free_conf(Vstr_conf *conf)
{
  if (!conf)
    return;

  assert(conf->user_ref > 0);
  assert(conf->user_ref <= conf->ref);

  --conf->user_ref;

  vstr__del_conf(conf);
}

int vstr_init(void)
{
  if (!vstr__options.def)
  {
    vstr__options.mmap_count = 0;
    vstr__options.mem_sz = 0;
    vstr__options.mem_num = 0;
    vstr__options.mem_vals = NULL;
    if (!(vstr__options.def = vstr_make_conf()))
      return (FALSE);
  }

  return (TRUE);
}

void vstr_exit(void)
{
  assert((vstr__options.def->user_ref == 1) &&
         (vstr__options.def->ref == 1));

  vstr_free_conf(vstr__options.def);
  vstr__options.def = NULL;

  assert(!vstr__options.mmap_count);
  assert(!vstr__options.mem_num);
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

  assert(!VSTR__CACHE(base)->vec->sz ||
         (VSTR__CACHE(base)->vec->off == base->conf->iov_min_offset));

  count = base->conf->iov_min_offset;
  scan = base->beg;

  VSTR__CACHE(base)->vec->v[count].iov_len = scan->len - base->used;
  if (scan->type == VSTR_TYPE_NODE_NON)
    VSTR__CACHE(base)->vec->v[count].iov_base = NULL;
  else
    VSTR__CACHE(base)->vec->v[count].iov_base = (vstr_export__node_ptr(scan) +
                                                 base->used);
  VSTR__CACHE(base)->vec->t[count] = scan->type;

  scan = scan->next;
  ++count;
  while (scan)
  {
    VSTR__CACHE(base)->vec->t[count] = scan->type;
    VSTR__CACHE(base)->vec->v[count].iov_len = scan->len;
    VSTR__CACHE(base)->vec->v[count].iov_base = vstr_export__node_ptr(scan);

    ++count;
    scan = scan->next;
  }

  base->iovec_upto_date = TRUE;

  return (TRUE);
}

#ifndef NDEBUG
static void vstr__cache_iovec_check(const Vstr_base *base)
{
  unsigned int count = 0;
  Vstr_node *scan = NULL;

  if (!base->iovec_upto_date || !base->beg)
    return;

  count = VSTR__CACHE(base)->vec->off;
  scan = base->beg;

  assert(scan->len > base->used);
  assert(VSTR__CACHE(base)->vec->sz >= base->num);
  assert(VSTR__CACHE(base)->vec->v[count].iov_len == (size_t)(scan->len - base->used));
  if (scan->type == VSTR_TYPE_NODE_NON)
    assert(!VSTR__CACHE(base)->vec->v[count].iov_base);
  else
    assert(VSTR__CACHE(base)->vec->v[count].iov_base ==
           (vstr_export__node_ptr(scan) + base->used));
  assert(VSTR__CACHE(base)->vec->t[count] == scan->type);

  if (!(scan = scan->next))
    assert(base->beg == base->end);

  ++count;
  while (scan)
  {
    assert(VSTR__CACHE(base)->vec->t[count] == scan->type);
    assert(VSTR__CACHE(base)->vec->v[count].iov_len == scan->len);
    assert(VSTR__CACHE(base)->vec->v[count].iov_base ==
           vstr_export__node_ptr(scan));

    ++count;

    if (!scan->next)
      assert(scan == base->end);

    scan = scan->next;
  }
}

static void vstr__cache_cstr_check(const Vstr_base *base)
{
  Vstr__cache_data_cstr *data = NULL;
  const char *ptr = NULL;

  ASSERT(base->conf->cache_pos_cb_cstr == 3);

  if (!(data = vstr_cache_get(base, base->conf->cache_pos_cb_cstr)))
    return;
  if (!data->ref || !data->len)
    return;

  ASSERT(data->len < data->sz);

  ptr  = data->ref->ptr;
  ptr += data->off;

  { /* special case vstr_cmp_buf_eq() ... works with _NON data */
    Vstr_iter iter[1];

    if (!vstr_iter_fwd_beg(base, data->pos, data->len, iter))
      assert(FALSE);

    do
    {
      if (iter->node->type != VSTR_TYPE_NODE_NON)
        ASSERT(!memcmp(iter->ptr, ptr, iter->len));
      ptr += iter->len;
    } while (vstr_iter_fwd_nxt(iter));
  }
}
#endif

static int vstr__init_base(Vstr_conf *conf, Vstr_base *base)
{
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
    {
      vstr__del_conf(conf);
      return (FALSE);
    }

    /* if they've used min alloc etc. ... we can start using */
    if (VSTR__CACHE(base) &&
        VSTR__CACHE(base)->vec && VSTR__CACHE(base)->vec->sz)
      base->iovec_upto_date = TRUE;
  }

  return (TRUE);
}

void vstr_free_base(Vstr_base *base)
{
  Vstr_conf *conf = NULL;

  if (!base)
    return;

  conf = base->conf;

  vstr__free_cache(base);

  vstr_del(base, 1, base->len);

  if (base->free_do)
    VSTR__F(base);

  vstr__del_conf(conf);
}

Vstr_base *vstr_make_base(Vstr_conf *conf)
{
  Vstr_base *base = NULL;
  size_t sz = 0;

  if (!conf)
    conf = vstr__options.def;

  if (conf->no_cache)
    sz = sizeof(Vstr_base);
  else
    sz = sizeof(Vstr__base_cache);

  base = VSTR__MK(sz);

  if (!base)
    goto malloc_fail_base;

  if (!vstr__init_base(conf, base))
    goto malloc_fail_init;

  base->free_do = TRUE;

  return (base);
 malloc_fail_init:
  VSTR__F(base);
 malloc_fail_base:
  conf->malloc_bad = TRUE;
  return (NULL);
}

Vstr_node *vstr__base_split_node(Vstr_base *base, Vstr_node *node, size_t pos)
{
  Vstr_node *beg = NULL;

  assert(base && pos && (pos <= node->len));

  switch (node->type)
  {
    case VSTR_TYPE_NODE_BUF:
      if (!vstr_cntl_conf(base->conf,
                          VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BUF, 1, UINT_MAX))
        return (NULL);

      --base->conf->spare_buf_num;
      beg = (Vstr_node *)base->conf->spare_buf_beg;
      base->conf->spare_buf_beg = (Vstr_node_buf *)beg->next;

      vstr_wrap_memcpy(((Vstr_node_buf *)beg)->buf,
                       ((Vstr_node_buf *)node)->buf + pos, node->len - pos);
      break;

    case VSTR_TYPE_NODE_NON:
      if (!vstr_cntl_conf(base->conf,
                          VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_NON, 1, UINT_MAX))
        return (NULL);

      --base->conf->spare_non_num;
      beg = (Vstr_node *)base->conf->spare_non_beg;
      base->conf->spare_non_beg = (Vstr_node_non *)beg->next;
      break;

    case VSTR_TYPE_NODE_PTR:
      if (!vstr_cntl_conf(base->conf,
                          VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_PTR, 1, UINT_MAX))
        return (NULL);

      --base->conf->spare_ptr_num;
      beg = (Vstr_node *)base->conf->spare_ptr_beg;
      base->conf->spare_ptr_beg = (Vstr_node_ptr *)beg->next;

      ((Vstr_node_ptr *)beg)->ptr = ((char *)((Vstr_node_ptr *)node)->ptr) + pos;
      break;

    case VSTR_TYPE_NODE_REF:
    {
      Vstr_ref *ref = NULL;

      if (!vstr_cntl_conf(base->conf,
                          VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_REF, 1, UINT_MAX))
        return (NULL);

      --base->conf->spare_ref_num;
      beg = (Vstr_node *)base->conf->spare_ref_beg;
      base->conf->spare_ref_beg = (Vstr_node_ref *)beg->next;

      ref = ((Vstr_node_ref *)node)->ref;
      ((Vstr_node_ref *)beg)->ref = vstr_ref_add(ref);
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
      node = VSTR__MK(sizeof(Vstr_node_buf) + conf->buf_sz);
      break;
    case VSTR_TYPE_NODE_NON:
      node = VSTR__MK(sizeof(Vstr_node_non));
      break;
    case VSTR_TYPE_NODE_PTR:
      node = VSTR__MK(sizeof(Vstr_node_ptr));
      break;
    case VSTR_TYPE_NODE_REF:
      node = VSTR__MK(sizeof(Vstr_node_ref));
      break;

    default:
      assert(FALSE);
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

unsigned int vstr_make_spare_nodes(Vstr_conf *passed_conf, unsigned int type,
                                   unsigned int num)
{
  Vstr_conf *conf = passed_conf ? passed_conf : vstr__options.def;
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
      VSTR__F(scan);
      break;

    case VSTR_TYPE_NODE_NON:
      if (!conf->spare_non_beg)
        return (FALSE);

      scan = (Vstr_node *)conf->spare_non_beg;

      assert(scan->type == VSTR_TYPE_NODE_NON);

      conf->spare_non_beg = (Vstr_node_non *)scan->next;
      --conf->spare_non_num;
      VSTR__F(scan);
      break;

    case VSTR_TYPE_NODE_PTR:
      if (!conf->spare_ptr_beg)
        return (FALSE);

      scan = (Vstr_node *)conf->spare_ptr_beg;

      assert(scan->type == VSTR_TYPE_NODE_PTR);

      conf->spare_ptr_beg = (Vstr_node_ptr *)scan->next;
      --conf->spare_ptr_num;
      VSTR__F(scan);
      break;

    case VSTR_TYPE_NODE_REF:
      if (!conf->spare_ref_beg)
        return (FALSE);

      scan = (Vstr_node *)conf->spare_ref_beg;

      assert(scan->type == type);

      conf->spare_ref_beg = (Vstr_node_ref *)scan->next;
      --conf->spare_ref_num;
      VSTR__F(scan);
      break;

    default:
      assert(FALSE);
      return (FALSE);
  }

  return (TRUE);
}

unsigned int vstr_free_spare_nodes(Vstr_conf *passed_conf, unsigned int type,
                                   unsigned int num)
{
  Vstr_conf *conf = passed_conf ? passed_conf : vstr__options.def;
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
int vstr__check_real_nodes(const Vstr_base *base)
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
    ASSERT(scan->len);

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

int vstr__check_spare_nodes(const Vstr_conf *conf)
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

Vstr_node **vstr__base_ptr_pos(const Vstr_base *base, size_t *pos,
                               unsigned int *num)
{
  Vstr_node *const *scan = &base->beg;

  *pos += base->used;
  *num = 1;

  while (*pos > (*scan)->len)
  {
    *pos -= (*scan)->len;

    assert((*scan)->next);
    scan = &(*scan)->next;
    ++*num;
  }

  return ((Vstr_node **) scan);
}

struct Vstr__conf_ref_linked
{
 Vstr_conf *conf;
 unsigned int l_ref;
};

/* put node on reference list */
static int vstr__convert_buf_ref_add(Vstr_conf *conf, Vstr_node *node)
{
  struct Vstr__conf_ref_linked *ln_ref;

  if (!(ln_ref = conf->ref_link))
  {
    if (!(ln_ref = VSTR__MK(sizeof(struct Vstr__conf_ref_linked))))
      return (FALSE);

    ln_ref->conf = conf;
    ln_ref->l_ref = 0;

    conf->ref_link = ln_ref;
    ++conf->ref;
  }
  ASSERT(ln_ref->l_ref < VSTR__CONF_REF_LINKED_SZ);

  ++ln_ref->l_ref;
  node->next = (Vstr_node *)ln_ref;

  if (ln_ref->l_ref >= VSTR__CONF_REF_LINKED_SZ)
    conf->ref_link = NULL;

  return (TRUE);
}

/* call back ... relink */
static void vstr__ref_cb_relink_bufnode_ref(Vstr_ref *ref)
{
  if (!ref)
    return;

  if (ref->ptr)
  {
    char *tmp = ref->ptr;
    Vstr_node_buf *node = NULL;
    struct Vstr__conf_ref_linked *ln_ref = NULL;
    Vstr_conf *conf = NULL;

    tmp -= offsetof(Vstr_node_buf, buf);
    node = (Vstr_node_buf *)tmp;
    ln_ref = (struct Vstr__conf_ref_linked *)(node->s.next);

    conf = ln_ref->conf;

    /* manual relink ... easy */
    node->s.next = (Vstr_node *)ln_ref->conf->spare_buf_beg;
    ln_ref->conf->spare_buf_beg = node;
    ++ln_ref->conf->spare_buf_num;

    free(ref); /* make_ptr()'d reference */

    if (!--ln_ref->l_ref)
    {
      if (ln_ref->conf->ref_link == ln_ref)
        ln_ref->conf->ref_link = NULL;

      VSTR__F(ln_ref);
      vstr__del_conf(conf);
    }
  }
}

void vstr__swap_node_X_X(const Vstr_base *base, size_t pos,
                         Vstr_node *node)
{
  Vstr_node *old_scan = NULL;
  size_t old_scan_len = 0;
  Vstr_node **scan = NULL;
  unsigned int num = 0;
  Vstr__cache_data_pos *data = NULL;

  scan = vstr__base_ptr_pos(base, &pos, &num);

  old_scan = *scan;
  old_scan_len = old_scan->len;

  node->next = (*scan)->next;
  vstr__relink_nodes(base->conf, *scan, &(*scan)->next, 1);
  *scan = node;

  if (node == base->beg)
    ((Vstr_base *)base)->used = 0;

  if ((data = vstr_cache_get(base, base->conf->cache_pos_cb_pos)) &&
      (data->node == old_scan))
  {
    data->node = NULL;

    if (node->len >= old_scan_len)
      data->node = node;
  }

  switch (node->type)
  {
    /* -- note, shoudl work, but not called like so currently --
      case VSTR_TYPE_NODE_BUF: ((Vstr_base *)base)->node_buf_used = TRUE; break;
      case VSTR_TYPE_NODE_NON: ((Vstr_base *)base)->node_non_used = TRUE; break;
    */
    case VSTR_TYPE_NODE_PTR: ((Vstr_base *)base)->node_ptr_used = TRUE; break;
    case VSTR_TYPE_NODE_REF: ((Vstr_base *)base)->node_ref_used = TRUE; break;
    default:
      ASSERT(FALSE);
  }

  vstr__cache_iovec_reset_node(base, node, num);
}


int vstr__chg_node_buf_ref(const Vstr_base *base,
                           Vstr_node **scan, unsigned int num)
{
  Vstr__cache_data_pos *data = NULL;
  Vstr_node *tmp = (*scan)->next; /* must be done now... */
  Vstr_ref *ref = NULL;
  Vstr_node *ref_node = NULL;

  assert((*scan)->type == VSTR_TYPE_NODE_BUF);

  if (!vstr_cntl_conf(base->conf, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_REF,
                      1, UINT_MAX))
    goto fail_malloc_nodes;

  if (!(ref = vstr_ref_make_ptr(((Vstr_node_buf *)(*scan))->buf,
                                vstr__ref_cb_relink_bufnode_ref)))
    goto fail_malloc_ref;
  if (!vstr__convert_buf_ref_add(base->conf, *scan))
    goto fail_malloc_conv_buf;

  --base->conf->spare_ref_num;
  ref_node = (Vstr_node *)base->conf->spare_ref_beg;
  base->conf->spare_ref_beg = (Vstr_node_ref *)ref_node->next;

  ((Vstr_base *)base)->node_ref_used = TRUE;

  ref_node->len = (*scan)->len;
  ((Vstr_node_ref *)ref_node)->ref = ref;
  ((Vstr_node_ref *)ref_node)->off = 0;
  if ((base->beg == *scan) && base->used)
  {
    ref_node->len -= base->used;
    ((Vstr_node_ref *)ref_node)->off = base->used;
    ((Vstr_base *)base)->used = 0;
  }

  if (!(ref_node->next = tmp))
    ((Vstr_base *)base)->end = ref_node;

  /* NOTE: we have just changed the type of the node, must update the cache */
  if ((data = vstr_cache_get(base, base->conf->cache_pos_cb_pos)) &&
      (data->node == *scan))
    data->node = ref_node;
  if (base->iovec_upto_date)
  {
    ASSERT(num);
    num += VSTR__CACHE(base)->vec->off - 1;
    assert(VSTR__CACHE(base)->vec->t[num] == VSTR_TYPE_NODE_BUF);
    VSTR__CACHE(base)->vec->t[num] = VSTR_TYPE_NODE_REF;
  }

  *scan = ref_node;

  return (TRUE);

 fail_malloc_conv_buf:
  ref->ptr = NULL;
  vstr_ref_del(ref);
 fail_malloc_ref:
  base->conf->malloc_bad = TRUE;
 fail_malloc_nodes:
  return (FALSE);
}
