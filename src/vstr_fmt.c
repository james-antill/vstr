#define VSTR_ADD_FMT_C
/* filename == vstr_fmt.c because CVS doesn't have a rename command
 * should be vstr_add_fmt.c */
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
/* this code is hacked from:
 *   vsprintf in linux/lib -- by Lars Wirzenius <liw@iki.fi>
 * He gave his permission for it to be released as LGPL.
 * ... it's changed a lot since then */
/* functions for portable printf functionality in a vstr */
#include "main.h"

#define vstr__fmt_isdigit(x) (strchr("0123456789", (x)) != NULL)

/* Increasingly misnamed "simple" fprintf like code... */

#define CHAR_TYPE 0
#define SHORT_TYPE 1
#define INT_TYPE 2
#define LONG_TYPE 3
#define LONG_DOUBLE_TYPE 4 /* note L for floats and ll for ints are merged */
#define SIZE_T_TYPE 5
#define PTRDIFF_T_TYPE 6
#define INTMAX_T_TYPE 7

/* 1 byte for the base of the number, can go upto base 255 */
#define BASE_MASK ((1 << 8) - 1)

#define ZEROPAD (1 << 8) /* pad with zero */
#define SIGN (1 << 9) /* unsigned/signed */
#define PLUS (1 << 10) /* show plus sign '+' */
#define SPACE (1 << 11) /* space if plus */
#define LEFT (1 << 12) /* left justified */
#define SPECIAL (1 << 13) /* 0x */
#define LARGE (1 << 14) /* use 'ABCDEF' instead of 'abcdef' */
#define THOUSAND_SEP (1 << 15) /* split at grouping marks according to locale */


#define IS_ZERO (1 << 29) /* is the number zero */
#define IS_NEGATIVE (1 << 30) /* is the number negative */

#define VSTR__CONST_STR_SPACE_64 \
"                                                                "

#define VSTR__CONST_STR_SPACE_256 \
  VSTR__CONST_STR_SPACE_64 VSTR__CONST_STR_SPACE_64 \
  VSTR__CONST_STR_SPACE_64 VSTR__CONST_STR_SPACE_64

#define VSTR__CONST_STR_SPACE_1024 \
  VSTR__CONST_STR_SPACE_256 VSTR__CONST_STR_SPACE_256 \
  VSTR__CONST_STR_SPACE_256 VSTR__CONST_STR_SPACE_256

#define VSTR__CONST_STR_SPACE VSTR__CONST_STR_SPACE_1024

#define VSTR__CONST_STR_ZERO_64 \
"0000000000000000000000000000000000000000000000000000000000000000"

#define VSTR__CONST_STR_ZERO_256 \
  VSTR__CONST_STR_ZERO_64 VSTR__CONST_STR_ZERO_64 \
  VSTR__CONST_STR_ZERO_64 VSTR__CONST_STR_ZERO_64

#define VSTR__CONST_STR_ZERO_1024 \
  VSTR__CONST_STR_ZERO_256 VSTR__CONST_STR_ZERO_256 \
  VSTR__CONST_STR_ZERO_256 VSTR__CONST_STR_ZERO_256

#define VSTR__CONST_STR_ZERO VSTR__CONST_STR_ZERO_1024

#define VSTR__FMT_ADD(x, y, z) vstr_add_buf((x), ((x)->len - pos_diff), y, z)

static int vstr__add_spaces(Vstr_base *base, size_t pos_diff, size_t len)
{
 size_t tmp = CONST_STRLEN(VSTR__CONST_STR_SPACE);
 unsigned int num = 0;
 unsigned int left = 0;

 if (base->end)
   left = (base->conf->buf_sz - base->end->len);
 
 if (len > left)
   num = (len / base->conf->buf_sz) + 1; /* overestimates ... but that's ok */

 if ((base->conf->spare_buf_num < num) &&
     (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, num) != num))
   return (FALSE);

 while (tmp < len)
 {
  VSTR__FMT_ADD(base, VSTR__CONST_STR_SPACE, tmp);
  len -= tmp;
 }

 if (len)
   VSTR__FMT_ADD(base, VSTR__CONST_STR_SPACE, len);
 
 return (TRUE);
}

static int vstr__add_zeros(Vstr_base *base, size_t pos_diff, size_t len)
{
 size_t tmp = CONST_STRLEN(VSTR__CONST_STR_ZERO);
 unsigned int num = 0;
 unsigned int left = 0;

 if (base->end)
   left = (base->conf->buf_sz - base->end->len);
 
 if (len > left)
   num = (len / base->conf->buf_sz) + 1; /* overestimates ... but that's ok */

 if ((base->conf->spare_buf_num < num) &&
     (vstr_add_spare_nodes(base->conf, VSTR_TYPE_NODE_BUF, num) != num))
   return (FALSE);

 while (tmp < len)
 {
  VSTR__FMT_ADD(base, VSTR__CONST_STR_ZERO, tmp);
  len -= tmp;
 }

 if (len)
   VSTR__FMT_ADD(base, VSTR__CONST_STR_ZERO, len);
 return (TRUE);
}

/* print a number backwards... */
#define VSTR__FMT_ADD_NUM(type) do { \
 type *ptr_num = (type *)passed_num; \
 type num = *ptr_num; \
 \
 if (!buf) { \
   size_t buf_sz = vstr__grouping_num_sz(1 + floor(log10(num)), \
                                         grouping, thou_len); \
   if (buf_sz < sizeof(norm_buf)) \
     buf = norm_buf; \
   else if (!(buf = malloc(buf_sz + 1))) \
     goto failed_alloc; } \
 \
 while (i < (sizeof(norm_buf) - 1)) \
 { \
  unsigned int char_offset = 0; \
  \
  if ((flags & THOUSAND_SEP) && !grp_num) { \
    memcpy(buf + i, loc->thousands_sep, thou_len); \
    i += thou_len; } \
  \
  char_offset = (num % num_base); \
  \
  num /= num_base; \
  buf[i++] = digits[char_offset]; \
  \
  if (!num) \
    break; \
  \
  if ((flags & THOUSAND_SEP) && !grp_num) \
  { if (grouping[1]) ++grouping; grp_num = *grouping; } \
  \
  if ((flags & THOUSAND_SEP) && (grp_num < CHAR_MAX)) --grp_num; \
 } \
} while (FALSE)

/* deals well with INT_MIN */
#define VSTR__FMT_ABS_NUM(unum, snum) do { \
 if (snum < 0) { \
   spec->flags |= IS_NEGATIVE; \
   ++snum; unum = -snum; ++unum; \
 } else \
   unum = snum; \
} while (FALSE)

/* rough estimate of needed size to convert the number to a string */
static int vstr__grouping_num_sz(int num_sz,
                                 const char *grouping, size_t thou_len)
{
  int tmp = num_sz;
  int ret = -thou_len;
  
  while (tmp > 0)
  {
    if ((unsigned char)*grouping >= CHAR_MAX)
      break;

    ret += thou_len;
    tmp -= *grouping;
    
    if (grouping[1])
      ++grouping;
  }

  return (num_sz + ret);
}

static int vstr__add_number(Vstr_base *base, size_t pos_diff,
                            void *passed_num, int int_type,
                            unsigned int size, unsigned int precision,
                            int flags)
{
 char sign = 0;
 /* used to hold the actual number, max == with thousand seperators
  * allow 2 chars every 1 as a quick store */
 char norm_buf[BUF_NUM_TYPE_SZ(uintmax_t) +
               ((BUF_NUM_TYPE_SZ(uintmax_t) / 1) * 2)];
 char *buf = norm_buf;
 unsigned int i = 0;
 int num_base = (flags & BASE_MASK);
 const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
 struct lconv *loc = NULL;
 const char *grouping = NULL;
 unsigned char grp_num = 0;
 size_t thou_len = 0;

 /* hacky spec, if precision == 0 and num ==0, then don't display anything */
 if ((flags & IS_ZERO) && !precision)
   return (TRUE);
 
 if (flags & LARGE)
   digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
 
 if (flags & LEFT)
   flags &= ~ZEROPAD; 
 
 switch (num_base)
 {
  case 10:
  case 16:
  case 8:
    break;
    
  default:
    assert(FALSE);
    return (TRUE);
 }
 
 if (flags & SIGN)
 {
  if (flags & IS_NEGATIVE)
    sign = '-';
  else
  {
   if (flags & PLUS)
     sign = '+';
   else
     if (flags & SPACE)
       sign = ' ';
     else
       ++size;
  }

  if (size)
    --size;
 }
 
 if ((flags & SPECIAL) && size)
 {
  if (num_base == 16)
    size -= 2;
  else
    if (num_base == 8)
      --size;
 }
 
 if (i > precision)
   precision = i; /* upgrade the precision if it is a long number */

 if (size < precision)
   size = 0;
 else
   size -= precision;

 if (sign)
   if (!VSTR__FMT_ADD(base, &sign, 1))
     goto failed_alloc;
 
 if (flags & SPECIAL)
   switch (num_base)
   {
    case 16:
      if (!vstr__add_zeros(base, pos_diff, 1))
        goto failed_alloc;
      if (!VSTR__FMT_ADD(base, (digits + 33), 1))
        goto failed_alloc;
      break;
     
    case 8:
      if (!vstr__add_zeros(base, pos_diff, 1))
        goto failed_alloc;
      /* FALLTHROUGH */
    default:
      break;
   }

 if (!(flags & (ZEROPAD | LEFT)))
   if (size > 0)
   { /* right justify number */
    if (flags & ZEROPAD)
    {
     if (!vstr__add_zeros(base, pos_diff, size))
       goto failed_alloc;
    }
    else
    {
     if (!vstr__add_spaces(base, pos_diff, size))
       goto failed_alloc;
    }
    
    size = 0;
   }

 loc = localeconv();
 grouping = loc->grouping;
 thou_len = strlen(loc->thousands_sep);
 grp_num = *grouping;
 if (!thou_len || (grp_num >= CHAR_MAX) || (grp_num <= 0))
   flags &= ~THOUSAND_SEP; /* don't do it */
 else if ((BUF_NUM_TYPE_SZ(uintmax_t) * thou_len) >= sizeof(norm_buf))
   /* this is a quick check that the number will always fit in,
    * a more complete check is done inside FMT_ADD_NUM() */
   buf = NULL;
 
 switch (int_type)
 {
  case CHAR_TYPE: VSTR__FMT_ADD_NUM(unsigned char); break;
  case SHORT_TYPE: VSTR__FMT_ADD_NUM(unsigned short); break;
  case INT_TYPE: VSTR__FMT_ADD_NUM(unsigned int); break;
  case LONG_TYPE: VSTR__FMT_ADD_NUM(unsigned long); break;
  case LONG_DOUBLE_TYPE: VSTR__FMT_ADD_NUM(unsigned long long); break;
  case SIZE_T_TYPE: VSTR__FMT_ADD_NUM(size_t); break;
    /* ptrdiff_t is actually promoted to intmax_t so the sign/unsign works */
  case PTRDIFF_T_TYPE: assert(FALSE); /* FALLTHROUGH */
  case INTMAX_T_TYPE: VSTR__FMT_ADD_NUM(uintmax_t); break;
  default: assert(FALSE); /* only valid types above */
 }
 
 if (precision > i) /* make number the desired length */
   if (!vstr__add_zeros(base, pos_diff, precision - i))
     goto failed_alloc_post_buf;
 
 while (i-- > 0) /* output number */
   if (!VSTR__FMT_ADD(base, buf + i, 1))
     goto failed_alloc_post_buf;

 if (size > 0) /* left justify number */
   if (!vstr__add_spaces(base, pos_diff, size))
     goto failed_alloc_post_buf;

 if (buf != norm_buf)
   free(buf);
 
 return (TRUE);

 failed_alloc_post_buf:
 if (buf != norm_buf)
   free(buf);
 failed_alloc:
 return (FALSE);
}

struct Vstr__fmt_spec
{
 union Vstr__fmt_sp_un
 {
  unsigned char data_c;
  unsigned short data_s;
  unsigned int data_i;
  wint_t data_wint;
  unsigned long data_l;
  unsigned long long data_L;
  size_t data_sz;
  uintmax_t data_m;
  
  double data_d;
  long double data_Ld;

  void *data_ptr;
 } u;
 
 unsigned char fmt_code;
 int num_base;
 int int_type;
 unsigned int flags;
 unsigned int field_width;
 /* min number of digits, after decimal point */
 unsigned int precision;

 unsigned int main_param;
 unsigned int field_width_param;
 unsigned int precision_param;

 /* these two needed for double, not used elsewhere */
 unsigned int precision_usr : 1; /* did the usr specify a precision */
 unsigned int field_width_usr : 1; /* did the usr specify a field width */
 
 struct Vstr__fmt_spec *next;
};

static struct Vstr__fmt_spec *spec_make = NULL;
static struct Vstr__fmt_spec *spec_list_beg = NULL;
static struct Vstr__fmt_spec *spec_list_end = NULL;

#define VSTR__FMT_MV_SPEC(x) do { \
 struct Vstr__fmt_spec *tmp = spec_make; \
 spec_make = tmp->next; \
 if (!spec_list_beg) { \
  spec_list_end = tmp; \
  spec_list_beg = tmp; \
 } else { \
  spec_list_end->next = tmp; \
  spec_list_end = tmp; \
 } \
 if (x && !tmp->main_param) tmp->main_param = ++params; \
 } while (FALSE);

static int vstr__fmt_add_spec(void)
{
 struct Vstr__fmt_spec *spec = NULL;
 
 if (!(spec = malloc(sizeof(struct Vstr__fmt_spec))))
   return (FALSE);
 
 spec->next = spec_make;
 spec_make = spec;
 
 return (TRUE);
}

static void vstr__fmt_init_spec(struct Vstr__fmt_spec *spec)
{
 spec->num_base = 10;
 spec->int_type = INT_TYPE;
 spec->flags = 0;
 spec->field_width = 0;

 spec->main_param = 0;
 spec->field_width_param = 0;
 spec->precision_param = 0;

 spec->next = NULL;
}

#define VSTR__FMT_MK_N_PTR(x, y, z) \
     case x: \
       if (spec->flags & SIGN) \
       { \
        y *len_curr = spec->u.data_ptr; \
        *len_curr = len; \
       } \
       else \
       { \
        z *len_curr = spec->u.data_ptr; \
        *len_curr = len; \
       } \
       break

static int vstr__fmt_write_spec(Vstr_base *base, size_t pos_diff,
                                size_t original_length)
{
  struct Vstr__fmt_spec *spec = spec_list_beg;
  mbstate_t state;
  size_t len_mbs = 0;
  char *buf_mbs = NULL;
  
  while (spec)
  {
    switch (spec->fmt_code)
    {
      case 'c':
        if (!(spec->flags & LEFT))
          if (spec->field_width > 0)
          {
            if (!vstr__add_spaces(base, pos_diff, spec->field_width))
              goto failed_alloc;
            
            spec->field_width = 0;
          }
        
        if (!VSTR__FMT_ADD(base, &spec->u.data_c, 1))
          goto failed_alloc;
        
        if (spec->field_width > 0)
        {
          if (!vstr__add_spaces(base, pos_diff, spec->field_width))
            goto failed_alloc;
          spec->field_width = 0;
        }
        break;
        
      case 'C':
      {
        memset(&state, 0, sizeof(mbstate_t));
        len_mbs = wcrtomb(NULL, spec->u.data_wint, &state);
        if (len_mbs == (size_t)-1)
          goto failed_EILSEQ;
        len_mbs += wcrtomb(NULL, L'\0', &state);
        
        if (!(buf_mbs = malloc(len_mbs)))
        {
          base->conf->malloc_bad = TRUE;
          goto failed_alloc;
        }
        memset(&state, 0, sizeof(mbstate_t));
        len_mbs = wcrtomb(buf_mbs, spec->u.data_wint, &state);
        len_mbs += wcrtomb(buf_mbs + len_mbs, L'\0', &state);
        --len_mbs; /* remove terminator */
        
        if (!(spec->flags & LEFT))
          if (spec->field_width > 0)
          {
            if (!vstr__add_spaces(base, pos_diff, spec->field_width))
              goto C_failed_alloc_free_buf_mbs;
            
            spec->field_width = 0;
          }
        
        if (!VSTR__FMT_ADD(base, buf_mbs, len_mbs))
          goto C_failed_alloc_free_buf_mbs;
        
        if (spec->field_width > 0)
        {
          if (!vstr__add_spaces(base, pos_diff, spec->field_width))
            goto C_failed_alloc_free_buf_mbs;
          spec->field_width = 0;
        }

        free(buf_mbs);
        break;
        
       C_failed_alloc_free_buf_mbs:
        free(buf_mbs);
        goto failed_alloc;
      }
      assert(FALSE);
      break;
      
      case 's':
      {
        size_t len = strnlen(spec->u.data_ptr, spec->precision);
        
        if (spec->field_width > spec->precision)
          spec->field_width = spec->precision;
        
        if (!(spec->flags & LEFT))
          if (spec->field_width > len)
          {
            if (!vstr__add_spaces(base, pos_diff, spec->field_width - len))
              goto failed_alloc;
            spec->field_width = len;
          }

        if (len)
        {
          if (!VSTR__FMT_ADD(base, spec->u.data_ptr, len))
            goto failed_alloc;
        }
        
        if (spec->field_width > len)
          if (!vstr__add_spaces(base, pos_diff, spec->field_width - len))
            goto failed_alloc;
      }
      break;
      
      case 'S':
      { /* note precision is number of wchar_t's allowed through, not bytes
         * as in sprintf() */
        const wchar_t *tmp = spec->u.data_ptr;
        
        memset(&state, 0, sizeof(mbstate_t));

        len_mbs = wcsnrtombs(NULL, &tmp, spec->precision, 0, &state);
        if (len_mbs == (size_t)-1)
          goto failed_EILSEQ;
        if (tmp) /* wcslen() > spec->precision */
        {
          size_t tmp_mbs = wcrtomb(NULL, L'\0', &state);

          if (tmp_mbs == (size_t)-1)
            goto failed_EILSEQ; /* don't think this can happen ? */
          
          len_mbs += tmp_mbs; /* include '\0' */
        }
        else
          ++len_mbs;
        
        if (!(buf_mbs = malloc(len_mbs)))
        {
          base->conf->malloc_bad = TRUE;
          goto failed_alloc;
        }
        tmp = spec->u.data_ptr;
        memset(&state, 0, sizeof(mbstate_t));
        len_mbs = wcsnrtombs(buf_mbs, &tmp, spec->precision, len_mbs, &state);
        if (tmp) /* wcslen() > spec->precision */
        {
          size_t tmp_mbs = wcrtomb(buf_mbs, L'\0', &state);
          len_mbs += tmp_mbs - 1; /* ignore '\0' */
        }
        
        if (spec->field_width > spec->precision)
          spec->field_width = spec->precision;
        
        if (!(spec->flags & LEFT))
          if (spec->field_width > len_mbs)
          {
            if (!vstr__add_spaces(base, pos_diff, spec->field_width - len_mbs))
              goto S_failed_alloc_free_buf_mbs;
            spec->field_width = len_mbs;
          }
        
        if (!VSTR__FMT_ADD(base, buf_mbs, len_mbs))
          goto S_failed_alloc_free_buf_mbs;
        
        if (spec->field_width > len_mbs)
          if (!vstr__add_spaces(base, pos_diff, spec->field_width - len_mbs))
            goto S_failed_alloc_free_buf_mbs;
        
        free(buf_mbs);
        break;

       S_failed_alloc_free_buf_mbs:
        free(buf_mbs);
        goto failed_alloc;
      }
      assert(FALSE);
      break;
        
      case 'd':
      case 'i':
      case 'u':
      case 'X':
      case 'x':
      case 'p':
        /* the addresses should all be identical... assert() that and do it */
#define VSTR__FMT_A(x) \
 (offsetof(union Vstr__fmt_sp_un, data_c) == \
 (offsetof(union Vstr__fmt_sp_un, x)))
        
        assert(VSTR__FMT_A(data_s) &&
               VSTR__FMT_A(data_i) &&
               VSTR__FMT_A(data_wint) &&
               VSTR__FMT_A(data_l) &&
               VSTR__FMT_A(data_L) &&
               VSTR__FMT_A(data_sz) &&
               VSTR__FMT_A(data_m) &&
               VSTR__FMT_A(data_d) &&
               VSTR__FMT_A(data_Ld) &&
               VSTR__FMT_A(data_ptr) &&
               TRUE);
#undef VSTR__FMT_A
        
        if (!vstr__add_number(base, pos_diff, &spec->u, spec->int_type,
                              spec->field_width, spec->precision,
                              spec->num_base | spec->flags))
          goto failed_alloc;
        break;
        
      case 'n':
      {
        size_t len = (base->len - original_length);
        
        switch (spec->int_type)
        {
          VSTR__FMT_MK_N_PTR(CHAR_TYPE, signed char, unsigned char);
          VSTR__FMT_MK_N_PTR(SHORT_TYPE, signed short, unsigned short);
          VSTR__FMT_MK_N_PTR(INT_TYPE, signed int, unsigned int);
          VSTR__FMT_MK_N_PTR(LONG_TYPE, signed long, unsigned long);
          VSTR__FMT_MK_N_PTR(LONG_DOUBLE_TYPE,
                             signed long long, unsigned long long);
          VSTR__FMT_MK_N_PTR(SIZE_T_TYPE, ssize_t, size_t);
          VSTR__FMT_MK_N_PTR(INTMAX_T_TYPE, intmax_t, uintmax_t);
          
          case PTRDIFF_T_TYPE:
            if (1)
            { /* no unsigned version ... */
              ptrdiff_t *len_curr = spec->u.data_ptr;
              *len_curr = len;
            }
            break;
            
          default:
            assert(FALSE);
        }
      }
      break;
      
      /* floats ... hey you can't _work_ with fp portably, you expect me to
       * write a portable fp decoder :p :P */
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
      case 'a':
      case 'A':
      {
        char fmt_buffer[13];
        char *float_buffer = NULL;
        unsigned int tmp = 1;
        int ret = -1;
        
        fmt_buffer[0] = '%';
        if (spec->flags & LEFT)
          fmt_buffer[tmp++] = '-';
        
        if (spec->flags & PLUS)
          fmt_buffer[tmp++] = '+';
        
        if (spec->flags & SPACE)
          fmt_buffer[tmp++] = ' ';
        
        if (spec->flags & SPECIAL)
          fmt_buffer[tmp++] = '#';
        
        if (spec->flags & ZEROPAD)
          fmt_buffer[tmp++] = '0';

        if (spec->flags & THOUSAND_SEP)
          fmt_buffer[tmp++] = '\'';

        if (spec->field_width_usr)
          fmt_buffer[tmp++] = '*';

        if (spec->precision_usr)
        {
          fmt_buffer[tmp++] = '.';
          fmt_buffer[tmp++] = '*';
        }
        
        if (spec->int_type == LONG_DOUBLE_TYPE)
          fmt_buffer[tmp++] = 'L';
        
        fmt_buffer[tmp++] = spec->fmt_code;
        assert(tmp <= sizeof(fmt_buffer));
        fmt_buffer[tmp] = 0;
        
        if (spec->int_type == LONG_DOUBLE_TYPE)
        {
          if (spec->field_width_usr && spec->precision_usr)
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->field_width, spec->precision, spec->u.data_Ld);
          else if (spec->field_width_usr)
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->field_width, spec->u.data_Ld);
          else if (spec->precision_usr)
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->precision, spec->u.data_Ld);
          else
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->u.data_Ld);
        }
        else
        {
          if (spec->field_width_usr && spec->precision_usr)
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->field_width, spec->precision, spec->u.data_d);
          else if (spec->field_width_usr)
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->field_width, spec->u.data_d);
          else if (spec->precision_usr)
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->precision, spec->u.data_d);
          else
            ret = asprintf(&float_buffer, fmt_buffer,
                           spec->u.data_d);
        }
        
        if ((ret > 0) || !VSTR__FMT_ADD(base, float_buffer, ret))
        {
          free(float_buffer);
          goto failed_alloc;
        }
        
        free(float_buffer);
      }
      break;
      
      default:
        assert(FALSE);
    }
    
    spec = spec->next;
  }
 
  return (TRUE);
  
 failed_alloc:
 failed_EILSEQ:
  return (FALSE);
}

#undef VSTR__FMT_MK_N_PTR
#define VSTR__FMT_MK_N_PTR(x, y, z) \
          case x: \
            if (spec->flags & SIGN) \
              u.data_ptr = va_arg(ap, y *); \
            else \
              u.data_ptr = va_arg(ap, z *); \
            break

static int vstr__fmt_fillin_spec(va_list ap, int have_dollars)
{
 struct Vstr__fmt_spec *beg = spec_list_beg;
 unsigned int count = 0;
 unsigned int need_to_fin_now = FALSE;
 
 while (beg)
 {
  struct Vstr__fmt_spec *spec = NULL;
  int done = FALSE;
  union Vstr__fmt_sp_un u;
  
  ++count;
  assert(beg);

  while (beg &&
         !beg->field_width_param &&
         !beg->precision_param &&
         !beg->main_param)
    beg = beg->next;

  if (beg && need_to_fin_now)
  {
   assert(FALSE);
   return (FALSE);
  }
  
  spec = beg;
  while (spec)
  {
   if (count == spec->field_width_param)
   {
    if (!done)
      u.data_i = va_arg(ap, int);
    done = TRUE;
    spec->field_width = u.data_i;
    spec->field_width_param = 0;
    if (!have_dollars)
      break;
   }
   
   if (count == spec->precision_param)
   {
    if (!done)
      u.data_i = va_arg(ap, int);
    done = TRUE;
    spec->precision = u.data_i;
    spec->precision_param = 0;
    if (!have_dollars)
      break;
   }
   
   if (count == spec->main_param)
   {
    if (!done)
      switch (spec->fmt_code)
      {
         /* could allow case 'm' for errno, but it's crapply defined in glibc to work
          * like syslog() ... Ie. the errno value isn't passed as an arg, so we have
          * to hope that the value of errno as we entered the function is correct
          * people can just do "%s", strerror(errno) and live with it */

        case 'c':
          u.data_c = va_arg(ap, int);
          break;
          
        case 'C':
          u.data_wint = va_arg(ap, wint_t);
          break;
         
        case 's':
          u.data_ptr = va_arg(ap, char *);
          
          assert(u.data_ptr);
          if (!u.data_ptr)
            u.data_ptr = (char *)"(null)";
          break;
          
        case 'S':
          u.data_ptr = va_arg(ap, wchar_t *);
          
          assert(u.data_ptr);
          if (!u.data_ptr)
            u.data_ptr = (wchar_t *)(L"(null)");
          break;
       
       case 'd':
         switch (spec->int_type)
         {
          case CHAR_TYPE:
            if (spec->flags & SIGN)
            {
             signed char tmp = (signed char) va_arg(ap, int);
             VSTR__FMT_ABS_NUM(u.data_c, tmp);
            }
            else
              u.data_c = (unsigned char) va_arg(ap, unsigned int);
            if (!u.data_c) spec->flags |= IS_ZERO; 
            break;
          case SHORT_TYPE:
            if (spec->flags & SIGN)
            {
             signed short tmp = (signed short) va_arg(ap, int);
             VSTR__FMT_ABS_NUM(u.data_s, tmp);
            }
            else
              u.data_s = (unsigned short) va_arg(ap, unsigned int);
            if (!u.data_s) spec->flags |= IS_ZERO;
            break;
          case INT_TYPE:
            if (spec->flags & SIGN)
            {
             int tmp = va_arg(ap, int);
             VSTR__FMT_ABS_NUM(u.data_i, tmp);
            }
            else
              u.data_i = va_arg(ap, unsigned int);
            if (!u.data_i) spec->flags |= IS_ZERO;
            break;
          case LONG_TYPE:
            if (spec->flags & SIGN)
            {
             long tmp = va_arg(ap, long);
             VSTR__FMT_ABS_NUM(u.data_l, tmp);
            }
            else
              u.data_l = va_arg(ap, unsigned long);
            if (!u.data_l) spec->flags |= IS_ZERO;
            break;
          case LONG_DOUBLE_TYPE:
            if (spec->flags & SIGN)
            {
             long long tmp = va_arg(ap, long long);
             VSTR__FMT_ABS_NUM(u.data_L, tmp);
            }
            else
              u.data_L = va_arg(ap, unsigned long long);
            if (!u.data_L) spec->flags |= IS_ZERO;
            break;
          case SIZE_T_TYPE:
            if (spec->flags & SIGN)
            {
             ssize_t tmp = va_arg(ap, ssize_t);
             VSTR__FMT_ABS_NUM(u.data_sz, tmp);
            }
            else
              u.data_sz = va_arg(ap, size_t);
            if (!u.data_sz) spec->flags |= IS_ZERO;
            break;
          case PTRDIFF_T_TYPE:
            if (1)
            { /* no unsigned version ... */
             intmax_t tmp = va_arg(ap, ptrdiff_t);
             VSTR__FMT_ABS_NUM(u.data_m, tmp);
            }
            if (!u.data_m) spec->flags |= IS_ZERO;
            spec->int_type = INTMAX_T_TYPE;
            break;
          case INTMAX_T_TYPE:
            if (spec->flags & SIGN)
            {
             intmax_t tmp = va_arg(ap, intmax_t);
             VSTR__FMT_ABS_NUM(u.data_m, tmp);
            }
            else
              u.data_m = va_arg(ap, uintmax_t);
            if (!u.data_m) spec->flags |= IS_ZERO;
            break;
            
          default:
            assert(FALSE);
         }
         break;
         
       case 'p':
       {
        void *tmp = va_arg(ap, void *);
        u.data_m = (uintmax_t) tmp; /* warning ... *sigh* */
       }
       break;
         
       case 'n':
         switch (spec->int_type)
         {
          VSTR__FMT_MK_N_PTR(CHAR_TYPE, signed char, unsigned char);
          VSTR__FMT_MK_N_PTR(SHORT_TYPE, signed short, unsigned short);
          VSTR__FMT_MK_N_PTR(INT_TYPE, signed int, unsigned int);
          VSTR__FMT_MK_N_PTR(LONG_TYPE, signed long, unsigned long);
          VSTR__FMT_MK_N_PTR(LONG_DOUBLE_TYPE,
                             signed long long, unsigned long long);
          VSTR__FMT_MK_N_PTR(SIZE_T_TYPE, ssize_t, size_t);
          VSTR__FMT_MK_N_PTR(INTMAX_T_TYPE, intmax_t, uintmax_t);
            
          case PTRDIFF_T_TYPE:
            u.data_ptr = va_arg(ap, ptrdiff_t *);
            break;
            
          default:
            assert(FALSE);
         }
         break;
         
         /* floats ... hey you can't _work_ with fp portably, you expect me to write
          * a portable fp decoder :p :P */
       case 'e': /* print like [-]x.xxxexx */
       case 'E': /* use big E instead */
       case 'f': /* print like an int */
       case 'F': /* print like an int - upper case infinity/nan */
       case 'g': /* use the smallest of e and f */
       case 'G': /* use the smallest of E and F */
       case 'a': /* print like [-]x.xxxpxx -- in hex using abcdef */
       case 'A': /* print like [-]x.xxxPxx -- in hex using ABCDEF */
         if (spec->int_type == LONG_DOUBLE_TYPE)
           u.data_Ld = va_arg(ap, long double);
         else
           u.data_d = va_arg(ap, double);
         break;
      }
    done = TRUE;
    spec->u = u;
    spec->main_param = 0;
    
    if (!have_dollars)
      break;
   }

   spec = spec->next;
  }

  if (!done)
    need_to_fin_now = 1;
 }

 return (TRUE);
}

size_t vstr_add_vfmt(Vstr_base *base, size_t pos, const char *fmt, va_list ap)
{
 size_t original_length = base->len; /* starting size */
 size_t pos_diff = 0;
 unsigned int params = 0;
 unsigned int have_dollars = 0; /* have posix %2$d etc. stuff */
   
 if (!base || !fmt || !*fmt || (pos > base->len) || base->conf->malloc_bad)
   return (0);

 pos_diff = base->len - pos;
 /* this will be correct as you add chars */
 assert((base->len - pos_diff) == pos);
 
 for (; *fmt ; ++fmt)
 {
  unsigned int str_precision = UINT_MAX; /* max number of chars */
  unsigned int numb_precision = 1;
  int tmp_num = 0;
  const char *fmt_orig = fmt;
  
  if (!spec_make && !vstr__fmt_add_spec())
    goto failed_alloc;

  vstr__fmt_init_spec(spec_make);
  
  if (*fmt != '%')
  {
   char *next_percent = strchr(fmt, '%');
   size_t len = 0;
   
   if (!next_percent)
     len = strlen(fmt);
   else
     len = next_percent - fmt;

   spec_make->u.data_ptr = (char *)fmt;
   spec_make->precision = len;
   spec_make->fmt_code = 's';
   VSTR__FMT_MV_SPEC(FALSE);
   
   fmt = fmt + len - 1;
   continue;
  }
  
  ++fmt; /* this skips first the '%' */
  if (*fmt == '%')
  {
   spec_make->u.data_c = '%';
   spec_make->fmt_code = 'c';
   VSTR__FMT_MV_SPEC(FALSE);
   continue;
  }
  
  /* process flags */

  while (TRUE)
  {
   switch (*fmt)
   {
    case '-': spec_make->flags |= LEFT; break;
    case '+': spec_make->flags |= PLUS; break;
    case ' ': spec_make->flags |= SPACE; break;
    case '#': spec_make->flags |= SPECIAL; break;
    case '0': spec_make->flags |= ZEROPAD; break;
      /* in POSIX it shouldn't do anything ... so we can only enable this
       * in locales */
    case '\'': spec_make->flags |= THOUSAND_SEP; break;
      
    default:
      goto got_flags;
   }
   ++fmt;
  }
 got_flags:

  /* get i18n param number */   
  if (vstr__fmt_isdigit((unsigned char) *fmt))
  {
   tmp_num = strtol(fmt, (char **) &fmt, 10);
   
   if (*fmt != '$')
     goto use_field_width;
   
   ++fmt;
   have_dollars = 1;
   spec_make->main_param = tmp_num;
  }
  
  /* get field width */   
  if (vstr__fmt_isdigit((unsigned char) *fmt))
  {
    spec_make->field_width_usr = TRUE;
    
    tmp_num = strtol(fmt, (char **) &fmt, 10);
    
   use_field_width:
    if (tmp_num < 0)
    {
      ++tmp_num;
      spec_make->field_width = -tmp_num;
      ++spec_make->field_width;
      spec_make->flags |= LEFT;
    }
    else
      spec_make->field_width = tmp_num;
  }
  else if (*fmt == '*')
  {
    const char *dollar_start = fmt;

    spec_make->field_width_usr = TRUE;

    ++fmt;
    if (vstr__fmt_isdigit((unsigned char) *fmt))
    {
      tmp_num = strtol(fmt, (char **) &fmt, 10);
      
      if (*fmt != '$')
      {
        fmt = dollar_start + 1;
        spec_make->field_width_param = ++params;
      }
      else
      {
        ++fmt;
        spec_make->field_width_param = tmp_num;
      }
    }
  }
  
  /* get the precision */
  if (*fmt == '.')
  {
    spec_make->precision_usr = TRUE;
    
    ++fmt;
    if (vstr__fmt_isdigit((unsigned char) *fmt))
    {
      tmp_num = strtol(fmt, (char**) &fmt, 10); /* warning */
      if (tmp_num < 0) /* check to make sure things go nicely */
        tmp_num = 0;
      
      numb_precision = str_precision = tmp_num;
    }
    else if (*fmt == '*')
    {
      const char *dollar_start = fmt;
      ++fmt;
      if (vstr__fmt_isdigit((unsigned char) *fmt))
      {
        tmp_num = strtol(fmt, (char **) &fmt, 10);
        
        if (*fmt != '$')
        {
          fmt = dollar_start + 1;
          spec_make->precision_param = ++params;
        }
        else
        {
          ++fmt;
          spec_make->precision_param = tmp_num;
        }
      }
    }
    else
      numb_precision = str_precision = 0;
  }
  
  /* get width of type */
  switch (*fmt)
  {
   case 'h':
     ++fmt;
     if (*fmt == 'h')
     {
      ++fmt;
      spec_make->int_type = CHAR_TYPE;
     }
     else
       spec_make->int_type = SHORT_TYPE;
     break;
   case 'l':
     ++fmt;
     if (*fmt == 'l')
     {
      ++fmt;
      spec_make->int_type = LONG_DOUBLE_TYPE;
     }
     else
       spec_make->int_type = LONG_TYPE;
     break;
   case 'L': ++fmt; spec_make->int_type = LONG_DOUBLE_TYPE; break;
   case 'z': ++fmt; spec_make->int_type = SIZE_T_TYPE; break;
   case 't': ++fmt; spec_make->int_type = PTRDIFF_T_TYPE; break;
   case 'j': ++fmt; spec_make->int_type = INTMAX_T_TYPE; break;

   default:
     break;
  }
  
  switch (*fmt)
  {
    case 'c':
      if (spec_make->int_type != LONG_TYPE) /* %lc == %C */
      {
        spec_make->fmt_code = *fmt;
        VSTR__FMT_MV_SPEC(TRUE);
        continue;
      }
      /* FALL THROUGH */
      
    case 'C':
      spec_make->fmt_code = 'C';
      VSTR__FMT_MV_SPEC(TRUE);
      continue;
      
    case 's':
      if (spec_make->int_type != LONG_TYPE) /* %ls == %S */
      {
        spec_make->precision = str_precision;
        spec_make->fmt_code = *fmt;
        VSTR__FMT_MV_SPEC(TRUE);
        continue;
      }
      /* FALL THROUGH */
      
    case 'S':
      spec_make->precision = str_precision;
      spec_make->fmt_code = 'S';
      VSTR__FMT_MV_SPEC(TRUE);
      continue;
      
    case 'd':
    case 'i':
      spec_make->flags |= SIGN;
    case 'u':
      break;
      
    case 'X':
      spec_make->flags |= LARGE;
    case 'x':
      spec_make->num_base = 16;
      break;
      
    case 'o':
      spec_make->num_base = 8;
      break;    
      
    case 'p':
      spec_make->num_base = 16;
      spec_make->int_type = INTMAX_T_TYPE;
      spec_make->fmt_code = 'p';
      spec_make->flags |= SPECIAL;
      spec_make->precision = numb_precision; /* FIXME: ok ? */
      VSTR__FMT_MV_SPEC(TRUE);
      continue;
      
    case 'n':
      spec_make->fmt_code = 'n';
      VSTR__FMT_MV_SPEC(TRUE);
      continue;
      
    case 'e': /* print like [-]x.xxxexx */
    case 'E': /* use big E instead */
    case 'f': /* print like an int */
    case 'F': /* print like an int - upper case infinity/nan */
    case 'g': /* use the smallest of e and f */
    case 'G': /* use the smallest of E and F */
    case 'a': /* print like [-]x.xxxpxx -- in hex using abcdef */
    case 'A': /* print like [-]x.xxxPxx -- in hex using ABCDEF */
      spec_make->fmt_code = *fmt;
      spec_make->precision = numb_precision;
      VSTR__FMT_MV_SPEC(TRUE);
      continue;
      
    default:
      assert(FALSE);
      
      vstr__fmt_init_spec(spec_make);
      spec_make->u.data_c = '%';
      spec_make->fmt_code = 'c';
      VSTR__FMT_MV_SPEC(FALSE);
      fmt = fmt_orig;
      continue;
  }

  /* one of: d, i, u, X, x, o */
  spec_make->precision = numb_precision;
  spec_make->fmt_code = 'd';
  VSTR__FMT_MV_SPEC(TRUE);
 }

 if (!vstr__fmt_fillin_spec(ap, have_dollars))
   goto no_format_for_arg;
 
 if (!vstr__fmt_write_spec(base, pos_diff, original_length))
   goto failed_alloc;
 
 spec_list_end->next = spec_make;
 spec_make = spec_list_beg;
 spec_list_beg = NULL;
 spec_list_end = NULL;
 
 return (base->len - original_length);

 failed_alloc:
 vstr_del(base, original_length, base->len - original_length);
 
 no_format_for_arg:
 spec_list_end->next = spec_make;
 spec_make = spec_list_beg;
 spec_list_beg = NULL;
 spec_list_end = NULL;
 
 return (0);
}

size_t vstr_add_fmt(Vstr_base *base, size_t pos, const char *fmt, ...)
{
 size_t len = 0;
 va_list ap;
 
 va_start(ap, fmt);
 
 len = vstr_add_vfmt(base, pos, fmt, ap);
 
 va_end(ap);
 
 return (len);
}
