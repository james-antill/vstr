#define VSTR_SPLIT_C
/*
 *  Copyright (C) 2002, 2003  James Antill
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
/* functions to split a Vstr into sections (ala. perl split()) */
#include "main.h"

/* do the null fields go to the end ? */
static int vstr__split_buf_null_end(const Vstr_base *base,
                                    size_t pos, size_t len,
                                    const void *buf, size_t buf_len,
                                    unsigned int *ret_num)
{
  assert(vstr_cmp_buf_eq(base, pos, buf_len, buf, buf_len));
  assert(len >= buf_len);

  *ret_num = 1;

  if (len == buf_len)
    return (TRUE);

  pos += buf_len;
  len -= buf_len;

  while (len >= buf_len)
  {
    if (!vstr_cmp_buf_eq(base, pos, buf_len, buf, buf_len))
      return (FALSE);

    ++*ret_num;
    pos += buf_len;
    len -= buf_len;
  }

  return (!len);
}

#define VSTR__SPLIT_HDL_ERR() do { \
 if (sects->malloc_bad) \
 { \
   assert(sects->num >= added); \
   sects->num -= added; \
   return (0); \
 } \
 \
 assert(!sects->can_add_sz); \
 assert(sects->num == sects->sz); \
 if (flags & VSTR_FLAG_SPLIT_NO_RET) \
   return (1); \
} while (FALSE)

static unsigned int vstr__split_hdl_null_beg(size_t *pos, size_t *len,
                                             size_t buf_len,
                                             Vstr_sects *sects,
                                             unsigned int flags,
                                             unsigned int count,
                                             unsigned int limit,
                                             unsigned int added)
{
  const int is_remain = !!(flags & VSTR_FLAG_SPLIT_REMAIN);

  if (limit && (count >= (limit - added)))
    count = (limit - is_remain) - added;

  assert(count);

  while (count)
  {
    if (flags & VSTR_FLAG_SPLIT_BEG_NULL)
    {
      if (!vstr_sects_add(sects, *pos, 0))
        VSTR__SPLIT_HDL_ERR();

      ++added;
    }

    *pos += buf_len;
    *len -= buf_len;
    --count;
  }

  return (added);
}

static unsigned int vstr__split_hdl_null_mid(size_t *pos, size_t *len,
                                             size_t buf_len,
                                             Vstr_sects *sects,
                                             unsigned int flags,
                                             unsigned int count,
                                             unsigned int limit,
                                             unsigned int added)
{
  const int is_remain = !!(flags & VSTR_FLAG_SPLIT_REMAIN);

  if (limit && (count >= (limit - added)))
    count = (limit - is_remain) - added;

  assert(count);

  while (count)
  {
    if (flags & VSTR_FLAG_SPLIT_MID_NULL)
    {
      if (!vstr_sects_add(sects, *pos, 0))
        VSTR__SPLIT_HDL_ERR();

      ++added;
    }

    *pos += buf_len;
    *len -= buf_len;
    --count;
  }

  return (added);
}

static unsigned int vstr__split_hdl_null_end(size_t pos, size_t len,
                                             size_t buf_len,
                                             Vstr_sects *sects,
                                             unsigned int flags,
                                             unsigned int count,
                                             unsigned int limit,
                                             unsigned int added)
{
  assert(len);

  if (!(flags & VSTR_FLAG_SPLIT_END_NULL))
    goto no_end_null;

  if (limit && (count > (limit - added)))
    count = limit - added;

  while (count)
  {
    if (!vstr_sects_add(sects, pos, 0))
      VSTR__SPLIT_HDL_ERR();

    ++added;

    pos += buf_len;
    len -= buf_len;
    --count;
  }
  assert(added);

  if (!(flags & VSTR_FLAG_SPLIT_POST_NULL) && !len)
    return (added);

 no_end_null:
  if (((len &&  (flags & VSTR_FLAG_SPLIT_REMAIN)) ||
       (!len && (flags & VSTR_FLAG_SPLIT_POST_NULL))))
  {
    if (!vstr_sects_add(sects, pos, len))
      VSTR__SPLIT_HDL_ERR();
    ++added;
  }

  return (added);
}

unsigned int vstr_split_buf(const Vstr_base *base, size_t pos, size_t len,
                            const void *buf, size_t buf_len,
                            Vstr_sects *sects,
                            unsigned int limit, unsigned int flags)
{
  size_t orig_pos = pos;
  size_t split_pos = 0;
  unsigned int added = 0;
  const int is_remain = !!(flags & VSTR_FLAG_SPLIT_REMAIN);

  while (len && (!limit || (added < (limit - is_remain))) &&
         (split_pos = vstr_srch_buf_fwd(base, pos, len, buf, buf_len)))
  {
    if (split_pos == orig_pos)
    {
      unsigned int count = 0;

      assert(orig_pos == pos);

      if (vstr__split_buf_null_end(base, pos, len, buf, buf_len, &count))
      {
        if (!(flags & VSTR_FLAG_SPLIT_BEG_NULL))
          return (0);

        return (vstr__split_hdl_null_end(pos, len, buf_len, sects, flags,
                                         count, limit, added));
      }
      added = vstr__split_hdl_null_beg(&pos, &len, buf_len, sects, flags,
                                       count, limit, added);
    }
    else if (split_pos == pos)
    {
      unsigned int count = 0;

      if (vstr__split_buf_null_end(base, pos, len, buf, buf_len, &count))
        return (vstr__split_hdl_null_end(pos, len, buf_len, sects, flags,
                                         count, limit, added));
      added = vstr__split_hdl_null_mid(&pos, &len, buf_len, sects, flags,
                                       count, limit, added);
    }
    else
    {
      size_t split_len = 0;

      assert(split_pos > pos);

      split_len = (split_pos - pos);

      if (!vstr_sects_add(sects, pos, split_len))
        VSTR__SPLIT_HDL_ERR();

      ++added;

      split_len += buf_len;

      pos += split_len;
      len -= split_len;
    }
  }

  if (len)
  {
    if (flags & VSTR_FLAG_SPLIT_REMAIN)
    {
      assert(!limit || (added <= (limit - 1)));

      if (!vstr_sects_add(sects, pos, len))
        VSTR__SPLIT_HDL_ERR();

      ++added;
    }
    else if (!split_pos)
    {
      if (!vstr_sects_add(sects, pos, len))
        VSTR__SPLIT_HDL_ERR();

      ++added;
    }
  }
  else if ((flags & VSTR_FLAG_SPLIT_POST_NULL) && (!limit || (added < limit)))
  {
    if (!vstr_sects_add(sects, pos, 0))
      VSTR__SPLIT_HDL_ERR();
    
    ++added;
  }

  return (added);
}

unsigned int vstr_split_chrs(const Vstr_base *base, size_t pos, size_t len,
                             const char *chrs, size_t chrs_len,
                             Vstr_sects *sects,
                             unsigned int limit, unsigned int flags)
{
  size_t orig_pos = pos;
  size_t split_pos = 0;
  unsigned int added = 0;
  const int is_remain = !!(flags & VSTR_FLAG_SPLIT_REMAIN);

  while (len && (!limit || (added < (limit - is_remain))) &&
         (split_pos = vstr_srch_chrs_fwd(base, pos, len, chrs, chrs_len)))
  {
    if (split_pos == orig_pos)
    {
      unsigned int count = 0;

      assert(orig_pos == pos);

      if ((count = vstr_spn_chrs_fwd(base, pos, len, chrs, chrs_len)) == len)
      {
        if (!(flags & VSTR_FLAG_SPLIT_BEG_NULL))
          return (0);

        return (vstr__split_hdl_null_end(pos, len, 1, sects, flags,
                                         count, limit, added));
      }
      added = vstr__split_hdl_null_beg(&pos, &len, 1, sects, flags,
                                       count, limit, added);
    }
    else if (split_pos == pos)
    {
      unsigned int count = 0;

      if ((count = vstr_spn_chrs_fwd(base, pos, len, chrs, chrs_len)) == len)
        return (vstr__split_hdl_null_end(pos, len, 1, sects, flags,
                                         count, limit, added));

      added = vstr__split_hdl_null_mid(&pos, &len, 1, sects, flags,
                                       count, limit, added);
    }
    else
    {
      size_t split_len = 0;

      assert(split_pos > pos);

      split_len = (split_pos - pos);

      if (!vstr_sects_add(sects, pos, split_len))
        VSTR__SPLIT_HDL_ERR();

      ++added;

      split_len += 1;

      pos += split_len;
      len -= split_len;
    }
  }

  if (len)
  {
    if (flags & VSTR_FLAG_SPLIT_REMAIN)
    {
      assert(!limit || (added <= (limit - 1)));

      if (!vstr_sects_add(sects, pos, len))
        VSTR__SPLIT_HDL_ERR();

      ++added;
    }
    else if (!split_pos)
    {
      if (!vstr_sects_add(sects, pos, len))
        VSTR__SPLIT_HDL_ERR();

      ++added;
    }
  }
  else if ((flags & VSTR_FLAG_SPLIT_POST_NULL) && (!limit || (added < limit)))
    if (!vstr_sects_add(sects, pos, 0))
      VSTR__SPLIT_HDL_ERR();

  return (added);
}
#undef VSTR__SPLIT_HDL_ERR
