#define VSTR_MOV_C
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
/* functions to move data from one vstr to another */
#include "main.h"

static int vstr__mov_slow(Vstr_base *base, size_t pos,
                          Vstr_base *from_base, size_t from_pos, size_t len)
{
  int ret = 0;

  assert(base != from_base);

  ret = vstr_nx_add_vstr(base, pos,
                         from_base, from_pos, len, VSTR_TYPE_ADD_DEF);
  if (!ret)
    return (FALSE);

  ret = vstr_nx_del(from_base, from_pos, len);
  if (!ret)
  {
    ret = vstr_nx_del(base, pos, len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }
  
  return (ret);
}

/* *ret == start of data */
static Vstr_node **vstr__mov_setup_beg(Vstr_base *base, size_t pos,
                                       unsigned int *num)
{
  Vstr_node *scan = NULL;

  assert(num && pos);
  --pos;
  if (!pos)
  {
    *num = 1;
    vstr__base_zero_used(base);
    return (&base->beg);
  }
  
  scan = vstr__base_pos(base, &pos, num, TRUE);
  
  if ((pos != scan->len) && !(scan = vstr__base_split_node(base, scan, pos)))
    return (NULL);

  ++*num;
  
  return (&scan->next);
}

/* *ret == after end of data */
static Vstr_node **vstr__mov_setup_end(Vstr_base *base, size_t pos,
                                       unsigned int *num)
{
  Vstr_node *scan = NULL;
  unsigned int dummy_num;

  if (!num)
    num = &dummy_num;
  
  if (!pos)
  {
    *num = 0;
    vstr__base_zero_used(base);
    return (&base->beg);
  }
  
  scan = vstr__base_pos(base, &pos, num, TRUE);
  
  if ((pos != scan->len) && !(scan = vstr__base_split_node(base, scan, pos)))
    return (NULL);
  
  return (&scan->next);
}

static void vstr__mov_iovec_kill(Vstr_base *base)
{
  if (!base->cache_available)
    return;

  if (!VSTR__CACHE(base))
    return;

  base->iovec_upto_date = FALSE;
}

int vstr_mov(Vstr_base *base, size_t pos,
             Vstr_base *from_base, size_t from_pos, size_t len)
{
  Vstr_node **beg = NULL;
  Vstr_node **end = NULL;
  Vstr_node **con = NULL;
  Vstr_node *tmp = NULL;
  unsigned int beg_num = 0;
  unsigned int end_num = 0;
  unsigned int num = 0;
  
  if (!len)
    return (TRUE);
  
  if (base->conf->buf_sz != from_base->conf->buf_sz)
    return (vstr__mov_slow(base, pos, from_base, from_pos, len));

  if ((base == from_base) && (pos >= from_pos) && (pos < (from_pos + len)))
    return (TRUE); /* move a string anywhere into itself doesn't do anything */
  
  assert(vstr__check_real_nodes(base));
  assert((from_base == base) || vstr__check_real_nodes(from_base));

  if (!(beg = vstr__mov_setup_beg(from_base, from_pos, &beg_num)))
    return (FALSE);
  
  if (!(end = vstr__mov_setup_end(from_base, from_pos + len - 1, &end_num)))
    return (FALSE);

  if (!(con = vstr__mov_setup_end(base, pos, NULL)))
    return (FALSE);

  /* NOTE: the numbers might be off by one if con is before beg,
   * but that doesn't matter as we just need the difference */
  num = end_num - beg_num + 1;
  
  tmp = *beg;
  *beg = *end;

  if (from_base->beg && !from_base->end)
    from_base->end = VSTR__CONV_PTR_NEXT_PREV(beg);

  from_base->len -= len;
  from_base->num -= num;
  vstr__cache_del(from_base, from_pos, len);
  vstr__mov_iovec_kill(from_base);
  
  beg = &tmp;

  *end = *con;
  *con = *beg;

  if (!*end)
    base->end = VSTR__CONV_PTR_NEXT_PREV(end);

  base->len += len;
  base->num += num;
  vstr__cache_add(base, pos, len);
  vstr__mov_iovec_kill(base); /* This is easy to rm, if they append */
  
  assert(vstr__check_real_nodes(base));
  assert((from_base == base) || vstr__check_real_nodes(from_base));

  return (TRUE);
}
