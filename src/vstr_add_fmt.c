#define VSTR_ADD_FMT_C
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
/* this code is hacked from:
 *   vsprintf in linux/lib -- by Lars Wirzenius <liw@iki.fi>
 * He gave his permission for it to be released as LGPL.
 * ... it's changed a lot since then */
/* functions for portable printf functionality in a vstr */
#include "main.h"

#if !USE_WIDE_CHAR_T
# undef wchar_t
# define wchar_t int /* doesn't matter ... fails out anyway */
# undef wint_t
# define wint_t  int /* doesn't matter ... fails out anyway */
#endif

static struct Vstr__fmt_spec *vstr__fmt_spec_make = NULL;
static struct Vstr__fmt_spec *vstr__fmt_spec_list_beg = NULL;
static struct Vstr__fmt_spec *vstr__fmt_spec_list_end = NULL;


#define VSTR__ADD_FMT_ISDIGIT(x) (((x) >= '0') && ((x) <= '9'))

#define VSTR__ADD_FMT_CHAR2INT_1(x) ((x) - '0')
#define VSTR__ADD_FMT_CHAR2INT_2(x, y) ((((x) - '0') * 10) + \
                                         ((y) - '0'))

/* a very small number of digits is _by far_ the norm */
#define VSTR__ADD_FMT_STRTOL(x) \
 (VSTR__ADD_FMT_ISDIGIT((x)[1]) ? \
  VSTR__ADD_FMT_ISDIGIT((x)[2]) ? strtol(x, (char **) (x), 10) : \
  ((x) += 2, VSTR__ADD_FMT_CHAR2INT_2((x)[-2], (x)[-1])) : \
  ((x) += 1, VSTR__ADD_FMT_CHAR2INT_1((x)[-1])))

/* Increasingly misnamed "simple" fprintf like code... */

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
#define ALT_DIGITS (1 << 16) /* does nothing in core code, can be used in
                              * custom format specifiers */

#define PREC_USR (1 << 28) /* user specified precision */
#define NUM_IS_ZERO (1 << 29) /* is the number zero */
#define NUM_IS_NEGATIVE (1 << 30) /* is the number negative */

#define VSTR__FMT_ADD(x, y, z) vstr_nx_add_buf((x), ((x)->len - pos_diff), y, z)
#define VSTR__FMT_ADD_REP_CHR(x, y, z) \
 vstr_nx_add_rep_chr((x), ((x)->len - pos_diff), y, z)

/* print a number backwards (any base)... */
#define VSTR__FMT_ADD_NUM(type, passed_num) do { \
 type num = passed_num; \
 \
 while (num) \
 { \
  unsigned int char_offset = (num % spec->num_base); \
  \
  assert(i < sizeof(buf)); \
  \
  num /= spec->num_base; \
  buf[sizeof(buf) - ++i] = digits[char_offset]; \
 } \
} while (FALSE)

/* deals well with INT_MIN */
#define VSTR__FMT_S2U_NUM(unum, snum) do { \
   ++snum; unum = -snum; ++unum; \
} while (FALSE)
#define VSTR__FMT_ABS_NUM(unum, snum) do { \
 if (snum < 0) { \
   spec->flags |= NUM_IS_NEGATIVE; \
   VSTR__FMT_S2U_NUM(unum, snum); \
 } else \
   unum = snum; \
} while (FALSE)

static unsigned int vstr__grouping_mod(const char *grouping, unsigned int num)
{
  unsigned int tmp = 0;

  if (!*grouping)
    return (num);
  
  while (((unsigned char)*grouping < SCHAR_MAX) &&
         ((tmp + *grouping) < num))
  {
    tmp += *grouping;
    if (grouping[1])
      ++grouping;
  }
  
  return (num - tmp);
}

static size_t vstr__grouping_num_sz(Vstr_base *base, size_t len)
{
  size_t ret = 0;
  int done = FALSE;
  
  while (len)
  {
    unsigned int num = vstr__grouping_mod(base->conf->loc->grouping, len);

    if (done)
      ret += base->conf->loc->thousands_sep_len;
    
    ret += num;
    assert(num <= len);
    len -= num;
    
    done = TRUE;
  }
  
  return (ret);
}

static int vstr__grouping_add_num(Vstr_base *base, size_t pos_diff,
                                  const char *buf, size_t len)
{
  int done = FALSE;
  
  while (len)
  {
    unsigned int num = vstr__grouping_mod(base->conf->loc->grouping, len);
    
    if (done &&
        !VSTR__FMT_ADD(base, base->conf->loc->thousands_sep_str,
                       base->conf->loc->thousands_sep_len))
      return (FALSE);
    
    if (!VSTR__FMT_ADD(base, buf, num))
      return (FALSE);
    
    buf += num;
    assert(num <= len);
    len -= num;
    
    done = TRUE;
  }

  return (TRUE);
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
#ifdef HAVE_LONG_LONG
  unsigned long long data_L;
#endif
  size_t data_sz;
  uintmax_t data_m;
  
  double data_d;
  long double data_Ld;

  void *data_ptr;
 } u;
 
 unsigned char fmt_code;
 int num_base;
 unsigned int int_type;
 unsigned int flags;
 unsigned int field_width;
 /* min number of digits, after decimal point */
 unsigned int precision;

 unsigned int main_param;
 unsigned int field_width_param;
 unsigned int precision_param;

 Vstr__fmt_usr_name_node *usr_spec;
 
 /* these two needed for double, not used elsewhere */
 /* unsigned int precision_usr : 1; done with the PREC_USR flag */
 unsigned int field_width_usr : 1; /* did the usr specify a field width */
 unsigned int escape_usr : 1;  /* did the usr specify this escape */
 
 struct Vstr__fmt_spec *next;
};

static int vstr__add_fmt_number(Vstr_base *base, size_t pos_diff,
                                struct Vstr__fmt_spec *spec)
{
 char sign = 0;
 /* used to hold the actual number */
 char buf[BUF_NUM_TYPE_SZ(uintmax_t)];
 size_t i = 0;
 size_t real_i = 0;
 const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
 const char *grouping = NULL;
 unsigned char grp_num = 0;
 const char *thou = NULL;
 size_t thou_len = 0;
 size_t max_p_i = 0;
 unsigned int field_width = 0;
 unsigned int precision = 0;

 if (spec->flags & PREC_USR)
   precision = spec->precision;
 else
   precision = 1;
 
 if (spec->field_width_usr)
   field_width = spec->field_width;
 
 /* if the usr specified a precision the '0' flag is ignored */
 if (spec->flags & PREC_USR)
   spec->flags &= ~ZEROPAD;
 
 if (spec->flags & LARGE)
   digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
 
 if (spec->flags & LEFT)
   spec->flags &= ~ZEROPAD;
 
 switch (spec->num_base)
 {
  case 10:
  case 16:
  case 8:
    break;
    
  default:
    assert(FALSE);
    return (TRUE);
 }
 
 if (spec->flags & SIGN)
 {
  if (spec->flags & NUM_IS_NEGATIVE)
    sign = '-';
  else
  {
   if (spec->flags & PLUS)
     sign = '+';
   else
     if (spec->flags & SPACE)
       sign = ' ';
     else
       ++field_width;
  }

  if (field_width) --field_width;
 }
 
 if (spec->flags & SPECIAL)
 {
  if (spec->num_base == 16)
  {
    if (field_width) --field_width;
    if (field_width) --field_width;
  }
  else
    if ((spec->num_base == 8) && field_width)
      --field_width;
 }

 grouping = base->conf->loc->grouping;
 thou_len = base->conf->loc->thousands_sep_len;
 thou = base->conf->loc->thousands_sep_str;
 
 grp_num = *grouping;
 if (!thou_len || (grp_num >= SCHAR_MAX) || (grp_num <= 0))
   spec->flags &= ~THOUSAND_SEP; /* don't do it */
 
 switch (spec->int_type)
 {
   case VSTR_TYPE_FMT_UCHAR:
     VSTR__FMT_ADD_NUM(unsigned char, spec->u.data_c); break;
   case VSTR_TYPE_FMT_USHORT:
     VSTR__FMT_ADD_NUM(unsigned short, spec->u.data_s); break;
   case VSTR_TYPE_FMT_UINT:
     VSTR__FMT_ADD_NUM(unsigned int, spec->u.data_i); break;
   case VSTR_TYPE_FMT_ULONG:
     VSTR__FMT_ADD_NUM(unsigned long, spec->u.data_l); break;
#ifdef HAVE_LONG_LONG
   case VSTR_TYPE_FMT_ULONG_LONG:
     VSTR__FMT_ADD_NUM(unsigned long long, spec->u.data_L); break;
#endif
   case VSTR_TYPE_FMT_SIZE_T:
     VSTR__FMT_ADD_NUM(size_t, spec->u.data_sz); break;
     /* ptrdiff_t is actually promoted to intmax_t so the sign/unsign works */
   case VSTR_TYPE_FMT_PTRDIFF_T: assert(FALSE); /* FALLTHROUGH */
   case VSTR_TYPE_FMT_UINTMAX_T:
     VSTR__FMT_ADD_NUM(uintmax_t, spec->u.data_m); break;
   default: assert(FALSE); /* only valid types above */
 }

 real_i = i;
 if (spec->flags & THOUSAND_SEP)
   real_i = vstr__grouping_num_sz(base, i);

 if (real_i > precision)
   max_p_i = real_i;
 else
   max_p_i = precision;
 
 if (field_width < max_p_i)
   field_width = 0;
 else
   field_width -= max_p_i;

 if (!(spec->flags & (ZEROPAD | LEFT)))
   if (field_width > 0)
   { /* right justify number, with spaces -- zeros done after sign/spacials */
     if (!VSTR__FMT_ADD_REP_CHR(base, ' ', field_width))
       goto failed_alloc;
     field_width = 0;
   }
 
 if (sign)
   if (!VSTR__FMT_ADD(base, &sign, 1))
     goto failed_alloc;
 
 if (spec->flags & SPECIAL)
 {
   if (spec->num_base == 16)
   {
     /* only append 0x if it is a non-zero value, but not if precision == 0 */
     if (!(spec->flags & NUM_IS_ZERO) && precision)
     {
       if (!VSTR__FMT_ADD_REP_CHR(base, '0', 1))
         goto failed_alloc;
       if (!VSTR__FMT_ADD(base, (digits + 33), 1))
         goto failed_alloc;
     }
   }
   /* hacky spec, if octal then we up the precision so that a 0 is printed as
    * the first character -- even if num == 0 and precision == 0 */
   if ((spec->num_base == 8) && !field_width &&
       (((spec->flags & NUM_IS_ZERO) && !precision) ||
        ((buf[sizeof(buf) - i] != '0') && (precision <= real_i))) &&
       (spec->flags & SPECIAL))
     precision = real_i + 1;
 }

 if (!(spec->flags & LEFT))
   if (field_width > 0)
   { /* right justify number, with zeros */
     assert(spec->flags & ZEROPAD);
     if (!VSTR__FMT_ADD_REP_CHR(base, '0', field_width))
       goto failed_alloc;
     field_width = 0;
   }
 
 if (precision > real_i) /* make number the desired length */
 {
   if (!VSTR__FMT_ADD_REP_CHR(base, '0', precision - real_i))
     goto failed_alloc;
 }

 if (i)
 {
   /* output number */
   if (spec->flags & THOUSAND_SEP)
   {
     if (!vstr__grouping_add_num(base, pos_diff, buf + sizeof(buf) - i, i))
       goto failed_alloc;
   }
   else
   {
     if (!VSTR__FMT_ADD(base, buf + sizeof(buf) - i, i))
       goto failed_alloc;
   }
 }
 
 if (field_width > 0)
 {
   assert(spec->flags & LEFT);
   if (!VSTR__FMT_ADD_REP_CHR(base, ' ', field_width))
     goto failed_alloc;
 }
 
 return (TRUE);

 failed_alloc:
 return (FALSE);
}

void vstr__add_fmt_cleanup_spec(void)
{ /* only done so leak detection is easier */
 struct Vstr__fmt_spec *scan = vstr__fmt_spec_make;

 assert(!vstr__fmt_spec_list_beg && !vstr__fmt_spec_list_beg);
 
 while (scan)
 {
   struct Vstr__fmt_spec *scan_next = scan->next;
   free(scan);
   scan = scan_next;
 }

 vstr__fmt_spec_make = NULL;
}

static int vstr__fmt_add_spec(void)
{
  struct Vstr__fmt_spec *spec = NULL;
  
  if (!(spec = malloc(sizeof(struct Vstr__fmt_spec))))
    return (FALSE);
  
  spec->next = vstr__fmt_spec_make;
  vstr__fmt_spec_make = spec;
  
  return (TRUE);
}

#define VSTR__FMT_MV_SPEC(x) vstr__fmt_mv_spec(spec, x, &params)
static void vstr__fmt_mv_spec(struct Vstr__fmt_spec *spec,
                              int main_param, int *params)
{ 
 vstr__fmt_spec_make = spec->next;

 if (!vstr__fmt_spec_list_beg)
 {
   vstr__fmt_spec_list_end = spec;
   vstr__fmt_spec_list_beg = spec;
 }
 else
 {
  vstr__fmt_spec_list_end->next = spec;
  vstr__fmt_spec_list_end = spec;
 }

 spec->next = NULL;

 /* does this count towards users $x numbers */
 if (main_param && !spec->main_param)
   spec->main_param = ++*params;
}

static void vstr__fmt_init_spec(struct Vstr__fmt_spec *spec)
{
 spec->num_base = 10;
 spec->int_type = VSTR_TYPE_FMT_UINT;
 spec->flags = 0;

 spec->main_param = 0;
 spec->field_width_param = 0;
 spec->precision_param = 0;

 spec->field_width_usr = FALSE;
 
 spec->escape_usr = FALSE;
 spec->usr_spec = NULL;
}

/* must match with code in vstr_version.c */
#if defined(VSTR_AUTOCONF_FMT_DBL_glibc)
# include "vstr_dbl/vstr_add_fmt_dbl_glibc.c"
#elif defined(VSTR_AUTOCONF_FMT_DBL_none)
# include "vstr_dbl/vstr_add_fmt_dbl_none.c"
#elif defined(VSTR_AUTOCONF_FMT_DBL_host)
# include "vstr_dbl/vstr_add_fmt_dbl_host.c"
#else
# error "Please configure properly..."
#endif

static int vstr__add_fmt_char(Vstr_base *base, size_t pos_diff,
                              struct Vstr__fmt_spec *spec)
{
  if (spec->field_width_usr && !(spec->flags & LEFT))
    if (spec->field_width > 0)
    {
      if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width))
        goto failed_alloc;
      spec->field_width_usr = FALSE;
    }
  
  if (!VSTR__FMT_ADD(base, &spec->u.data_c, 1))
    goto failed_alloc;
  
  if (spec->field_width_usr && (spec->field_width > 0))
  {
    if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width))
      goto failed_alloc;
  }

  return (TRUE);

 failed_alloc:
  return (FALSE);
}

#if USE_WIDE_CHAR_T
static int vstr__add_fmt_wide_char(Vstr_base *base, size_t pos_diff,
                                   struct Vstr__fmt_spec *spec)
{
  mbstate_t state;
  size_t len_mbs = 0;
  char *buf_mbs = NULL;
  
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
  
  if (spec->field_width_usr && !(spec->flags & LEFT))
    if (spec->field_width > 0)
    {
      if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width))
        goto C_failed_alloc_free_buf_mbs;
      spec->field_width_usr = FALSE;
    }
  
  if (!VSTR__FMT_ADD(base, buf_mbs, len_mbs))
    goto C_failed_alloc_free_buf_mbs;
  
  if (spec->field_width_usr && (spec->field_width > 0))
  {
    if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width))
      goto C_failed_alloc_free_buf_mbs;
  }
  
  free(buf_mbs);
  return (TRUE);
  
 C_failed_alloc_free_buf_mbs:
  free(buf_mbs);
  return (FALSE);

 failed_alloc:
 failed_EILSEQ: /* FIXME: */
  return (FALSE);
}
#else
static int vstr__add_fmt_wide_char(Vstr_base *base __attribute__((unused)),
                                   size_t pos_diff __attribute__((unused)),
                                   struct Vstr__fmt_spec *spec
                                   __attribute__((unused)))
{
  assert(FALSE);
  return (FALSE);
}
#endif

static int vstr__add_fmt_cstr(Vstr_base *base, size_t pos_diff,
                              struct Vstr__fmt_spec *spec)
{
  size_t len = 0;
  const char *str = spec->u.data_ptr;
  
  if (spec->flags & PREC_USR)
    len = strnlen(str, spec->precision);
  else
    len = strlen(str);
  
  if ((spec->flags & PREC_USR) && spec->field_width_usr &&
      (spec->field_width > spec->precision))
    spec->field_width = spec->precision;
  
  if (spec->field_width_usr && !(spec->flags & LEFT))
    if (spec->field_width > len)
    {
      if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width - len))
        goto failed_alloc;
      spec->field_width_usr = FALSE;
    }
  
  if (!VSTR__FMT_ADD(base, str, len))
    goto failed_alloc;
  
  if (spec->field_width_usr && (spec->field_width > len))
    if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width - len))
      goto failed_alloc;

  return (TRUE);

 failed_alloc:
  return (FALSE);
}

#if USE_WIDE_CHAR_T
static int vstr__add_fmt_wide_cstr(Vstr_base *base, size_t pos_diff,
                                   struct Vstr__fmt_spec *spec)
{ /* note precision is number of wchar_t's allowed through, not bytes
   * as in sprintf() */
  mbstate_t state;
  size_t len_mbs = 0;
  char *buf_mbs = NULL;
  const wchar_t *tmp = spec->u.data_ptr;
  
  memset(&state, 0, sizeof(mbstate_t));

  if (!(spec->flags & PREC_USR))
    spec->precision = UINT_MAX;
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
  
  if ((spec->flags & PREC_USR) && spec->field_width_usr &&
      (spec->field_width > spec->precision))
    spec->field_width = spec->precision;
  
  if (spec->field_width_usr && !(spec->flags & LEFT))
    if (spec->field_width > len_mbs)
    {
      if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width - len_mbs))
        goto S_failed_alloc_free_buf_mbs;
      spec->field_width_usr = FALSE;
    }
  
  if (!VSTR__FMT_ADD(base, buf_mbs, len_mbs))
    goto S_failed_alloc_free_buf_mbs;
  
  if (spec->field_width_usr && (spec->field_width > len_mbs))
    if (!VSTR__FMT_ADD_REP_CHR(base, ' ', spec->field_width - len_mbs))
      goto S_failed_alloc_free_buf_mbs;
  
  free(buf_mbs);
  return (TRUE);
  
 S_failed_alloc_free_buf_mbs:
  free(buf_mbs);
 failed_alloc:
 failed_EILSEQ: /* FIXME: */
  return (FALSE);
}
#else
static int vstr__add_fmt_wide_cstr(Vstr_base *base __attribute__((unused)),
                                   size_t pos_diff __attribute__((unused)),
                                   struct Vstr__fmt_spec *spec
                                   __attribute__((unused)))
{
  assert(FALSE);
  return (FALSE);
}
#endif

#define VSTR__FMT_USR_SZ 8
static
struct Vstr__fmt_spec *
vstr__add_fmt_usr_write_spec(Vstr_base *base, size_t orig_len, size_t pos_diff,
                             struct Vstr__fmt_spec *spec)
{
  struct Vstr__fmt_spec *beg  = spec;
  struct Vstr__fmt_spec *last = NULL;
  struct 
  {
   Vstr_fmt_spec usr_spec;
   void *params[VSTR__FMT_USR_SZ];
  } dummy;
  Vstr_fmt_spec *usr_spec = NULL;
  unsigned int scan = 0;

  assert(spec->escape_usr);
  assert(spec->usr_spec);
  
  if (beg->usr_spec->sz <= VSTR__FMT_USR_SZ)
    usr_spec = &dummy.usr_spec;
  else
  {
    if (!(usr_spec = malloc(sizeof(Vstr_fmt_spec) + spec->usr_spec->sz)))
      return (NULL);
  }

  usr_spec->vstr_orig_len = orig_len;
  usr_spec->name = spec->usr_spec->name_str;

  usr_spec->obj_precision = 0;
  usr_spec->obj_field_width = 0;
  
  if ((usr_spec->fmt_precision = !!(spec->flags & PREC_USR)))
    usr_spec->obj_precision = spec->precision;
  if ((usr_spec->fmt_field_width =  spec->field_width_usr))
    usr_spec->obj_field_width = spec->field_width;

  usr_spec->fmt_minus = !!(spec->flags & LEFT);
  usr_spec->fmt_plus =  !!(spec->flags & PLUS);
  usr_spec->fmt_space = !!(spec->flags & SPACE);
  usr_spec->fmt_hash =  !!(spec->flags & SPECIAL);
  usr_spec->fmt_zero =  !!(spec->flags & ZEROPAD);
  usr_spec->fmt_quote = !!(spec->flags & THOUSAND_SEP);
  usr_spec->fmt_I =     !!(spec->flags & ALT_DIGITS);

  if (!beg->usr_spec->types[scan])
  {
    assert(spec->escape_usr && !!spec->usr_spec);

    last = spec;
    spec = spec->next;
  }
  
  while (beg->usr_spec->types[scan])
  {
    switch (beg->usr_spec->types[scan])
    {
      case VSTR_TYPE_FMT_INT:
      case VSTR_TYPE_FMT_UINT:
      case VSTR_TYPE_FMT_LONG:
      case VSTR_TYPE_FMT_ULONG:
      case VSTR_TYPE_FMT_LONG_LONG:
      case VSTR_TYPE_FMT_ULONG_LONG:
      case VSTR_TYPE_FMT_SSIZE_T:
      case VSTR_TYPE_FMT_SIZE_T:
      case VSTR_TYPE_FMT_PTRDIFF_T:
      case VSTR_TYPE_FMT_INTMAX_T:
      case VSTR_TYPE_FMT_UINTMAX_T:
      case VSTR_TYPE_FMT_DOUBLE:
      case VSTR_TYPE_FMT_DOUBLE_LONG:
        usr_spec->data_ptr[scan] = &spec->u;
        break;
      case VSTR_TYPE_FMT_PTR_VOID:
        usr_spec->data_ptr[scan] =  spec->u.data_ptr;
        break;
      default:
        assert(FALSE);
    }
    assert(spec->escape_usr && (scan ? !spec->usr_spec : !!spec->usr_spec));
    
    ++scan;
    last = spec;
    spec = spec->next;
  }
  assert(!spec || !spec->escape_usr || spec->usr_spec);
  usr_spec->data_ptr[scan] = NULL;

  if (!(*beg->usr_spec->func)(base, base->len - pos_diff, usr_spec))
  {
    if (beg->usr_spec->sz > VSTR__FMT_USR_SZ)
      free(usr_spec);
    
    return (NULL);
  }
  
  return (last);
}

#define VSTR__FMT_N_PTR(x, y, z) \
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
                                size_t orig_len)
{
  struct Vstr__fmt_spec *const beg  = vstr__fmt_spec_list_beg;
  struct Vstr__fmt_spec *const end  = vstr__fmt_spec_list_end;
  struct Vstr__fmt_spec *      spec = vstr__fmt_spec_list_beg;

  assert(beg && end);

  /* allow vstr_add_vfmt() to be called form a usr cb */
  vstr__fmt_spec_list_beg = NULL;
  vstr__fmt_spec_list_end = NULL;

  while (spec)
  {
    if (spec->escape_usr)
    {
      if (!(spec = vstr__add_fmt_usr_write_spec(base, orig_len,
                                                pos_diff, spec)))
        goto failed_alloc;
    }
    else switch (spec->fmt_code)
    {
      case 'c':
        if (!vstr__add_fmt_char(base, pos_diff, spec))
          goto failed_alloc;
        break;
        
      case 'C':
        if (!vstr__add_fmt_wide_char(base, pos_diff, spec))
          goto failed_alloc;
        break;
      
      case 's':
        if (!vstr__add_fmt_cstr(base, pos_diff, spec))
          goto failed_alloc;
        break;
      
      case 'S':
        if (!vstr__add_fmt_wide_cstr(base, pos_diff, spec))
          goto failed_alloc;
        break;
        
      case 'p': /* convert ptr to unsigned long and print */
      {
        unsigned long data_l = 0;
        
        assert(spec->int_type == VSTR_TYPE_FMT_ULONG);
        assert(!(spec->flags & SIGN));

        data_l = (unsigned long)spec->u.data_ptr;
        spec->u.data_l = data_l;
      }
      /* FALLTHROUGH */
      case 'd':
      case 'i':
      case 'u':
      case 'X':
      case 'x':
      case 'o':
        if (!vstr__add_fmt_number(base, pos_diff, spec))
          goto failed_alloc;
        break;
        
      case 'n':
      {
        size_t len = (base->len - orig_len);
        
        switch (spec->int_type)
        {
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_UCHAR, signed char, unsigned char);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_USHORT, signed short, unsigned short);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_UINT, signed int, unsigned int);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_ULONG, signed long, unsigned long);
#ifdef HAVE_LONG_LONG
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_ULONG_LONG,
                          signed long long, unsigned long long);
#endif
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_SIZE_T, ssize_t, size_t);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_UINTMAX_T, intmax_t, uintmax_t);
          
          case VSTR_TYPE_FMT_PTRDIFF_T:
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
      
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
      case 'a':
      case 'A':
        if (!vstr__add_fmt_dbl(base, pos_diff, spec))
          goto failed_alloc;
        break;
      
      default:
        assert(FALSE);
    }
    
    spec = spec->next;
  }

  end->next = vstr__fmt_spec_make;
  vstr__fmt_spec_make = beg;

  return (TRUE);
  
 failed_alloc:
  end->next = vstr__fmt_spec_make;
  vstr__fmt_spec_make = beg;
  
  return (FALSE);
}

#undef VSTR__FMT_N_PTR
#define VSTR__FMT_N_PTR(x, y, z) \
          case x: \
            if (spec->flags & SIGN) \
              u.data_ptr = va_arg(ap, y *); \
            else \
              u.data_ptr = va_arg(ap, z *); \
            break

static int vstr__fmt_fillin_spec(va_list ap, int have_dollars)
{
 struct Vstr__fmt_spec *beg = vstr__fmt_spec_list_beg;
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
    int tmp_fw_p = 0;
    
   if (count == spec->field_width_param)
   {
    if (!done) tmp_fw_p = va_arg(ap, int); done = TRUE;

    if (tmp_fw_p < 0) /* negative field width == flag '-' */
    {
      spec->flags |= LEFT;
      VSTR__FMT_S2U_NUM(spec->field_width, tmp_fw_p);
    }
    else
      spec->field_width = tmp_fw_p;
    
    spec->field_width_param = 0;
    if (!have_dollars)
      break;
   }
   
   if (count == spec->precision_param)
   {
     if (!done) tmp_fw_p = va_arg(ap, int); done = TRUE;
     
     if (tmp_fw_p < 0) /* negative precision == pretend one wasn't given */
       spec->flags &= ~PREC_USR;
     else
       spec->precision = tmp_fw_p;
     spec->precision_param = 0;
     if (!have_dollars)
       break;
   }
   
   if (count == spec->main_param)
   {
    if (!done)
      switch (spec->fmt_code)
      {
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
        case 'i':
        case 'u':
        case 'X':
        case 'x':
        case 'o':
         switch (spec->int_type)
         {
          case VSTR_TYPE_FMT_UCHAR:
            if (spec->flags & SIGN)
            {
             signed char tmp = (signed char) va_arg(ap, int);
             VSTR__FMT_ABS_NUM(u.data_c, tmp);
            }
            else
              u.data_c = (unsigned char) va_arg(ap, unsigned int);
            if (!u.data_c) spec->flags |= NUM_IS_ZERO; 
            break;
          case VSTR_TYPE_FMT_USHORT:
            if (spec->flags & SIGN)
            {
             signed short tmp = (signed short) va_arg(ap, int);
             VSTR__FMT_ABS_NUM(u.data_s, tmp);
            }
            else
              u.data_s = (unsigned short) va_arg(ap, unsigned int);
            if (!u.data_s) spec->flags |= NUM_IS_ZERO;
            break;
          case VSTR_TYPE_FMT_UINT:
            if (spec->flags & SIGN)
            {
             int tmp = va_arg(ap, int);
             VSTR__FMT_ABS_NUM(u.data_i, tmp);
            }
            else
              u.data_i = va_arg(ap, unsigned int);
            if (!u.data_i) spec->flags |= NUM_IS_ZERO;
            break;
          case VSTR_TYPE_FMT_ULONG:
            if (spec->flags & SIGN)
            {
             long tmp = va_arg(ap, long);
             VSTR__FMT_ABS_NUM(u.data_l, tmp);
            }
            else
              u.data_l = va_arg(ap, unsigned long);
            if (!u.data_l) spec->flags |= NUM_IS_ZERO;
            break;
#ifdef HAVE_LONG_LONG
          case VSTR_TYPE_FMT_ULONG_LONG:
            if (spec->flags & SIGN)
            {
             long long tmp = va_arg(ap, long long);
             VSTR__FMT_ABS_NUM(u.data_L, tmp);
            }
            else
              u.data_L = va_arg(ap, unsigned long long);
            if (!u.data_L) spec->flags |= NUM_IS_ZERO;
            break;
#endif
          case VSTR_TYPE_FMT_SIZE_T:
            if (spec->flags & SIGN)
            {
             ssize_t tmp = va_arg(ap, ssize_t);
             VSTR__FMT_ABS_NUM(u.data_sz, tmp);
            }
            else
              u.data_sz = va_arg(ap, size_t);
            if (!u.data_sz) spec->flags |= NUM_IS_ZERO;
            break;
          case VSTR_TYPE_FMT_PTRDIFF_T:
            if (1)
            { /* no unsigned version ... */
             intmax_t tmp = va_arg(ap, ptrdiff_t);
             VSTR__FMT_ABS_NUM(u.data_m, tmp);
            }
            if (!u.data_m) spec->flags |= NUM_IS_ZERO;
            spec->int_type = VSTR_TYPE_FMT_UINTMAX_T;
            break;
          case VSTR_TYPE_FMT_UINTMAX_T:
            if (spec->flags & SIGN)
            {
             intmax_t tmp = va_arg(ap, intmax_t);
             VSTR__FMT_ABS_NUM(u.data_m, tmp);
            }
            else
              u.data_m = va_arg(ap, uintmax_t);
            if (!u.data_m) spec->flags |= NUM_IS_ZERO;
            break;
            
          default:
            assert(FALSE);
         }
         break;
         
       case 'p':
       {
        void *tmp = va_arg(ap, void *);
        u.data_ptr = tmp;
       }
       break;
         
       case 'n':
         switch (spec->int_type)
         {
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_UCHAR, signed char, unsigned char);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_USHORT, signed short, unsigned short);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_UINT, signed int, unsigned int);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_ULONG, signed long, unsigned long);
#ifdef HAVE_LONG_LONG
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_ULONG_LONG,
                          signed long long, unsigned long long);
#endif
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_SIZE_T, ssize_t, size_t);
          VSTR__FMT_N_PTR(VSTR_TYPE_FMT_UINTMAX_T, intmax_t, uintmax_t);
            
          case VSTR_TYPE_FMT_PTRDIFF_T:
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
         if (spec->int_type == VSTR_TYPE_FMT_ULONG_LONG)
           u.data_Ld = va_arg(ap, long double);
         else
           u.data_d = va_arg(ap, double);
         break;

        default:
          assert(FALSE);
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

static const char *vstr__add_fmt_usr_esc(Vstr_conf *conf,
                                         const char *fmt,
                                         struct Vstr__fmt_spec *spec,
                                         unsigned int *passed_params)
{
  struct Vstr__fmt_spec *beg_spec = spec;
  unsigned int params = *passed_params;
  Vstr__fmt_usr_name_node *scan_node = conf->fmt_usr_names;
  
  while (scan_node)
  {
    if (!strncmp(fmt, scan_node->name_str, scan_node->name_len))
      break;
    
    scan_node = scan_node->next;
  }
  
  if (scan_node)
  {
    unsigned int scan = 0;

    spec->usr_spec = scan_node;

    while (TRUE)
    {
      int have_arg = TRUE;
      
      spec->escape_usr = TRUE;
    
      switch (scan_node->types[scan])
      {
        case VSTR_TYPE_FMT_END:
          have_arg = FALSE;
          break;
        case VSTR_TYPE_FMT_INT:
          spec->flags |= SIGN;
          spec->fmt_code = 'd';
          break;
        case VSTR_TYPE_FMT_UINT:
          spec->fmt_code = 'u';
          break;
        case VSTR_TYPE_FMT_LONG:
          spec->int_type = VSTR_TYPE_FMT_ULONG;
          spec->flags |= SIGN;
          spec->fmt_code = 'd';
          break;
        case VSTR_TYPE_FMT_ULONG:
          spec->int_type = VSTR_TYPE_FMT_ULONG;
          spec->fmt_code = 'u';
          break;
        case VSTR_TYPE_FMT_LONG_LONG:
          spec->int_type = VSTR_TYPE_FMT_ULONG_LONG;
          spec->flags |= SIGN;
          spec->fmt_code = 'd';
          break;
        case VSTR_TYPE_FMT_ULONG_LONG:
          spec->int_type = VSTR_TYPE_FMT_ULONG_LONG;
          spec->fmt_code = 'u';
          break;
        case VSTR_TYPE_FMT_SSIZE_T:
          spec->int_type = VSTR_TYPE_FMT_SIZE_T;
          spec->flags |= SIGN;
          spec->fmt_code = 'd';
          break;
        case VSTR_TYPE_FMT_SIZE_T:
          spec->int_type = VSTR_TYPE_FMT_SIZE_T;
          spec->fmt_code = 'u';
          break;
        case VSTR_TYPE_FMT_PTRDIFF_T:
          spec->int_type = VSTR_TYPE_FMT_PTRDIFF_T;
          spec->flags |= SIGN;
          spec->fmt_code = 'd';
          break;
        case VSTR_TYPE_FMT_INTMAX_T:
          spec->int_type = VSTR_TYPE_FMT_UINTMAX_T;
          spec->flags |= SIGN;
          spec->fmt_code = 'd';
          break;
        case VSTR_TYPE_FMT_UINTMAX_T:
          spec->int_type = VSTR_TYPE_FMT_UINTMAX_T;
          spec->fmt_code = 'u';
          break;
        case VSTR_TYPE_FMT_DOUBLE:
          spec->fmt_code = 'f';
          break;
        case VSTR_TYPE_FMT_DOUBLE_LONG:
          spec->int_type = VSTR_TYPE_FMT_ULONG_LONG;
          spec->fmt_code = 'f';
          break;
        case VSTR_TYPE_FMT_PTR_VOID:
          spec->fmt_code = 'p';
          break;
        default:
          assert(FALSE);
      }

      if (scan && (params == *passed_params))
        spec->main_param = beg_spec->main_param + scan;
      VSTR__FMT_MV_SPEC(have_arg);

      if (!have_arg || !scan_node->types[++scan])
        break;

      if (!vstr__fmt_spec_make && !vstr__fmt_add_spec())
        goto failed_alloc;
      spec = vstr__fmt_spec_make;
      vstr__fmt_init_spec(spec);
    }

    *passed_params = params;

    return (fmt + scan_node->name_len);
  }

  return (fmt);
  
 failed_alloc:
  return (NULL);
}

static const char *vstr__add_fmt_spec(const char *fmt,
                                      struct Vstr__fmt_spec *spec,
                                      unsigned int *params,
                                      unsigned int *have_dollars)
{
  int tmp_num = 0;

  /* process flags */
  while (TRUE)
  {
    switch (*fmt)
    {
      case '-':  spec->flags |= LEFT;         break;
      case '+':  spec->flags |= PLUS;         break;
      case ' ':  spec->flags |= SPACE;        break;
      case '#':  spec->flags |= SPECIAL;      break;
      case '0':  spec->flags |= ZEROPAD;      break;
      case '\'': spec->flags |= THOUSAND_SEP; break;
      case 'I':  spec->flags |= ALT_DIGITS;   break;
        
      default:
        goto got_flags;
    }
    ++fmt;
  }
 got_flags:
  
  /* get i18n param number */   
  if (VSTR__ADD_FMT_ISDIGIT(*fmt))
  {
    tmp_num = VSTR__ADD_FMT_STRTOL(fmt);
    
    if (*fmt != '$')
      goto use_field_width;
    
    ++fmt;
    *have_dollars = TRUE;
    spec->main_param = tmp_num;
  }
  
  /* get field width */   
  if (VSTR__ADD_FMT_ISDIGIT(*fmt))
  {
    tmp_num = VSTR__ADD_FMT_STRTOL(fmt);
    
   use_field_width:
    spec->field_width_usr = TRUE;
    
    if (tmp_num < 0)
    {
      ++tmp_num;
      spec->field_width = -tmp_num;
      ++spec->field_width;
      spec->flags |= LEFT;
    }
    else
      spec->field_width = tmp_num;
  }
  else if (*fmt == '*')
  {
    const char *dollar_start = fmt;
    
    spec->field_width_usr = TRUE;
    
    ++fmt;
    tmp_num = 0;
    if (VSTR__ADD_FMT_ISDIGIT(*fmt))
      tmp_num = VSTR__ADD_FMT_STRTOL(fmt);
    
    if (*fmt != '$')
    {
      fmt = dollar_start + 1;
      spec->field_width_param = ++*params;
    }
    else
    {
      ++fmt;
      spec->field_width_param = tmp_num;
    }
  }
  
  /* get the precision */
  if (*fmt == '.')
  {
    spec->flags |= PREC_USR;
    
    ++fmt;
    if (VSTR__ADD_FMT_ISDIGIT(*fmt))
    {
      tmp_num = VSTR__ADD_FMT_STRTOL(fmt);
      if (tmp_num < 0) /* check to make sure things go nicely */
        tmp_num = 0;

      spec->precision = tmp_num;
    }
    else if (*fmt == '*')
    {
      const char *dollar_start = fmt;
      ++fmt;

      tmp_num = 0;
      if (VSTR__ADD_FMT_ISDIGIT(*fmt))
        tmp_num = VSTR__ADD_FMT_STRTOL(fmt);
        
      if (*fmt != '$')
      {
        fmt = dollar_start + 1;
        spec->precision_param = ++*params;
      }
      else
      {
        ++fmt;
        spec->precision_param = tmp_num;
      }
    }
  }
  
  /* get width of type */
  switch (*fmt)
  {
   case 'h':
     ++fmt;
     if (*fmt == 'h')
     {
      ++fmt;
      spec->int_type = VSTR_TYPE_FMT_UCHAR;
     }
     else
       spec->int_type = VSTR_TYPE_FMT_USHORT;
     break;
   case 'l':
     ++fmt;
     if (*fmt == 'l')
     {
      ++fmt;
      spec->int_type = VSTR_TYPE_FMT_ULONG_LONG;
     }
     else
       spec->int_type = VSTR_TYPE_FMT_ULONG;
     break;
   case 'L': ++fmt; spec->int_type = VSTR_TYPE_FMT_ULONG_LONG; break;
   case 'z': ++fmt; spec->int_type = VSTR_TYPE_FMT_SIZE_T;     break;
   case 't': ++fmt; spec->int_type = VSTR_TYPE_FMT_PTRDIFF_T;  break;
   case 'j': ++fmt; spec->int_type = VSTR_TYPE_FMT_UINTMAX_T;  break;

   default:
     break;
  }

  return (fmt);
}

static size_t vstr__add_vfmt(Vstr_base *base, size_t pos, unsigned int userfmt,
                             const char *fmt, va_list ap)
{
 size_t start_pos = pos + 1;
 size_t orig_len = base->len;
 size_t pos_diff = 0;
 unsigned int params = 0;
 unsigned int have_dollars = FALSE; /* have posix %2$d etc. stuff */
 unsigned char fmt_usr_escape = 0;
 
 if (!base || !fmt || !*fmt || (pos > base->len))
   return (0);

 /* this will be correct as you add chars */
 pos_diff = base->len - pos;

 if (userfmt)
   fmt_usr_escape = base->conf->fmt_usr_escape;
 
 while (*fmt)
 {
  const char *fmt_orig = fmt;
  struct Vstr__fmt_spec *spec = NULL;
  
  if (!vstr__fmt_spec_make && !vstr__fmt_add_spec())
    goto failed_alloc;

  spec = vstr__fmt_spec_make;
  vstr__fmt_init_spec(spec);
  
  if ((*fmt != '%') && (*fmt != fmt_usr_escape))
  {
    char *next_escape = strchr(fmt, '%');
    
    spec->fmt_code = 's';
    spec->u.data_ptr = (char *)fmt;
    
    if (fmt_usr_escape)
    { /* find first of the two escapes */
      char *next_usr_escape = strchr(fmt, fmt_usr_escape);
      if (next_usr_escape && (!next_escape || (next_usr_escape < next_escape)))
        next_escape = next_usr_escape;
    }
    
    if (next_escape)
    {
      size_t len = next_escape - fmt;
      spec->precision = len;
      spec->flags |= PREC_USR;
      fmt = fmt + len;
    }
    else
      fmt = "";
    
    VSTR__FMT_MV_SPEC(FALSE);
    
    continue;
  }

  if (fmt[0] == fmt[1])
  {
   spec->u.data_c = fmt[0];
   spec->fmt_code = 'c';
   VSTR__FMT_MV_SPEC(FALSE);
   fmt += 2; /* skip escs */
   continue;
  }
  assert(fmt_orig == fmt);
  ++fmt; /* skip esc */

  fmt = vstr__add_fmt_spec(fmt, spec, &params, &have_dollars);
  
  if (fmt_usr_escape && (*fmt_orig == fmt_usr_escape))
  {
    const char *fmt_end = vstr__add_fmt_usr_esc(base->conf, fmt, spec, &params);
    if (!fmt_end)
      goto failed_alloc;
    else if (fmt_end == fmt)
    {
      if (fmt_usr_escape == '%')
        goto vstr__fmt_sys_escapes;
      
      assert(FALSE);
      
      vstr__fmt_init_spec(spec);
      spec->u.data_c = *fmt_orig;
      spec->fmt_code = 'c';
      VSTR__FMT_MV_SPEC(FALSE);
      fmt = fmt_orig + 1;
    }
    else
      fmt = fmt_end;
    continue;
  }

  spec->fmt_code = *fmt;
  vstr__fmt_sys_escapes:
  switch (*fmt)
  {
    case 'c':
      if (spec->int_type != VSTR_TYPE_FMT_ULONG) /* %lc == %C */
        break;
      /* FALL THROUGH */
      
    case 'C':
      spec->fmt_code = 'C';
      break;
      
    case 's':
      if (spec->int_type != VSTR_TYPE_FMT_ULONG) /* %ls == %S */
        break;
      /* FALL THROUGH */
      
    case 'S':
      spec->fmt_code = 'S';
      break;
      
    case 'd':
    case 'i':
      spec->flags |= SIGN;
    case 'u':
      break;
      
    case 'X':
      spec->flags |= LARGE;
    case 'x':
      spec->num_base = 16;
      break;
      
    case 'o':
      spec->num_base = 8;
      break;    
      
    case 'p':
      spec->num_base = 16;
      spec->int_type = VSTR_TYPE_FMT_ULONG;
      spec->flags |= SPECIAL;
      break;
      
    case 'n':
      break;
      
    case 'A': /* print like [-]x.xxxPxx -- in hex using ABCDEF */
    case 'E': /* use big E instead */
    case 'F': /* print like an int - upper case infinity/nan */
    case 'G': /* use the smallest of E and F */
      spec->flags |= LARGE;
    case 'a': /* print like [-]x.xxxpxx -- in hex using abcdef */
    case 'e': /* print like [-]x.xxxexx */
    case 'f': /* print like an int */
    case 'g': /* use the smallest of e and f */
      break;
      
    default:
      assert(FALSE);
      
      vstr__fmt_init_spec(spec);
      spec->u.data_c = *fmt_orig;
      spec->fmt_code = 'c';
      VSTR__FMT_MV_SPEC(FALSE);
      fmt = fmt_orig + 1;
      continue;
  }

  VSTR__FMT_MV_SPEC(TRUE);
  ++fmt;
 }

 if (!vstr__fmt_fillin_spec(ap, have_dollars))
   goto failed_alloc;
 
 if (!vstr__fmt_write_spec(base, pos_diff, orig_len))
   goto failed_write_spec;
 
 return (base->len - orig_len);

 failed_write_spec:

 if (base->len - orig_len)
   vstr_nx_del(base, start_pos, base->len - orig_len);

 if (0) /* already done in write_spec */
 {
  failed_alloc:
   vstr__fmt_spec_list_end->next = vstr__fmt_spec_make;
   vstr__fmt_spec_make = vstr__fmt_spec_list_beg;
   vstr__fmt_spec_list_beg = NULL;
   vstr__fmt_spec_list_end = NULL; 
 }
 
 return (0);
}

size_t vstr_nx_add_vfmt(Vstr_base *base, size_t pos,
                        const char *fmt, va_list ap)
{
  return (vstr__add_vfmt(base, pos, TRUE,  fmt, ap));
}

size_t vstr_nx_add_vsysfmt(Vstr_base *base, size_t pos,
                           const char *fmt, va_list ap)
{
  return (vstr__add_vfmt(base, pos, FALSE, fmt, ap));
}

size_t vstr_nx_add_fmt(Vstr_base *base, size_t pos, const char *fmt, ...)
{
  size_t len = 0;
  va_list ap;
  
  va_start(ap, fmt);
  
  len =   vstr__add_vfmt(base, pos, TRUE,  fmt, ap);
  
  va_end(ap);
  
  return (len);
}

size_t vstr_nx_add_sysfmt(Vstr_base *base, size_t pos, const char *fmt, ...)
{
  size_t len = 0;
  va_list ap;
  
  va_start(ap, fmt);
  
  len =   vstr__add_vfmt(base, pos, FALSE, fmt, ap);
  
  va_end(ap);
  
  return (len);
}

static
Vstr__fmt_usr_name_node *
vstr__fmt_usr_srch(Vstr_conf *conf, const char *name)
{
  Vstr__fmt_usr_name_node *scan = conf->fmt_usr_names;
  size_t len = strlen(name);
  
  while (scan)
  {
    if ((scan->name_len == len) && !memcmp(scan->name_str, name, len))
      return (scan);
    
    scan = scan->next;
  }
  
  return (NULL);
}

int vstr_nx_fmt_add(Vstr_conf *conf, const char *name,
                    int (*func)(Vstr_base *, size_t, Vstr_fmt_spec *), ...)
{
  Vstr__fmt_usr_name_node *beg = conf->fmt_usr_names;
  va_list ap;
  unsigned int count = 1;
  Vstr__fmt_usr_name_node *node = malloc(sizeof(Vstr__fmt_usr_name_node) +
                                         (sizeof(unsigned int) * count));
  unsigned int scan_type = 0;

  if (vstr__fmt_usr_srch(conf, name))
    return (FALSE);
  
  if (!node)
  {
    conf->malloc_bad = TRUE;
    return (FALSE);
  }
  
  node->next = beg;
  node->prev = NULL;
  node->name_str = name;
  node->name_len = strlen(name);
  node->func = func;
  
  va_start(ap, func);
  while ((scan_type = va_arg(ap, unsigned int)))
  {
    Vstr__fmt_usr_name_node *tmp = realloc(node,
                                           sizeof(Vstr__fmt_usr_name_node) +
                                           (sizeof(unsigned int) * ++count));
    if (!tmp)
    {
      conf->malloc_bad = TRUE;
      free(node);
      va_end(ap);
      return (FALSE);
    }
    node = tmp;

    assert(FALSE ||
           (scan_type == VSTR_TYPE_FMT_INT) ||
           (scan_type == VSTR_TYPE_FMT_UINT) ||
           (scan_type == VSTR_TYPE_FMT_LONG) ||
           (scan_type == VSTR_TYPE_FMT_ULONG) ||
           (scan_type == VSTR_TYPE_FMT_LONG_LONG) ||
           (scan_type == VSTR_TYPE_FMT_ULONG_LONG) ||
           (scan_type == VSTR_TYPE_FMT_SSIZE_T) ||
           (scan_type == VSTR_TYPE_FMT_SIZE_T) ||
           (scan_type == VSTR_TYPE_FMT_PTRDIFF_T) ||
           (scan_type == VSTR_TYPE_FMT_INTMAX_T) ||
           (scan_type == VSTR_TYPE_FMT_UINTMAX_T) ||
           (scan_type == VSTR_TYPE_FMT_DOUBLE) ||
           (scan_type == VSTR_TYPE_FMT_DOUBLE_LONG) ||
           (scan_type == VSTR_TYPE_FMT_PTR_VOID) ||
           FALSE);
    
    node->types[count - 2] = scan_type;
  }
  assert(count >= 1);
  node->types[count - 1] = scan_type;
  node->sz = count;

  va_end(ap);

  if (node->next)
    node->next->prev = node;
  conf->fmt_usr_names = node;
  
  return (TRUE);
}

void vstr_nx_fmt_del(Vstr_conf *conf, const char *name)
{
  Vstr__fmt_usr_name_node *scan = vstr__fmt_usr_srch(conf, name);

  if (scan)
  {
    if (scan->prev)
      scan->prev->next = scan->next;
    else
      conf->fmt_usr_names = scan->next;

    if (scan->next)
      scan->next->prev = scan->prev;

    free(scan);
  }
}

int vstr_nx_fmt_srch(Vstr_conf *conf, const char *name)
{
  return (!!vstr__fmt_usr_srch(conf, name));
}
