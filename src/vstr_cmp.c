#define VSTR_CMP_C
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
/* functions for comparing vstrs */
#include "main.h"

/* compare 2 vector strings */
int vstr_nx_cmp(const Vstr_base *base_1, size_t pos_1, size_t len_1,
                const Vstr_base *base_2, size_t pos_2, size_t len_2)
{
 Vstr_node *scan_1 = NULL;
 Vstr_node *scan_2 = NULL;
 unsigned int num_1 = 0;
 unsigned int num_2 = 0;
 char *scan_str_1 = NULL;
 char *scan_str_2 = NULL;
 size_t scan_len_1 = 0;
 size_t scan_len_2 = 0;

 scan_1 = vstr__base_scan_fwd_beg(base_1, pos_1, &len_1, &num_1,
                                  &scan_str_1, &scan_len_1);
 scan_2 = vstr__base_scan_fwd_beg(base_2, pos_2, &len_2, &num_2,
                                  &scan_str_2, &scan_len_2);
 if (!scan_1 && !scan_2)
   return (0);
 if (!scan_1)
   return (-1);
 if (!scan_2)
   return (1);

 do
 {
  size_t tmp = scan_len_1;
  if (tmp > scan_len_2)
    tmp = scan_len_2;
  
  if ((scan_1->type != VSTR_TYPE_NODE_NON) &&
      (scan_2->type != VSTR_TYPE_NODE_NON))
  {
   int ret = vstr_nx_wrap_memcmp(scan_str_1, scan_str_2, tmp);
   if (ret)
     return (ret);
   scan_str_1 += tmp;
   scan_str_2 += tmp;
  }
  else if (scan_1->type != VSTR_TYPE_NODE_NON)
    return (1);
  else if (scan_2->type != VSTR_TYPE_NODE_NON)
    return (-1);

  scan_len_1 -= tmp;
  scan_len_2 -= tmp;

  assert(!scan_len_1 || !scan_len_2);
  if (!scan_len_1)
    scan_1 = vstr__base_scan_fwd_nxt(base_1, &len_1, &num_1,
                                     scan_1, &scan_str_1, &scan_len_1);
  if (!scan_len_2)
    scan_2 = vstr__base_scan_fwd_nxt(base_2, &len_2, &num_2,
                                     scan_2, &scan_str_2, &scan_len_2);
 } while (scan_1 && scan_2);

 if (scan_1)
   return (1);
 if (scan_2)
   return (-1);
 
 return (0);
}

/* compare with a "normal" C string */
int vstr_nx_cmp_buf(const Vstr_base *base, size_t pos, size_t len,
                    const void *buf, size_t buf_len)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan && !buf_len)
    return (0);
  if (!scan)
    return (-1);
  if (!buf_len)
    return (1);
  
  do
  {
    int ret = 0;
    
    if (scan_len > buf_len)
    {
      scan_len = buf_len;
      len += !len; /* just need enough for test at end, Ie. 1 when len == 0 */
    }
    
    if ((scan->type == VSTR_TYPE_NODE_NON) && buf)
      return (-1);
    if ((scan->type != VSTR_TYPE_NODE_NON) && !buf)
      return (1);
    
    if (buf)
    {
      if ((ret = vstr_nx_wrap_memcmp(scan_str, buf, scan_len)))
        return (ret);
      buf = ((char *)buf) + scan_len;
    }
    
    buf_len -= scan_len;
  } while (buf_len &&
           (scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));

  if (len)
    return (1);
  if (buf_len)
    return (-1);
  
  return (0);
}

/* only do ASCII/binary case comparisons -- regardless of the OS/compiler
 * char set default */
static int vstr__cmp_memcasecmp(const char *str1, const char *str2, size_t len)
{
  while (len)
  {
    unsigned char a = *str1;
    unsigned char b = *str2;

    if (VSTR__IS_ASCII_UPPER(a))
      a = VSTR__TO_ASCII_LOWER(a);
    if (VSTR__IS_ASCII_UPPER(b))
      b = VSTR__TO_ASCII_LOWER(b);
    
    if (a - b)
      return (a - b);
    
    ++str1;
    ++str2;
    --len;
  }
  
 return (0);
}

/* don't include ASCII case when comparing */
int vstr_nx_cmp_case(const Vstr_base *base_1, size_t pos_1, size_t len_1,
                     const Vstr_base *base_2, size_t pos_2, size_t len_2)
{
  Vstr_node *scan_1 = NULL;
  Vstr_node *scan_2 = NULL;
  unsigned int num_1 = 0;
  unsigned int num_2 = 0;
  char *scan_str_1 = NULL;
  char *scan_str_2 = NULL;
  size_t scan_len_1 = 0;
  size_t scan_len_2 = 0;
  
  scan_1 = vstr__base_scan_fwd_beg(base_1, pos_1, &len_1, &num_1,
                                   &scan_str_1, &scan_len_1);
  scan_2 = vstr__base_scan_fwd_beg(base_2, pos_2, &len_2, &num_2,
                                   &scan_str_2, &scan_len_2);
  if (!scan_1 && !scan_2)
    return (0);
  if (!scan_1)
    return (-1);
  if (!scan_2)
    return (1);
  
  do
  {
    size_t tmp = scan_len_1;
    if (tmp > scan_len_2)
      tmp = scan_len_2;
    
    if ((scan_1->type != VSTR_TYPE_NODE_NON) &&
        (scan_2->type != VSTR_TYPE_NODE_NON))
    {
      int ret = vstr__cmp_memcasecmp(scan_str_1, scan_str_2, tmp);
      if (ret)
        return (ret);
      scan_str_1 += tmp;
      scan_str_2 += tmp;
    }
    else if (scan_1->type != VSTR_TYPE_NODE_NON)
      return (1);
    else if (scan_2->type != VSTR_TYPE_NODE_NON)
      return (-1);
    
    scan_len_1 -= tmp;
    scan_len_2 -= tmp;
    
    assert(!scan_len_1 || !scan_len_2);
    if (!scan_len_1)
      scan_1 = vstr__base_scan_fwd_nxt(base_1, &len_1, &num_1,
                                       scan_1, &scan_str_1, &scan_len_1);
    if (!scan_len_2)
      scan_2 = vstr__base_scan_fwd_nxt(base_2, &len_2, &num_2,
                                       scan_2, &scan_str_2, &scan_len_2);
  } while (scan_1 && scan_2);
  
  if (scan_1)
    return (1);
  if (scan_2)
    return (-1);
  
  return (0);
}

int vstr_nx_cmp_case_buf(const Vstr_base *base, size_t pos, size_t len,
                         const char *buf, size_t buf_len)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan && !buf_len)
    return (0);
  if (!scan)
    return (-1);
  if (!buf_len)
    return (1);
  
  do
  {
    int ret = 0;
    
    if (scan_len > buf_len)
    {
      scan_len = buf_len;
      len += !len; /* just need enough for test at end, Ie. 1 when len == 0 */
    }
    
    if ((scan->type == VSTR_TYPE_NODE_NON) && buf)
      return (-1);
    if ((scan->type != VSTR_TYPE_NODE_NON) && !buf)
      return (1);
    
    if (buf)
    {
      if ((ret = vstr__cmp_memcasecmp(scan_str, buf, scan_len)))
        return (ret);
      buf += scan_len;
    }
    
    buf_len -= scan_len;
  } while (buf_len &&
           (scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));
  
  if (len)
    return (1);
  if (buf_len)
    return (-1);
  
  return (0);
}

#define VSTR__CMP_BAD (-1)

#define VSTR__CMP_NORM 0
#define VSTR__CMP_NUMB 1
#define VSTR__CMP_FRAC 2

#define VSTR__CMP_NON_MAIN_LOOP 3

#define VSTR__CMP_LEN_POS 4 /* return positive if scan_1 length is longer */
#define VSTR__CMP_LEN_NEG 8 /* return negative if scan_1 length is longer */
#define VSTR__CMP_DONE 9

static int vstr__cmp_vers(const char *scan_str_1,
                          const char *scan_str_2, size_t len,
                          int state, int *difference)
{
 int diff = 0;

 while ((state < VSTR__CMP_NON_MAIN_LOOP) &&
        len && !(diff = *scan_str_1 - *scan_str_2))
 {
  switch (state)
  {
   case VSTR__CMP_NORM:
     if (VSTR__IS_ASCII_DIGIT(*scan_str_1))
       state = VSTR__CMP_NUMB;
     if (*scan_str_1 == VSTR__ASCII_DIGIT_0())
     {
      assert(state == VSTR__CMP_NUMB);
      ++state;
      assert(state == VSTR__CMP_FRAC);
     }
     break;
   case VSTR__CMP_FRAC:
     if (VSTR__IS_ASCII_DIGIT(*scan_str_1) &&
         (*scan_str_1 != VSTR__ASCII_DIGIT_0()))
       state = VSTR__CMP_NUMB;
   case VSTR__CMP_NUMB:
     if (!VSTR__IS_ASCII_DIGIT(*scan_str_1))
       state = VSTR__CMP_NORM;
     break;
     
   default:
     state = VSTR__CMP_NORM;
     assert(FALSE);
     break;
   }
  
  ++scan_str_1;
  ++scan_str_2;
  
  --len;
 }

 if (diff)
 {
  int new_state = VSTR__CMP_BAD;
  
  assert(len);

  *difference = diff;

  switch (state)
  {
   case VSTR__CMP_NORM:
     if (VSTR__IS_ASCII_DIGIT(*scan_str_1) &&
         (*scan_str_1 != VSTR__ASCII_DIGIT_0()) &&
         VSTR__IS_ASCII_DIGIT(*scan_str_2) &&
         (*scan_str_2 != VSTR__ASCII_DIGIT_0()))
       state = VSTR__CMP_NUMB;
     break;
   case VSTR__CMP_FRAC:
   case VSTR__CMP_NUMB:
     if (!VSTR__IS_ASCII_DIGIT(*scan_str_1) && !VSTR__IS_ASCII_DIGIT(*scan_str_2))
       state = VSTR__CMP_NORM;
     break;
     
   default:
     state = VSTR__CMP_NORM;
     assert(FALSE);
     break;
  }
  
  if (state == VSTR__CMP_NORM)
    return (VSTR__CMP_DONE);
  
  assert((state == VSTR__CMP_NUMB) ||
         (state == VSTR__CMP_FRAC));
  
  /* if a string is longer return positive or negative ignoring difference */
  new_state = state << 2;
  
  assert(((state == VSTR__CMP_NUMB) && (new_state == VSTR__CMP_LEN_POS)) ||
         ((state == VSTR__CMP_FRAC) && (new_state == VSTR__CMP_LEN_NEG)) ||
         FALSE);
  
  state = new_state;
 }

 if (state >= VSTR__CMP_NON_MAIN_LOOP)
 {
  assert((state == VSTR__CMP_LEN_POS) ||
         (state == VSTR__CMP_LEN_NEG));

  while (len &&
         VSTR__IS_ASCII_DIGIT(*scan_str_1) &&
         VSTR__IS_ASCII_DIGIT(*scan_str_2))
  {
   ++scan_str_1;
   ++scan_str_2;
   
   --len;
  }
  
  if (len)
  {
   assert((VSTR__CMP_LEN_POS + 1) < VSTR__CMP_LEN_NEG);
   
   if (VSTR__IS_ASCII_DIGIT(*scan_str_1))
     *difference = ((-state) + VSTR__CMP_LEN_POS + 1);
   if (VSTR__IS_ASCII_DIGIT(*scan_str_2))
     *difference = (state - VSTR__CMP_LEN_POS - 1);
   /* if both are the same length then use the initial stored difference */
   
   return (VSTR__CMP_DONE);
  }
 }
 
 return (state);
}

/* Compare strings while treating digits characters numerically. *
 * However digits starting with a 0 are clasified as fractional (Ie. 0.x)
 */
int vstr_nx_cmp_vers(const Vstr_base *base_1, size_t pos_1, size_t len_1,
                     const Vstr_base *base_2, size_t pos_2, size_t len_2)
{
 Vstr_node *scan_1 = NULL;
 Vstr_node *scan_2 = NULL;
 unsigned int num_1 = 0;
 unsigned int num_2 = 0;
 char *scan_str_1 = NULL;
 char *scan_str_2 = NULL;
 size_t scan_len_1 = 0;
 size_t scan_len_2 = 0;
 int state = VSTR__CMP_NORM;
 int ret = 0;
 
 scan_1 = vstr__base_scan_fwd_beg(base_1, pos_1, &len_1, &num_1,
                                  &scan_str_1, &scan_len_1);
 scan_2 = vstr__base_scan_fwd_beg(base_2, pos_2, &len_2, &num_2,
                                  &scan_str_2, &scan_len_2);
 if (!scan_1 && !scan_2)
   return (0);
 if (!scan_1)
   return (-1);
 if (!scan_2)
   return (1);

 do
 {
   size_t tmp = scan_len_1;
   if (tmp > scan_len_2)
     tmp = scan_len_2;
  
   if ((scan_1->type != VSTR_TYPE_NODE_NON) &&
       (scan_2->type != VSTR_TYPE_NODE_NON))
   {
     state = vstr__cmp_vers(scan_str_1, scan_str_2, tmp, state, &ret);
     if (state == VSTR__CMP_DONE)
       return (ret);
     scan_str_1 += tmp;
     scan_str_2 += tmp;
   }
   else if (scan_1->type != VSTR_TYPE_NODE_NON)
     goto scan_1_longer;
   else if (scan_2->type != VSTR_TYPE_NODE_NON)
     goto scan_2_longer;
   
   scan_len_1 -= tmp;
   scan_len_2 -= tmp;
   
   assert(!scan_len_1 || !scan_len_2);
   if (!scan_len_1)
     scan_1 = vstr__base_scan_fwd_nxt(base_1, &len_1, &num_1,
                                      scan_1, &scan_str_1, &scan_len_1);
   if (!scan_len_2)
     scan_2 = vstr__base_scan_fwd_nxt(base_2, &len_2, &num_2,
                                      scan_2, &scan_str_2, &scan_len_2);
 } while (scan_1 && scan_2);
 
 if (scan_1)
   goto scan_1_longer;
 if (scan_2)
   goto scan_2_longer;
 
 return (ret); /* same length, might have been different at a previous point */

 scan_1_longer:  
 if ((state == VSTR__CMP_FRAC) || (state == VSTR__CMP_LEN_NEG))
   return (-1);
 
 assert((state == VSTR__CMP_NORM) || (state == VSTR__CMP_NUMB) ||
        (state == VSTR__CMP_LEN_POS));
 return (1);
 
 scan_2_longer:  
 if ((state == VSTR__CMP_FRAC) || (state == VSTR__CMP_LEN_NEG))
   return (1);
 
 assert((state == VSTR__CMP_NORM) || (state == VSTR__CMP_NUMB) ||
        (state == VSTR__CMP_LEN_POS));
 return (-1);
}

int vstr_nx_cmp_vers_buf(const Vstr_base *base, size_t pos, size_t len,
                         const char *buf, size_t buf_len)
{
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;
  int state = VSTR__CMP_NORM;
  int ret = 0;

  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  if (!scan && !buf_len)
    return (0);
  if (!scan)
    return (-1);
  if (!buf_len)
    return (1);
  
  do
  {
    if (scan_len > buf_len)
    {
      scan_len = buf_len;
      len += !len; /* just need enough for test at end, Ie. 1 when len == 0 */
    }
    
    if ((scan->type == VSTR_TYPE_NODE_NON) && buf)
      goto scan_2_longer;
    if ((scan->type != VSTR_TYPE_NODE_NON) && !buf)
      goto scan_1_longer;
    
    if (buf)
    {
      state = vstr__cmp_vers(scan_str, buf, scan_len, state, &ret);
      if (state == VSTR__CMP_DONE)
        return (ret);

      buf += scan_len;
    }
    
    buf_len -= scan_len;
  } while (buf_len &&
           (scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                           scan, &scan_str, &scan_len)));
  
  if (len)
    goto scan_1_longer;
  if (buf_len)
    goto scan_2_longer;
  
  return (ret);

 scan_1_longer:  
  if ((state == VSTR__CMP_FRAC) || (state == VSTR__CMP_LEN_NEG))
    return (-1);
  
  assert((state == VSTR__CMP_NORM) || (state == VSTR__CMP_NUMB) ||
         (state == VSTR__CMP_LEN_POS));
  return (1);
  
 scan_2_longer:  
  if ((state == VSTR__CMP_FRAC) || (state == VSTR__CMP_LEN_NEG))
    return (1);
  
  assert((state == VSTR__CMP_NORM) || (state == VSTR__CMP_NUMB) ||
         (state == VSTR__CMP_LEN_POS));
  return (-1);
}

