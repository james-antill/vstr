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
/* Floating point output for `printf'.
   Copyright (C) 1995-1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
/* for isnan/isnanl */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
/* This code does the %F, %f, %G, %g, %A, %a, %E, %e from *printf by using
 * the code from glibc */
/* NOTE: This file assumes the local char set is ASCII */
/* NOTE: This file assumes stuff about the double implementation */
/* NOTE: This file assumes long long exists */
/* Note that this file is #include'd */

#include <ieee754.h>

/* Pretend the world's a (FILE *) brain damage for glibc */
struct vstr__FILE_wrap
{
 Vstr_base *base;
 size_t pos_diff;
 struct Vstr__fmt_spec *spec;
};

struct vstr__fmt_printf_info
{
  int prec;                     /* Precision.  */
  int width;                    /* Width.  */
  wchar_t spec;                 /* Format letter.  */
  unsigned int is_long_double:1;/* L flag.  */
 /* unsigned int is_short:1; */      /* h flag.  */
 /* unsigned int is_long:1; */       /* l flag.  */
  unsigned int alt:1;           /* # flag.  */
  unsigned int space:1;         /* Space flag.  */
  unsigned int left:1;          /* - flag.  */
  unsigned int showsign:1;      /* + flag.  */
  unsigned int group:1;         /* ' flag.  */
 /* unsigned int extra:1; */         /* For special use.  */
 /* unsigned int is_char:1; */       /* hh flag.  */
 /* unsigned int wide:1; */          /* Nonzero for wide character streams.  */
 /* unsigned int i18n:1; */          /* I flag.  */
  wchar_t pad;                  /* Padding character.  */
};

#undef FILE
#define FILE struct vstr__FILE_wrap
#undef wide
#define wide 0 /* no wide output device */

#define VSTR__FMT_GLIBC_ADD(f, y, z) \
  vstr_nx_add_buf((f)->base, ((f)->base->len - (f)->pos_diff), y, z)

#undef putc
#define putc(c, f) vstr__fmt_dbl_putc(c, f)
static inline int vstr__fmt_dbl_putc(unsigned char c, FILE *f)
{
  char buf[1];

  buf[0] = c;
  
  if (!VSTR__FMT_GLIBC_ADD(f, buf, 1))
    return (EOF);
  
  return (c);
}

#define PUT(f, s, n) VSTR__FMT_GLIBC_ADD(f, s, n)

#define PAD(f, c, n) (((f->spec->flags & ZEROPAD) ? \
 vstr__add_zeros(f->base, f->pos_diff, n) : \
 vstr__add_spaces(f->base, f->pos_diff, n)) * n)

#undef isupper
#define isupper(x) (fp->spec->flags & LARGE)
#undef tolower
#define tolower(x) (VSTR__IS_ASCII_UPPER(x) ? VSTR__TO_ASCII_LOWER(x) : (x))


#ifndef HAVE_INLINE
# undef inline
# define inline /* nothing */
#endif

#include <endian.h>

/* can have different byte order for floats */
#ifndef __FLOAT_WORD_ORDER
# define __FLOAT_WORD_ORDER BYTE_ORDER
#endif

#if __FLOAT_WORD_ORDER == BIG_ENDIAN

typedef union
{
  double value;
  struct
  {
    u_int32_t msw;
    u_int32_t lsw;
  } parts;
} ieee_double_shape_type;

#endif

#if __FLOAT_WORD_ORDER == LITTLE_ENDIAN

typedef union
{
  double value;
  struct
  {
    u_int32_t lsw;
    u_int32_t msw;
  } parts;
} ieee_double_shape_type;

#endif

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)                                \
do {                                                            \
  ieee_double_shape_type ew_u;                                  \
  ew_u.value = (d);                                             \
  (ix0) = ew_u.parts.msw;                                       \
  (ix1) = ew_u.parts.lsw;                                       \
} while (0)

/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)                                      \
do {                                                            \
  ieee_double_shape_type gh_u;                                  \
  gh_u.value = (d);                                             \
  (i) = gh_u.parts.msw;                                         \
} while (0)

/* Get the less significant 32 bit int from a double.  */

#define GET_LOW_WORD(i,d)                                       \
do {                                                            \
  ieee_double_shape_type gl_u;                                  \
  gl_u.value = (d);                                             \
  (i) = gl_u.parts.lsw;                                         \
} while (0)

/* FIXME: major hack for now... autoconf it */
#if !defined(VSTR__LDOUBLE_BITS_128) && !defined(VSTR__LDOUBLE_BITS_96) && \
    defined(__i386__) && defined(__linux__)
# define VSTR__LDOUBLE_BITS_96 1
#else
# error "No long double size defined"
#endif

/* 128 bit long double */
#ifdef VSTR__LDOUBLE_BITS_128
#if __FLOAT_WORD_ORDER == BIG_ENDIAN

typedef union
{
  long double value;
  struct
  {
    u_int64_t msw;
    u_int64_t lsw;
  } parts64;
  struct
  {
    u_int32_t w0, w1, w2, w3;
  } parts32;
} ieee854_long_double_shape_type;

#endif

#if __FLOAT_WORD_ORDER == LITTLE_ENDIAN

typedef union
{
  long double value;
  struct
  {
    u_int64_t lsw;
    u_int64_t msw;
  } parts64;
  struct
  {
    u_int32_t w3, w2, w1, w0;
  } parts32;
} ieee854_long_double_shape_type;

#endif

#define GET_LDOUBLE_WORDS64(ix0,ix1,d)                          \
do {                                                            \
  ieee854_long_double_shape_type qw_u;                          \
  qw_u.value = (d);                                             \
  (ix0) = qw_u.parts64.msw;                                     \
  (ix1) = qw_u.parts64.lsw;                                     \
} while (0)

static inline int vstr__fmt_dbl_isinfl(long double x)
{
  int64_t hx,lx;
  GET_LDOUBLE_WORDS64(hx,lx,x);
  lx |= (hx & 0x7fffffffffffffffLL) ^ 0x7fff000000000000LL;
  lx |= -lx;
  return ~(lx >> 63) & (hx >> 62);
}

static inline int vstr__fmt_dbl_isnanl(long double x)
{
  int64_t hx,lx;
  GET_LDOUBLE_WORDS64(hx,lx,x);
  hx &= 0x7fffffffffffffffLL;
  hx |= (u_int64_t)(lx|(-lx))>>63;
  hx = 0x7fff000000000000LL - hx;
  return (int)((u_int64_t)hx>>63);
}

#endif

/* 96 bit long double */
#ifdef VSTR__LDOUBLE_BITS_96

#if __FLOAT_WORD_ORDER == BIG_ENDIAN

typedef union
{
  long double value;
  struct
  {
    int sign_exponent:16;
    unsigned int empty:16;
    u_int32_t msw;
    u_int32_t lsw;
  } parts;
} ieee_long_double_shape_type;

#endif

#if __FLOAT_WORD_ORDER == LITTLE_ENDIAN

typedef union
{
  long double value;
  struct
  {
    u_int32_t lsw;
    u_int32_t msw;
    int sign_exponent:16;
    unsigned int empty:16;
  } parts;
} ieee_long_double_shape_type;

#endif

#define GET_LDOUBLE_WORDS(exp,ix0,ix1,d)                        \
do {                                                            \
  ieee_long_double_shape_type ew_u;                             \
  ew_u.value = (d);                                             \
  (exp) = ew_u.parts.sign_exponent;                             \
  (ix0) = ew_u.parts.msw;                                       \
  (ix1) = ew_u.parts.lsw;                                       \
} while (0)

static inline int vstr__fmt_dbl_isinfl(long double x)
{
  int32_t se,hx,lx;
  GET_LDOUBLE_WORDS(se,hx,lx,x);
  /* This additional ^ 0x80000000 is necessary because in Intel's
     internal representation of the implicit one is explicit.  */
  lx |= (hx ^ 0x80000000) | ((se & 0x7fff) ^ 0x7fff);
  lx |= -lx;
  se &= 0x8000;
  return ~(lx >> 31) & (1 - (se >> 14));
}

static inline int vstr__fmt_dbl_isnanl(long double x)
{
  int32_t se,hx,lx;
  GET_LDOUBLE_WORDS(se,hx,lx,x);
  se = (se & 0x7fff) << 1;
  /* The additional & 0x7fffffff is required because Intel's
     extended format has the normally implicit 1 explicit
     present.  Sigh!  */
  lx |= hx & 0x7fffffff;
  se |= (u_int32_t)(lx|(-lx))>>31;
  se = 0xfffe - se;
  return (int)((u_int32_t)(se))>>16;
}

#endif


static inline int vstr__fmt_dbl_isinf(double x)
{
  int32_t hx,lx;
  EXTRACT_WORDS(hx,lx,x);
  lx |= (hx & 0x7fffffff) ^ 0x7ff00000;
  lx |= -lx;
  return ~(lx >> 31) & (hx >> 30);
}

#undef __isinf
#define __isinf(x) vstr__fmt_dbl_isinf(x)
#undef __isinfl
#define __isinfl(x) vstr__fmt_dbl_isinfl(x)

static inline int vstr__fmt_dbl_isnan(double x)
{
  int32_t hx,lx;
  EXTRACT_WORDS(hx,lx,x);
  hx &= 0x7fffffff;
  hx |= (u_int32_t)(lx|(-lx))>>31;
  hx = 0x7ff00000 - hx;
  return (int)(((u_int32_t)hx)>>31);
}

#undef __isnan
#define __isnan(x) vstr__fmt_dbl_isnan(x)
#undef __isnanl
#define __isnanl(x) vstr__fmt_dbl_isnanl(x)

/* from bits/mathinline.h from glibc - converted to endian agnostic */
static inline int vstr__fmt_dbl_signbit(double x)
{
  int32_t hx;
  
  GET_HIGH_WORD (hx, x);
  return hx & 0x80000000;
}

#undef signbit
#define signbit(x) vstr__fmt_dbl_signbit(x)

/* write the number backwards ... */
static inline char *vstr__fmt_dbl_itoa_word(unsigned long value, char *buflim,
                                            unsigned int base, int upper_case)
{
  const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";

  if (upper_case)
    digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  switch (base)
  {
    case 16:
      do
        *--buflim = digits[value % 16];
      while (value /= 16);
      break;
    case 10:
      do
        *--buflim = digits[value % 10];
      while (value /= 10);
      break;
      
    default:
      assert(FALSE);
  }
  
  return (buflim);
}

static inline wchar_t *vstr__fmt_dbl_itowa_word(unsigned long value,
                                                wchar_t *buflim,
                                                unsigned int base,
                                                int upper_case)
{
  switch (base)
  {
    case 16:
      do
        --buflim;
      while (value /= 16);
      break;
    case 10:
      do
        --buflim;
      while (value /= 10);
      break;
      
    default:
      assert(FALSE);
  }
  
  return (buflim);
}

#undef _itoa_word
#define _itoa_word(a, b, c, d) vstr__fmt_dbl_itoa_word(a, b, c, d)
#undef _itowa_word
#define _itowa_word(a, b, c, d) vstr__fmt_dbl_itowa_word(a, b, c, d)

static inline char *vstr__fmt_dbl_itoa(unsigned long long value, char *buflim,
                                       unsigned int base, int upper_case)
{
  const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";

  if (upper_case)
    digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  switch (base)
  {
    case 16:
      do
        *--buflim = digits[value % 16];
      while (value /= 16);
      break;
    case 10:
      do
        *--buflim = digits[value % 10];
      while (value /= 10);
      break;
      
    default:
      assert(FALSE);
  }
  
  return (buflim);
}

static inline wchar_t *vstr__fmt_dbl_itowa(unsigned long long value,
                                           wchar_t *buflim,
                                           unsigned int base, int upper_case)
{
  switch (base)
  {
    case 16:
      do
        --buflim;
      while (value /= 16);
      break;
    case 10:
      do
        --buflim;
      while (value /= 10);
      break;
      
    default:
      assert(FALSE);
  }
  
  return (buflim);
}

#undef _itoa
#define _itoa(a, b, c, d) vstr__fmt_dbl_itoa(a, b, c, d)
#undef _itowa
#define _itowa(a, b, c, d) vstr__fmt_dbl_itowa(a, b, c, d)

#undef inline                    

/* BEG: ==== printf_fphex ==== */

/* Macros for doing the actual output.  */

#define outchar(ch)							      \
  do									      \
    {									      \
      register const int outc = (ch);					      \
      if (putc (outc, fp) == EOF)					      \
	return -1;							      \
      ++done;								      \
    } while (0)

#define PRINT(ptr, wptr, len)						      \
  do									      \
    {									      \
      register size_t outlen = (len);					      \
      if (wide)								      \
	while (outlen-- > 0)						      \
	  outchar (*wptr++);						      \
      else								      \
	while (outlen-- > 0)						      \
	  outchar (*ptr++);						      \
    } while (0)

#define PADN(ch, len)							      \
  do									      \
    {									      \
      if (PAD (fp, ch, len) != len)					      \
	return -1;							      \
      done += len;							      \
    }									      \
  while (0)

#ifndef MIN
# define MIN(a,b) ((a)<(b)?(a):(b))
#endif


static int
vstr__fmt_printf_fphex (FILE *fp,
                        const struct vstr__fmt_printf_info *info,
                        const void *const *args)
{
  /* The floating-point value to output.  */
  union
    {
      union ieee754_double dbl;
      union ieee854_long_double ldbl;
    }
  fpnum;

  /* Locale-dependent representation of decimal point.	*/
  const char *decimal;
  wchar_t decimalwc = L'.';

  /* "NaN" or "Inf" for the special cases.  */
  const char *special = NULL;
  const wchar_t *wspecial = NULL;

  /* Buffer for the generated number string for the mantissa.  The
     maximal size for the mantissa is 128 bits.  */
  char numbuf[32];
  char *numstr;
  char *numend;
  wchar_t wnumbuf[32];
  wchar_t *wnumstr;
  wchar_t *wnumend;
  int negative;

  /* The maximal exponent of two in decimal notation has 5 digits.  */
  char expbuf[5];
  char *expstr;
  wchar_t wexpbuf[5];
  wchar_t *wexpstr;
  int expnegative;
  int exponent;

  /* Non-zero is mantissa is zero.  */
  int zero_mantissa;

  /* The leading digit before the decimal point.  */
  char leading;

  /* Precision.  */
  int precision = info->prec;

  /* Width.  */
  int width = info->width;

  /* Number of characters written.  */
  int done = 0;

  /* Nonzero if this is output on a wide character stream.  */
  /* int wide = info->wide; */

  size_t strlen_decimal = 0;

  /* Figure out the decimal point character.  */
  decimal = fp->base->conf->loc->decimal_point_str;
  strlen_decimal = fp->base->conf->loc->decimal_point_len;
  
  /* The decimal point character must never be zero.  */
  assert (*decimal != '\0' && decimalwc != L'\0');


  /* Fetch the argument value.	*/
#ifndef __NO_LONG_DOUBLE_MATH
  if (info->is_long_double && sizeof (long double) > sizeof (double))
    {
      fpnum.ldbl.d = *(const long double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (__isnanl (fpnum.ldbl.d))
	{
	  if (isupper (info->spec))
	    {
	      special = "NAN";
	      wspecial = L"NAN";
	    }
	  else
	    {
	      special = "nan";
	      wspecial = L"nan";
	    }
	  negative = 0;
	}
      else
	{
	  if (__isinfl (fpnum.ldbl.d))
	    {
	      if (isupper (info->spec))
		{
		  special = "INF";
		  wspecial = L"INF";
		}
	      else
		{
		  special = "inf";
		  wspecial = L"inf";
		}
	    }

	  negative = signbit (fpnum.ldbl.d);
	}
    }
  else
#endif	/* no long double */
    {
      fpnum.dbl.d = *(const double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (__isnan (fpnum.dbl.d))
	{
	  if (isupper (info->spec))
	    {
	      special = "NAN";
	      wspecial = L"NAN";
	    }
	  else
	    {
	      special = "nan";
	      wspecial = L"nan";
	    }
	  negative = 0;
	}
      else
	{
	  if (__isinf (fpnum.dbl.d))
	    {
              if (isupper (info->spec))
		{
		  special = "INF";
		  wspecial = L"INF";
		}
	      else
		{
		  special = "inf";
		  wspecial = L"inf";
		}
	    }

	  negative = signbit (fpnum.dbl.d);
	}
    }

  if (special)
    {
      int width = info->width;

      if (negative || info->showsign || info->space)
	--width;
      width -= 3;

      if (!info->left && width > 0)
	PADN (' ', width);

      if (negative)
	outchar ('-');
      else if (info->showsign)
	outchar ('+');
      else if (info->space)
	outchar (' ');

      PRINT (special, wspecial, 3);

      if (info->left && width > 0)
	PADN (' ', width);

      return !fp->base->conf->malloc_bad;
    }

  if (info->is_long_double == 0 || sizeof (double) == sizeof (long double))
    {
      /* We have 52 bits of mantissa plus one implicit digit.  Since
	 52 bits are representable without rest using hexadecimal
	 digits we use only the implicit digits for the number before
	 the decimal point.  */
      unsigned long long int num;

      num = (((unsigned long long int) fpnum.dbl.ieee.mantissa0) << 32
	     | fpnum.dbl.ieee.mantissa1);

      zero_mantissa = num == 0;

      if (sizeof (unsigned long int) > 6)
	{
	  wnumstr = _itowa_word (num, wnumbuf + (sizeof wnumbuf) / sizeof (wchar_t), 16,
				 info->spec == 'A');
	  numstr = _itoa_word (num, numbuf + sizeof numbuf, 16,
			       info->spec == 'A');
	}
      else
	{
	  wnumstr = _itowa (num, wnumbuf + sizeof wnumbuf / sizeof (wchar_t), 16,
			    info->spec == 'A');
	  numstr = _itoa (num, numbuf + sizeof numbuf, 16,
			  info->spec == 'A');
	}

      /* Fill with zeroes.  */
      while (wnumstr > wnumbuf + (sizeof wnumbuf - 52) / sizeof (wchar_t))
	{
	  *--wnumstr = L'0';
	  *--numstr = '0';
	}

      leading = fpnum.dbl.ieee.exponent == 0 ? '0' : '1';

      exponent = fpnum.dbl.ieee.exponent;

      if (exponent == 0)
	{
	  if (zero_mantissa)
	    expnegative = 0;
	  else
	    {
	      /* This is a denormalized number.  */
	      expnegative = 1;
	      exponent = IEEE754_DOUBLE_BIAS - 1;
	    }
	}
      else if (exponent >= IEEE754_DOUBLE_BIAS)
	{
	  expnegative = 0;
	  exponent -= IEEE754_DOUBLE_BIAS;
	}
      else
	{
	  expnegative = 1;
	  exponent = -(exponent - IEEE754_DOUBLE_BIAS);
	}
    }
#ifdef PRINT_FPHEX_LONG_DOUBLE
  else
    PRINT_FPHEX_LONG_DOUBLE;
#endif

  /* Look for trailing zeroes.  */
  if (! zero_mantissa)
    {
      wnumend = wnumbuf + sizeof wnumbuf;
      numend = numbuf + sizeof numbuf;
      while (wnumend[-1] == L'0')
	{
	  --wnumend;
	  --numend;
	}

      if (precision == -1)
	precision = numend - numstr;
      else if (precision < numend - numstr
	       && (numstr[precision] > '8'
		   || (('A' < '0' || 'a' < '0')
		       && numstr[precision] < '0')
		   || (numstr[precision] == '8'
		       && (precision + 1 < numend - numstr
			   /* Round to even.  */
			   || (precision > 0
			       && ((numstr[precision - 1] & 1)
				   ^ (isdigit (numstr[precision - 1]) == 0)))
			   || (precision == 0
			       && ((leading & 1)
				   ^ (isdigit (leading) == 0)))))))
	{
	  /* Round up.  */
	  int cnt = precision;
	  while (--cnt >= 0)
	    {
	      char ch = numstr[cnt];
	      /* We assume that the digits and the letters are ordered
		 like in ASCII.  This is true for the rest of GNU, too.  */
	      if (ch == '9')
		{
		  wnumstr[cnt] = (wchar_t) info->spec;
		  numstr[cnt] = info->spec;	/* This is tricky,
		  				   think about it!  */
		  break;
		}
	      else if (tolower (ch) < 'f')
		{
		  ++numstr[cnt];
		  ++wnumstr[cnt];
		  break;
		}
	      else
		{
		  numstr[cnt] = '0';
		  wnumstr[cnt] = L'0';
		}
	    }
	  if (cnt < 0)
	    {
	      /* The mantissa so far was fff...f  Now increment the
		 leading digit.  Here it is again possible that we
		 get an overflow.  */
	      if (leading == '9')
		leading = info->spec;
	      else if (tolower (leading) < 'f')
		++leading;
	      else
		{
		  leading = 1;
		  if (expnegative)
		    {
		      exponent += 4;
		      if (exponent >= 0)
			expnegative = 0;
		    }
		  else
		    exponent += 4;
		}
	    }
	}
    }
  else
    {
      if (precision == -1)
	precision = 0;
      numend = numstr;
      wnumend = wnumstr;
    }

  /* Now we can compute the exponent string.  */
  expstr = _itoa_word (exponent, expbuf + sizeof expbuf, 10, 0);
  wexpstr = _itowa_word (exponent,
			 wexpbuf + sizeof wexpbuf / sizeof (wchar_t), 10, 0);

  /* Now we have all information to compute the size.  */
  width -= ((negative || info->showsign || info->space)
	    /* Sign.  */
	    + 2    + 1 + 0 + precision + 1 + 1
	    /* 0x    h   .   hhh         P   ExpoSign.  */
	    + ((expbuf + sizeof expbuf) - expstr));
	    /* Exponent.  */

  /* Count the decimal point.  */
  if (precision > 0 || info->alt)
    width -= wide ? 1 : strlen_decimal;

  /* A special case when the mantissa or the precision is zero and the `#'
     is not given.  In this case we must not print the decimal point.  */
  if (precision == 0 && !info->alt)
    ++width;		/* This nihilates the +1 for the decimal-point
			   character in the following equation.  */

  if (!info->left && width > 0)
    PADN (' ', width);

  if (negative)
    outchar ('-');
  else if (info->showsign)
    outchar ('+');
  else if (info->space)
    outchar (' ');

  outchar ('0');
  if ('X' - 'A' == 'x' - 'a')
    outchar (info->spec + ('x' - 'a'));
  else
    outchar (info->spec == 'A' ? 'X' : 'x');
  outchar (leading);

  if (precision > 0 || info->alt)
    {
      const wchar_t *wtmp = &decimalwc;
      PRINT (decimal, wtmp, wide ? 1 : strlen_decimal);
    }

  if (precision > 0)
    {
      ssize_t tofill = precision - (numend - numstr);
      PRINT (numstr, wnumstr, MIN (numend - numstr, precision));
      if (tofill > 0)
	PADN ('0', tofill);
    }

  if (info->left && info->pad == '0' && width > 0)
    PADN ('0', width);

  if ('P' - 'A' == 'p' - 'a')
    outchar (info->spec + ('p' - 'a'));
  else
    outchar (info->spec == 'A' ? 'P' : 'p');

  outchar (expnegative ? '-' : '+');

  PRINT (expstr, wexpstr, (expbuf + sizeof expbuf) - expstr);

  if (info->left && info->pad != '0' && width > 0)
    PADN (info->pad, width);

  return !fp->base->conf->malloc_bad;
}

/* END: ==== printf_fphex ==== */

static int vstr__add_fmt_dbl(Vstr_base *base, size_t pos_diff,
                             struct Vstr__fmt_spec *spec)
{
  FILE wrap_fp;
  struct vstr__fmt_printf_info info;
  const void *dbl_ptr = NULL;
  
  wrap_fp.base = base;
  wrap_fp.pos_diff = pos_diff;
  wrap_fp.spec = spec;

  if (spec->flags & PREC_USR)
    info.prec = spec->precision;
  else
    info.prec = -1; /* if they didn't supply them then work it out */

  info.width = spec->field_width;
  info.spec = spec->fmt_code;
  
  info.is_long_double = (spec->int_type == LONG_DOUBLE_TYPE);
  info.alt =      !!(spec->flags & SPECIAL);
  info.space =    !!(spec->flags & SPACE);
  info.left =     !!(spec->flags & LEFT);
  info.showsign = !!(spec->flags & PLUS);
  info.group =    !!(spec->flags & THOUSAND_SEP);

  if (spec->flags & ZEROPAD)
    info.pad = '0';
  else
    info.pad = ' ';
  
  if (spec->int_type == LONG_DOUBLE_TYPE)
    dbl_ptr = &spec->u.data_Ld;
  else
    dbl_ptr = &spec->u.data_d;
  
  switch (spec->fmt_code)
  {
    case 'a':
    case 'A':
      return (vstr__fmt_printf_fphex(&wrap_fp, &info, &dbl_ptr));
    case 'e':
    case 'E':
      break;
    case 'f':
    case 'F':
    case 'g':
    case 'G':
      break;

    default:
      assert(FALSE);
  }
  
  return (FALSE);
}

#undef FILE
#undef wide

#undef putc
#undef PUT
#undef PAD
#undef outchar
#undef PRINT
#undef PADN

#undef isupper
#undef __isinf
#undef __isinfl
#undef __isnan
#undef __isnanl
#undef signbit
 
#undef _itoa_word
#undef _itowa_word


