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
/*
  Copyright (C) 1991, 1993, 1994, 1996 Free Software Foundation, Inc.

This file is part of the GNU MP Library.

The GNU MP Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The GNU MP Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */
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
/* NOTE: This file assumes alloca() exists */
/* NOTE: This file includes assembler, works for ia32 atm. */
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
  unsigned int extra:1;         /* For special use.  */
 /* unsigned int is_char:1; */       /* hh flag.  */
 unsigned int wide:1;           /* Nonzero for wide character streams.  */
 /* unsigned int i18n:1; */          /* I flag.  */
  wchar_t pad;                  /* Padding character.  */
};

#ifndef CHAR_MAX /* for compiling against dietlibc */
# define CHAR_MAX SCHAR_MAX
#endif

#undef FILE
#define FILE struct vstr__FILE_wrap  

#undef putc
#define putc(c, f) \
 (vstr_nx_add_rep_chr((f)->base, (f)->base->len - (f)->pos_diff, c, 1) ? \
  (c) : EOF)

#define PUT(f, s, n) \
 vstr_nx_add_buf((f)->base, ((f)->base->len - (f)->pos_diff), s, n)

#define PAD(f, c, n) \
 (vstr_nx_add_rep_chr((f)->base, (f)->base->len - (f)->pos_diff, c, n) * n)

#undef isupper
#define isupper(x) (fp->spec->flags & LARGE)
#undef tolower
#define tolower(x) (VSTR__IS_ASCII_UPPER(x) ? VSTR__TO_ASCII_LOWER(x) : (x))


#ifndef HAVE_INLINE
# undef inline
# define inline /* nothing */
#endif

/* We use the GNU MP library to handle large numbers.

   An MP variable occupies a varying number of entries in its array.  We keep
   track of this number for efficiency reasons.  Otherwise we would always
   have to process the whole array.  */
#define MPN_VAR(name) mp_limb_t *name; mp_size_t name##size

#define MPN_ASSIGN(dst,src)                                                   \
  memcpy (dst, src, (dst##size = src##size) * sizeof (mp_limb_t))
#define MPN_GE(u,v) \
  (u##size > v##size || (u##size == v##size && __mpn_cmp (u, v, u##size) >= 0))

/* Copy NLIMBS *limbs* from SRC to DST.  */
#define MPN_COPY_INCR(DST, SRC, NLIMBS) \
  do {                                                                  \
    mp_size_t __i;                                                      \
    for (__i = 0; __i < (NLIMBS); __i++)                                \
      (DST)[__i] = (SRC)[__i];                                          \
  } while (0)
#define MPN_COPY_DECR(DST, SRC, NLIMBS) \
  do {                                                                  \
    mp_size_t __i;                                                      \
    for (__i = (NLIMBS) - 1; __i >= 0; __i--)                           \
      (DST)[__i] = (SRC)[__i];                                          \
  } while (0)
#define MPN_COPY MPN_COPY_INCR

/* Zero NLIMBS *limbs* AT DST.  */
#define MPN_ZERO(DST, NLIMBS) \
  do {                                                                  \
    mp_size_t __i;                                                      \
    for (__i = 0; __i < (NLIMBS); __i++)                                \
      (DST)[__i] = 0;                                                   \
  } while (0)

#define MPN_MUL_N_RECURSE(prodp, up, vp, size, tspace) \
  do {                                                                  \
    if ((size) < KARATSUBA_THRESHOLD)                                   \
      impn_mul_n_basecase (prodp, up, vp, size);                        \
    else                                                                \
      impn_mul_n (prodp, up, vp, size, tspace);                 \
  } while (0);


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

typedef unsigned long int       mp_limb_t;
typedef long int                mp_limb_signed_t;

typedef mp_limb_t *             mp_ptr;
typedef const mp_limb_t *       mp_srcptr;
typedef long int                mp_size_t;

#ifndef BITS_PER_MP_LIMB
# if defined(__i386__) && defined(__linux__)
#  define BITS_PER_MP_LIMB 32
# endif
#endif

#ifndef BYTES_PER_MP_LIMB
# if defined(__i386__) && defined(__linux__)
#  define BYTES_PER_MP_LIMB 4
# else
#  define BYTES_PER_MP_LIMB sizeof(mp_limb_t)
# endif
#endif


/* arch specific code for mpn */
#if (defined (__i386__) || defined (__i486__))
#define USItype unsigned int
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("addl %5,%1\n"                                               \
        "adcl %3,%0"                                                    \
           : "=r" ((USItype) (sh)),                                     \
             "=&r" ((USItype) (sl))                                     \
           : "%0" ((USItype) (ah)),                                     \
             "g" ((USItype) (bh)),                                      \
             "%1" ((USItype) (al)),                                     \
             "g" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subl %5,%1\n"                                               \
        "sbbl %3,%0"                                                    \
           : "=r" ((USItype) (sh)),                                     \
             "=&r" ((USItype) (sl))                                     \
           : "0" ((USItype) (ah)),                                      \
             "g" ((USItype) (bh)),                                      \
             "1" ((USItype) (al)),                                      \
             "g" ((USItype) (bl)))
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("mull %3"                                                    \
           : "=a" ((USItype) (w0)),                                     \
             "=d" ((USItype) (w1))                                      \
           : "%0" ((USItype) (u)),                                      \
             "rm" ((USItype) (v)))
#define udiv_qrnnd(q, r, n1, n0, d) \
  __asm__ ("divl %4"                                                    \
           : "=a" ((USItype) (q)),                                      \
             "=d" ((USItype) (r))                                       \
           : "0" ((USItype) (n0)),                                      \
             "1" ((USItype) (n1)),                                      \
             "rm" ((USItype) (d)))
#define count_leading_zeros(count, x) \
  do {                                                                  \
    USItype __cbtmp;                                                    \
    __asm__ ("bsrl %1,%0"                                               \
             : "=r" (__cbtmp) : "rm" ((USItype) (x)));                  \
    (count) = __cbtmp ^ 31;                                             \
  } while (0)
#define count_trailing_zeros(count, x) \
  __asm__ ("bsfl %1,%0" : "=r" (count) : "rm" ((USItype)(x)))
#define UMUL_TIME 40
#define UDIV_TIME 40
#endif /* 80x86 */

#ifndef add_ssaaaa
# error "No arch specific code for add_ssaaaa etc."
#endif

#include <float.h> /* LDBL_MIN_EXP FLT_MANT_DIG */ 

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

/* generic ldbl == 128 bits */


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

#define PRINT_FPHEX_LONG_DOUBLE \
do {                                                                          \
      /* We have 112 bits of mantissa plus one implicit digit.  Since         \
         112 bits are representable without rest using hexadecimal            \
         digits we use only the implicit digits for the number before         \
         the decimal point.  */                                               \
      unsigned long long int num0, num1;                                      \
                                                                              \
      assert (sizeof (long double) == 16);                                    \
                                                                              \
      num0 = (((unsigned long long int) fpnum.ldbl.ieee.mantissa0) << 32      \
             | fpnum.ldbl.ieee.mantissa1);                                    \
      num1 = (((unsigned long long int) fpnum.ldbl.ieee.mantissa2) << 32      \
             | fpnum.ldbl.ieee.mantissa3);                                    \
                                                                              \
      zero_mantissa = (num0|num1) == 0;                                       \
                                                                              \
      if (sizeof (unsigned long int) > 6)                                     \
        {                                                                     \
          numstr = _itoa_word (num1, numbuf + sizeof numbuf, 16,              \
                               info->spec == 'A');                            \
          wnumstr = _itowa_word (num1,                                        \
                                 wnumbuf + sizeof (wnumbuf) / sizeof (wchar_t),\
                                 16, info->spec == 'A');                      \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          numstr = _itoa (num1, numbuf + sizeof numbuf, 16,                   \
                          info->spec == 'A');                                 \
          wnumstr = _itowa (num1,                                             \
                            wnumbuf + sizeof (wnumbuf) / sizeof (wchar_t),    \
                            16, info->spec == 'A');                           \
        }                                                                     \
                                                                              \
      while (numstr > numbuf + (sizeof numbuf - 64 / 4))                      \
        {                                                                     \
          *--numstr = '0';                                                    \
          *--wnumstr = L'0';                                                  \
        }                                                                     \
                                                                              \
      if (sizeof (unsigned long int) > 6)                                     \
        {                                                                     \
          numstr = _itoa_word (num0, numstr, 16, info->spec == 'A');          \
          wnumstr = _itowa_word (num0, wnumstr, 16, info->spec == 'A');       \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          numstr = _itoa (num0, numstr, 16, info->spec == 'A');               \
          wnumstr = _itowa (num0, wnumstr, 16, info->spec == 'A');            \
        }                                                                     \
                                                                              \
      /* Fill with zeroes.  */                                                \
      while (numstr > numbuf + (sizeof numbuf - 112 / 4))                     \
        {                                                                     \
          *--numstr = '0';                                                    \
          *--wnumstr = L'0';                                                  \
        }                                                                     \
                                                                              \
      leading = fpnum.ldbl.ieee.exponent == 0 ? '0' : '1';                    \
                                                                              \
      exponent = fpnum.ldbl.ieee.exponent;                                    \
                                                                              \
      if (exponent == 0)                                                      \
        {                                                                     \
          if (zero_mantissa)                                                  \
            expnegative = 0;                                                  \
          else                                                                \
            {                                                                 \
              /* This is a denormalized number.  */                           \
              expnegative = 1;                                                \
              exponent = IEEE854_LONG_DOUBLE_BIAS - 1;                        \
            }                                                                 \
        }                                                                     \
      else if (exponent >= IEEE854_LONG_DOUBLE_BIAS)                          \
        {                                                                     \
          expnegative = 0;                                                    \
          exponent -= IEEE854_LONG_DOUBLE_BIAS;                               \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          expnegative = 1;                                                    \
          exponent = -(exponent - IEEE854_LONG_DOUBLE_BIAS);                  \
        }                                                                     \
} while (0)

/* Convert a `long double' in IEEE854 quad-precision format to a
   multi-precision integer representing the significand scaled up by its
   number of bits (113 for long double) and an integral power of two
   (MPN frexpl). */

static mp_size_t
vstr__fmt_dbl_mpn_extract_long_double (mp_ptr res_ptr, mp_size_t size,
                                       int *expt, int *is_neg,
                                       long double value)
{
  union ieee854_long_double u;
  u.d = value;

  *is_neg = u.ieee.negative;
  *expt = (int) u.ieee.exponent - IEEE854_LONG_DOUBLE_BIAS;

#if BITS_PER_MP_LIMB == 32
  res_ptr[0] = u.ieee.mantissa3; /* Low-order 32 bits of fraction.  */
  res_ptr[1] = u.ieee.mantissa2;
  res_ptr[2] = u.ieee.mantissa1;
  res_ptr[3] = u.ieee.mantissa0; /* High-order 32 bits.  */
  #define N 4
#elif BITS_PER_MP_LIMB == 64
  /* Hopefully the compiler will combine the two bitfield extracts
     and this composition into just the original quadword extract.  */
  res_ptr[0] = ((unsigned long int) u.ieee.mantissa2 << 32) | u.ieee.mantissa3;
  res_ptr[1] = ((unsigned long int) u.ieee.mantissa0 << 32) | u.ieee.mantissa1;
  #define N 2
#else
  #error "mp_limb size " BITS_PER_MP_LIMB "not accounted for"
#endif
/* The format does not fill the last limb.  There are some zeros.  */
#define NUM_LEADING_ZEROS (BITS_PER_MP_LIMB \
                           - (LDBL_MANT_DIG - ((N - 1) * BITS_PER_MP_LIMB)))

  if (u.ieee.exponent == 0)
    {
      /* A biased exponent of zero is a special case.
         Either it is a zero or it is a denormal number.  */
      if (res_ptr[0] == 0 && res_ptr[1] == 0
          && res_ptr[N - 2] == 0 && res_ptr[N - 1] == 0) /* Assumes N<=4.  */
        /* It's zero.  */
        *expt = 0;
      else
        {
          /* It is a denormal number, meaning it has no implicit leading
             one bit, and its exponent is in fact the format minimum.  */
          int cnt;

#if N == 2
          if (res_ptr[N - 1] != 0)
            {
              count_leading_zeros (cnt, res_ptr[N - 1]);
              cnt -= NUM_LEADING_ZEROS;
              res_ptr[N - 1] = res_ptr[N - 1] << cnt
                               | (res_ptr[0] >> (BITS_PER_MP_LIMB - cnt));
              res_ptr[0] <<= cnt;
              *expt = LDBL_MIN_EXP - 1 - cnt;
            }
          else
            {
              count_leading_zeros (cnt, res_ptr[0]);
              if (cnt >= NUM_LEADING_ZEROS)
                {
                  res_ptr[N - 1] = res_ptr[0] << (cnt - NUM_LEADING_ZEROS);
                  res_ptr[0] = 0;
                }
              else
                {
                  res_ptr[N - 1] = res_ptr[0] >> (NUM_LEADING_ZEROS - cnt);
                  res_ptr[0] <<= BITS_PER_MP_LIMB - (NUM_LEADING_ZEROS - cnt);
                }
              *expt = LDBL_MIN_EXP - 1
                - (BITS_PER_MP_LIMB - NUM_LEADING_ZEROS) - cnt;
            }
#else
          int j, k, l;

          for (j = N - 1; j > 0; j--)
            if (res_ptr[j] != 0)
              break;

          count_leading_zeros (cnt, res_ptr[j]);
          cnt -= NUM_LEADING_ZEROS;
          l = N - 1 - j;
          if (cnt < 0)
            {
              cnt += BITS_PER_MP_LIMB;
              l--;
            }
          if (!cnt)
            for (k = N - 1; k >= l; k--)
              res_ptr[k] = res_ptr[k-l];
          else
            {
              for (k = N - 1; k > l; k--)
                res_ptr[k] = res_ptr[k-l] << cnt
                             | res_ptr[k-l-1] >> (BITS_PER_MP_LIMB - cnt);
              res_ptr[k--] = res_ptr[0] << cnt;
            }

          for (; k >= 0; k--)
            res_ptr[k] = 0;
          *expt = LDBL_MIN_EXP - 1 - l * BITS_PER_MP_LIMB - cnt;
#endif
        }
    }
  else
    /* Add the implicit leading one bit for a normalized number.  */
    res_ptr[N - 1] |= 1L << (LDBL_MANT_DIG - 1 - ((N - 1) * BITS_PER_MP_LIMB));

  return N;
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

/* generic ldbl == 96 bits */



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

#ifndef LONG_DOUBLE_DENORM_BIAS
# define LONG_DOUBLE_DENORM_BIAS (IEEE854_LONG_DOUBLE_BIAS - 1)
#endif

#define PRINT_FPHEX_LONG_DOUBLE \
do {                                                                          \
      /* The "strange" 80 bit format on ix86 and m68k has an explicit         \
         leading digit in the 64 bit mantissa.  */                            \
      unsigned long long int num;                                             \
                                                                              \
      assert (sizeof (long double) == 12);                                    \
                                                                              \
      num = (((unsigned long long int) fpnum.ldbl.ieee.mantissa0) << 32       \
             | fpnum.ldbl.ieee.mantissa1);                                    \
                                                                              \
      zero_mantissa = num == 0;                                               \
                                                                              \
      if (sizeof (unsigned long int) > 6)                                     \
        {                                                                     \
          numstr = _itoa_word (num, numbuf + sizeof numbuf, 16,               \
                               info->spec == 'A');                            \
          wnumstr = _itowa_word (num,                                         \
                                 wnumbuf + sizeof (wnumbuf) / sizeof (wchar_t),\
                                 16, info->spec == 'A');                      \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          numstr = _itoa (num, numbuf + sizeof numbuf, 16, info->spec == 'A');\
          wnumstr = _itowa (num,                                              \
                            wnumbuf + sizeof (wnumbuf) / sizeof (wchar_t),    \
                            16, info->spec == 'A');                           \
        }                                                                     \
                                                                              \
      /* Fill with zeroes.  */                                                \
      while (numstr > numbuf + (sizeof numbuf - 64 / 4))                      \
        {                                                                     \
          *--numstr = '0';                                                    \
          *--wnumstr = L'0';                                                  \
        }                                                                     \
                                                                              \
      /* We use a full nibble for the leading digit.  */                      \
      leading = *numstr++;                                                    \
                                                                              \
      /* We have 3 bits from the mantissa in the leading nibble.              \
         Therefore we are here using `IEEE854_LONG_DOUBLE_BIAS + 3'.  */      \
      exponent = fpnum.ldbl.ieee.exponent;                                    \
                                                                              \
      if (exponent == 0)                                                      \
        {                                                                     \
          if (zero_mantissa)                                                  \
            expnegative = 0;                                                  \
          else                                                                \
            {                                                                 \
              /* This is a denormalized number.  */                           \
              expnegative = 1;                                                \
              /* This is a hook for the m68k long double format, where the    \
                 exponent bias is the same for normalized and denormalized    \
                 numbers.  */                                                 \
              exponent = LONG_DOUBLE_DENORM_BIAS + 3;                         \
            }                                                                 \
        }                                                                     \
      else if (exponent >= IEEE854_LONG_DOUBLE_BIAS + 3)                      \
        {                                                                     \
          expnegative = 0;                                                    \
          exponent -= IEEE854_LONG_DOUBLE_BIAS + 3;                           \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          expnegative = 1;                                                    \
          exponent = -(exponent - (IEEE854_LONG_DOUBLE_BIAS + 3));            \
        }                                                                     \
} while (0)

/* NOTE NOTE NOTE: glibc has two versions of this,
   one in ./sysdeps/i386/ldbl2mpn.c
   one in ./sysdeps/ieee754/ldbl-96/ldbl2mpn.c
   * The first one works around bugs on the i386 CPU... */

#if defined (__i386__)
/* Convert a `long double' in IEEE854 standard double-precision format to a
   multi-precision integer representing the significand scaled up by its
   number of bits (64 for long double) and an integral power of two
   (MPN frexpl). */

static mp_size_t
vstr__fmt_dbl_mpn_extract_long_double (mp_ptr res_ptr,
                                       mp_size_t size __attribute__((unused)),
                                       int *expt, int *is_neg,
                                       long double value)
{
  union ieee854_long_double u;
  u.d = value;

  *is_neg = u.ieee.negative;
  *expt = (int) u.ieee.exponent - IEEE854_LONG_DOUBLE_BIAS;

#if BITS_PER_MP_LIMB == 32
  res_ptr[0] = u.ieee.mantissa1; /* Low-order 32 bits of fraction.  */
  res_ptr[1] = u.ieee.mantissa0; /* High-order 32 bits.  */
# define N 2
#elif BITS_PER_MP_LIMB == 64
  /* Hopefully the compiler will combine the two bitfield extracts
     and this composition into just the original quadword extract.  */
  res_ptr[0] = ((unsigned long int) u.ieee.mantissa0 << 32) | u.ieee.mantissa1;
# define N 1
#else
# error "mp_limb size " BITS_PER_MP_LIMB "not accounted for"
#endif

  if (u.ieee.exponent == 0)
    {
      /* A biased exponent of zero is a special case.
         Either it is a zero or it is a denormal number.  */
      if (res_ptr[0] == 0 && res_ptr[N - 1] == 0) /* Assumes N<=2.  */
        /* It's zero.  */
        *expt = 0;
      else
        {
          /* It is a denormal number, meaning it has no implicit leading
             one bit, and its exponent is in fact the format minimum.  */
          int cnt;

          /* One problem with Intel's 80-bit format is that the explicit
             leading one in the normalized representation has to be zero
             for denormalized number.  If it is one, the number is according
             to Intel's specification an invalid number.  We make the
             representation unique by explicitly clearing this bit.  */
          res_ptr[N - 1] &= ~(1L << ((LDBL_MANT_DIG - 1) % BITS_PER_MP_LIMB));

          if (res_ptr[N - 1] != 0)
            {
              count_leading_zeros (cnt, res_ptr[N - 1]);
              if (cnt != 0)
                {
#if N == 2
                  res_ptr[N - 1] = res_ptr[N - 1] << cnt
                                   | (res_ptr[0] >> (BITS_PER_MP_LIMB - cnt));
                  res_ptr[0] <<= cnt;
#else
                  res_ptr[N - 1] <<= cnt;
#endif
                }
              *expt = LDBL_MIN_EXP - 1 - cnt;
            }
          else if (res_ptr[0] != 0)
            {
              count_leading_zeros (cnt, res_ptr[0]);
              res_ptr[N - 1] = res_ptr[0] << cnt;
              res_ptr[0] = 0;
              *expt = LDBL_MIN_EXP - 1 - BITS_PER_MP_LIMB - cnt;
            }
          else
            {
              /* This is the special case of the pseudo denormal number
                 with only the implicit leading bit set.  The value is
                 in fact a normal number and so we have to treat this
                 case differently.  */
#if N == 2
              res_ptr[N - 1] = 0x80000000ul;
#else
              res_ptr[0] = 0x8000000000000000ul;
#endif
              *expt = LDBL_MIN_EXP - 1;
            }
        }
    }

  return N;
}
#else
/* Convert a `long double' in IEEE854 standard double-precision format to a
   multi-precision integer representing the significand scaled up by its
   number of bits (64 for long double) and an integral power of two
   (MPN frexpl). */

static mp_size_t
vstr__fmt_dbl_mpn_extract_long_double (mp_ptr res_ptr,
                                       mp_size_t size __attribute__((unused)),
                                       int *expt, int *is_neg,
                                       long double value)
{
  union ieee854_long_double u;
  u.d = value;

  *is_neg = u.ieee.negative;
  *expt = (int) u.ieee.exponent - IEEE854_LONG_DOUBLE_BIAS;

#if BITS_PER_MP_LIMB == 32
  res_ptr[0] = u.ieee.mantissa1; /* Low-order 32 bits of fraction.  */
  res_ptr[1] = u.ieee.mantissa0; /* High-order 32 bits.  */
# define N 2
#elif BITS_PER_MP_LIMB == 64
  /* Hopefully the compiler will combine the two bitfield extracts
     and this composition into just the original quadword extract.  */
  res_ptr[0] = ((unsigned long int) u.ieee.mantissa0 << 32) | u.ieee.mantissa1;
# define N 1
#else
# error "mp_limb size " BITS_PER_MP_LIMB "not accounted for"
#endif

  if (u.ieee.exponent == 0)
    {
      /* A biased exponent of zero is a special case.
         Either it is a zero or it is a denormal number.  */
      if (res_ptr[0] == 0 && res_ptr[N - 1] == 0) /* Assumes N<=2.  */
        /* It's zero.  */
        *expt = 0;
      else
        {
          /* It is a denormal number, meaning it has no implicit leading
             one bit, and its exponent is in fact the format minimum.  */
          int cnt;

          if (res_ptr[N - 1] != 0)
            {
              count_leading_zeros (cnt, res_ptr[N - 1]);
              if (cnt != 0)
                {
#if N == 2
                  res_ptr[N - 1] = res_ptr[N - 1] << cnt
                                   | (res_ptr[0] >> (BITS_PER_MP_LIMB - cnt));
                  res_ptr[0] <<= cnt;
#else
                  res_ptr[N - 1] <<= cnt;
#endif
                }
              *expt = LDBL_MIN_EXP - 1 - cnt;
            }
          else
            {
              count_leading_zeros (cnt, res_ptr[0]);
              res_ptr[N - 1] = res_ptr[0] << cnt;
              res_ptr[0] = 0;
              *expt = LDBL_MIN_EXP - 1 - BITS_PER_MP_LIMB - cnt;
            }
        }
    }

  return N;
}
#endif

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

static inline int vstr__fmt_dbl_signbitl(long double x)
{
  int32_t se,hx,lx;
  GET_LDOUBLE_WORDS(se,hx,lx,x);
  return se & 0x8000;
}

#undef signbitl
#define signbitl(x) vstr__fmt_dbl_signbitl(x)

/* write the number backwards ... */
static inline char *vstr__fmt_dbl_itoa_word(unsigned long value, char *buflim,
                                            unsigned int base,
                                            int upper_case)
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
                                                int upper_case __attribute__((unused)))
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
                                           unsigned int base,
                                           int upper_case __attribute__((unused)))
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

#undef __mpn_extract_long_double
#define __mpn_extract_long_double vstr__fmt_dbl_mpn_extract_long_double

/* Convert a `double' in IEEE754 standard double-precision format to a
   multi-precision integer representing the significand scaled up by its
   number of bits (52 for double) and an integral power of two (MPN frexp). */

static mp_size_t
vstr__fmt_dbl_mpn_extract_double (mp_ptr res_ptr,
                                  mp_size_t size __attribute__((unused)),
                                  int *expt, int *is_neg,
                                  double value)
{
  union ieee754_double u;
  u.d = value;

  *is_neg = u.ieee.negative;
  *expt = (int) u.ieee.exponent - IEEE754_DOUBLE_BIAS;

#if BITS_PER_MP_LIMB == 32
  res_ptr[0] = u.ieee.mantissa1; /* Low-order 32 bits of fraction.  */
  res_ptr[1] = u.ieee.mantissa0; /* High-order 20 bits.  */
  #define N 2
#elif BITS_PER_MP_LIMB == 64
  /* Hopefully the compiler will combine the two bitfield extracts
     and this composition into just the original quadword extract.  */
  res_ptr[0] = ((unsigned long int) u.ieee.mantissa0 << 32) | u.ieee.mantissa1;
  #define N 1
#else
  #error "mp_limb size " BITS_PER_MP_LIMB "not accounted for"
#endif
/* The format does not fill the last limb.  There are some zeros.  */
#define NUM_LEADING_ZEROS (BITS_PER_MP_LIMB \
                           - (DBL_MANT_DIG - ((N - 1) * BITS_PER_MP_LIMB)))

  if (u.ieee.exponent == 0)
    {
      /* A biased exponent of zero is a special case.
         Either it is a zero or it is a denormal number.  */
      if (res_ptr[0] == 0 && res_ptr[N - 1] == 0) /* Assumes N<=2.  */
        /* It's zero.  */
        *expt = 0;
      else
        {
          /* It is a denormal number, meaning it has no implicit leading
             one bit, and its exponent is in fact the format minimum.  */
          int cnt;

          if (res_ptr[N - 1] != 0)
            {
              count_leading_zeros (cnt, res_ptr[N - 1]);
              cnt -= NUM_LEADING_ZEROS;
#if N == 2
              res_ptr[N - 1] = res_ptr[1] << cnt
                               | (N - 1)
                                 * (res_ptr[0] >> (BITS_PER_MP_LIMB - cnt));
              res_ptr[0] <<= cnt;
#else
              res_ptr[N - 1] <<= cnt;
#endif
              *expt = DBL_MIN_EXP - 1 - cnt;
            }
          else
            {
              count_leading_zeros (cnt, res_ptr[0]);
              if (cnt >= NUM_LEADING_ZEROS)
                {
                  res_ptr[N - 1] = res_ptr[0] << (cnt - NUM_LEADING_ZEROS);
                  res_ptr[0] = 0;
                }
              else
                {
                  res_ptr[N - 1] = res_ptr[0] >> (NUM_LEADING_ZEROS - cnt);
                  res_ptr[0] <<= BITS_PER_MP_LIMB - (NUM_LEADING_ZEROS - cnt);
                }
              *expt = DBL_MIN_EXP - 1
                      - (BITS_PER_MP_LIMB - NUM_LEADING_ZEROS) - cnt;
            }
        }
    }
  else
    /* Add the implicit leading one bit for a normalized number.  */
    res_ptr[N - 1] |= 1L << (DBL_MANT_DIG - 1 - ((N - 1) * BITS_PER_MP_LIMB));

  return N;
}

#undef __mpn_extract_double
#define __mpn_extract_double vstr__fmt_dbl_mpn_extract_double

#ifndef USE_MPN_C_VERSION
#define USE_MPN_C_VERSION 1
#endif

#if USE_MPN_C_VERSION
/* Generic MPN code ... */
static int
vstr__fmt_dbl_mpn_cmp (mp_srcptr op1_ptr, mp_srcptr op2_ptr, mp_size_t size)
{
  mp_size_t i;
  mp_limb_t op1_word, op2_word;

  for (i = size - 1; i >= 0; i--)
    {
      op1_word = op1_ptr[i];
      op2_word = op2_ptr[i];
      if (op1_word != op2_word)
        goto diff;
    }
  return 0;
 diff:
  /* This can *not* be simplified to
        op2_word - op2_word
     since that expression might give signed overflow.  */
  return (op1_word > op2_word) ? 1 : -1;
}

#undef __mpn_cmp
#define __mpn_cmp vstr__fmt_dbl_mpn_cmp
#undef mpn_cmp
#define mpn_cmp vstr__fmt_dbl_mpn_cmp

static mp_limb_t
vstr__fmt_dbl_mpn_add_1 (register mp_ptr res_ptr,
                         register mp_srcptr s1_ptr,
                         register mp_size_t s1_size,
                         register mp_limb_t s2_limb)
{
  register mp_limb_t x;

  x = *s1_ptr++;
  s2_limb = x + s2_limb;
  *res_ptr++ = s2_limb;
  if (s2_limb < x)
    {
      while (--s1_size != 0)
        {
          x = *s1_ptr++ + 1;
          *res_ptr++ = x;
          if (x != 0)
            goto fin;
        }

      return 1;
    }

 fin:
  if (res_ptr != s1_ptr)
    {
      mp_size_t i;
      for (i = 0; i < s1_size - 1; i++)
        res_ptr[i] = s1_ptr[i];
    }
  return 0;
}

#undef mpn_add_1
#define mpn_add_1 vstr__fmt_dbl_mpn_add_1

static mp_limb_t
vstr__fmt_dbl_mpn_add_n (mp_ptr res_ptr, mp_srcptr s1_ptr,
                         mp_srcptr s2_ptr, mp_size_t size)
{
  register mp_limb_t x, y, cy;
  register mp_size_t j;

  /* The loop counter and index J goes from -SIZE to -1.  This way
     the loop becomes faster.  */
  j = -size;

  /* Offset the base pointers to compensate for the negative indices.  */
  s1_ptr -= j;
  s2_ptr -= j;
  res_ptr -= j;

  cy = 0;
  do
    {
      y = s2_ptr[j];
      x = s1_ptr[j];
      y += cy;                  /* add previous carry to one addend */
      cy = (y < cy);            /* get out carry from that addition */
      y = x + y;                /* add other addend */
      cy = (y < x) + cy;        /* get out carry from that add, combine */
      res_ptr[j] = y;
    }
  while (++j != 0);

  return cy;
}

#undef mpn_add_n
#define mpn_add_n vstr__fmt_dbl_mpn_add_n

static mp_limb_t
vstr__fmt_dbl_mpn_sub_n (mp_ptr res_ptr, mp_srcptr s1_ptr,
                         mp_srcptr s2_ptr, mp_size_t size)
{
  register mp_limb_t x, y, cy;
  register mp_size_t j;

  /* The loop counter and index J goes from -SIZE to -1.  This way
     the loop becomes faster.  */
  j = -size;

  /* Offset the base pointers to compensate for the negative indices.  */
  s1_ptr -= j;
  s2_ptr -= j;
  res_ptr -= j;

  cy = 0;
  do
    {
      y = s2_ptr[j];
      x = s1_ptr[j];
      y += cy;                  /* add previous carry to subtrahend */
      cy = (y < cy);            /* get out carry from that addition */
      y = x - y;                /* main subtract */
      cy = (y > x) + cy;        /* get out carry from the subtract, combine */
      res_ptr[j] = y;
    }
  while (++j != 0);

  return cy;
}

#undef mpn_sub_n
#define mpn_sub_n vstr__fmt_dbl_mpn_sub_n

static mp_limb_t
vstr__fmt_dbl_mpn_addmul_1(mp_ptr res_ptr, mp_srcptr s1_ptr,
                           mp_size_t s1_size, mp_limb_t s2_limb)
{
  mp_limb_t cy_limb;
  mp_size_t j;
  mp_limb_t prod_high, prod_low;
  mp_limb_t x;

  /* The loop counter and index J goes from -SIZE to -1.  This way
     the loop becomes faster.  */
  j = -s1_size;

  /* Offset the base pointers to compensate for the negative indices.  */
  res_ptr -= j;
  s1_ptr -= j;

  cy_limb = 0;
  do
    {
      umul_ppmm (prod_high, prod_low, s1_ptr[j], s2_limb);

      prod_low += cy_limb;
      cy_limb = (prod_low < cy_limb) + prod_high;

      x = res_ptr[j];
      prod_low = x + prod_low;
      cy_limb += (prod_low < x);
      res_ptr[j] = prod_low;
    }
  while (++j != 0);

  return cy_limb;
}

#undef mpn_addmul_1
#define mpn_addmul_1 vstr__fmt_dbl_mpn_addmul_1

static mp_limb_t
vstr__fmt_dbl_mpn_submul_1 (mp_ptr res_ptr, mp_srcptr s1_ptr,
                            mp_size_t s1_size, mp_limb_t s2_limb)
{
  register mp_limb_t cy_limb;
  register mp_size_t j;
  register mp_limb_t prod_high, prod_low;
  register mp_limb_t x;

  /* The loop counter and index J goes from -SIZE to -1.  This way
     the loop becomes faster.  */
  j = -s1_size;

  /* Offset the base pointers to compensate for the negative indices.  */
  res_ptr -= j;
  s1_ptr -= j;

  cy_limb = 0;
  do
    {
      umul_ppmm (prod_high, prod_low, s1_ptr[j], s2_limb);

      prod_low += cy_limb;
      cy_limb = (prod_low < cy_limb) + prod_high;

      x = res_ptr[j];
      prod_low = x - prod_low;
      cy_limb += (prod_low > x);
      res_ptr[j] = prod_low;
    }
  while (++j != 0);

  return cy_limb;
}

#undef mpn_submul_1
#define mpn_submul_1 vstr__fmt_dbl_mpn_submul_1

/* Divide num (NP/NSIZE) by den (DP/DSIZE) and write
   the NSIZE-DSIZE least significant quotient limbs at QP
   and the DSIZE long remainder at NP.  If QEXTRA_LIMBS is
   non-zero, generate that many fraction bits and append them after the
   other quotient limbs.
   Return the most significant limb of the quotient, this is always 0 or 1.

   Preconditions:
   0. NSIZE >= DSIZE.
   1. The most significant bit of the divisor must be set.
   2. QP must either not overlap with the input operands at all, or
      QP + DSIZE >= NP must hold true.  (This means that it's
      possible to put the quotient in the high part of NUM, right after the
      remainder in NUM.
   3. NSIZE >= DSIZE, even if QEXTRA_LIMBS is non-zero.  */

static mp_limb_t
vstr__fmt_dbl_mpn_divrem (mp_ptr qp, mp_size_t qextra_limbs,
                          mp_ptr np, mp_size_t nsize,
                          mp_srcptr dp, mp_size_t dsize)
{
  mp_limb_t most_significant_q_limb = 0;

  switch (dsize)
    {
    case 0:
      /* We are asked to divide by zero, so go ahead and do it!  (To make
         the compiler not remove this statement, return the value.)  */
      return 1 / dsize;

    case 1:
      {
        mp_size_t i;
        mp_limb_t n1;
        mp_limb_t d;

        d = dp[0];
        n1 = np[nsize - 1];

        if (n1 >= d)
          {
            n1 -= d;
            most_significant_q_limb = 1;
          }

        qp += qextra_limbs;
        for (i = nsize - 2; i >= 0; i--)
          udiv_qrnnd (qp[i], n1, n1, np[i], d);
        qp -= qextra_limbs;

        for (i = qextra_limbs - 1; i >= 0; i--)
          udiv_qrnnd (qp[i], n1, n1, 0, d);

        np[0] = n1;
      }
      break;

    case 2:
      {
        mp_size_t i;
        mp_limb_t n1, n0, n2;
        mp_limb_t d1, d0;

        np += nsize - 2;
        d1 = dp[1];
        d0 = dp[0];
        n1 = np[1];
        n0 = np[0];

        if (n1 >= d1 && (n1 > d1 || n0 >= d0))
          {
            sub_ddmmss (n1, n0, n1, n0, d1, d0);
            most_significant_q_limb = 1;
          }

        for (i = qextra_limbs + nsize - 2 - 1; i >= 0; i--)
          {
            mp_limb_t q;
            mp_limb_t r;

            if (i >= qextra_limbs)
              np--;
            else
              np[0] = 0;

            if (n1 == d1)
              {
                /* Q should be either 111..111 or 111..110.  Need special
                   treatment of this rare case as normal division would
                   give overflow.  */
                q = ~(mp_limb_t) 0;

                r = n0 + d1;
                if (r < d1)     /* Carry in the addition? */
                  {
                    add_ssaaaa (n1, n0, r - d0, np[0], 0, d0);
                    qp[i] = q;
                    continue;
                  }
                n1 = d0 - (d0 != 0);
                n0 = -d0;
              }
            else
              {
                udiv_qrnnd (q, r, n1, n0, d1);
                umul_ppmm (n1, n0, d0, q);
              }

            n2 = np[0];
          q_test:
            if (n1 > r || (n1 == r && n0 > n2))
              {
                /* The estimated Q was too large.  */
                q--;

                sub_ddmmss (n1, n0, n1, n0, 0, d0);
                r += d1;
                if (r >= d1)    /* If not carry, test Q again.  */
                  goto q_test;
              }

            qp[i] = q;
            sub_ddmmss (n1, n0, r, n2, n1, n0);
          }
        np[1] = n1;
        np[0] = n0;
      }
      break;

    default:
      {
        mp_size_t i;
        mp_limb_t dX, d1, n0;

        np += nsize - dsize;
        dX = dp[dsize - 1];
        d1 = dp[dsize - 2];
        n0 = np[dsize - 1];

        if (n0 >= dX)
          {
            if (n0 > dX || mpn_cmp (np, dp, dsize - 1) >= 0)
              {
                mpn_sub_n (np, np, dp, dsize);
                n0 = np[dsize - 1];
                most_significant_q_limb = 1;
              }
          }

        for (i = qextra_limbs + nsize - dsize - 1; i >= 0; i--)
          {
            mp_limb_t q;
            mp_limb_t n1, n2;
            mp_limb_t cy_limb;

            if (i >= qextra_limbs)
              {
                np--;
                n2 = np[dsize];
              }
            else
              {
                n2 = np[dsize - 1];
                MPN_COPY_DECR (np + 1, np, dsize);
                np[0] = 0;
              }

            if (n0 == dX)
              /* This might over-estimate q, but it's probably not worth
                 the extra code here to find out.  */
              q = ~(mp_limb_t) 0;
            else
              {
                mp_limb_t r;

                udiv_qrnnd (q, r, n0, np[dsize - 1], dX);
                umul_ppmm (n1, n0, d1, q);

                while (n1 > r || (n1 == r && n0 > np[dsize - 2]))
                  {
                    q--;
                    r += dX;
                    if (r < dX) /* I.e. "carry in previous addition?"  */
                      break;
                    n1 -= n0 < d1;
                    n0 -= d1;
                  }
              }

            /* Possible optimization: We already have (q * n0) and (1 * n1)
               after the calculation of q.  Taking advantage of that, we
               could make this loop make two iterations less.  */

            cy_limb = mpn_submul_1 (np, dp, dsize, q);

            if (n2 != cy_limb)
              {
                mpn_add_n (np, np, dp, dsize);
                q--;
              }

            qp[i] = q;
            n0 = np[dsize - 1];
          }
      }
    }

  return most_significant_q_limb;
}

#undef mpn_divrem
#define mpn_divrem vstr__fmt_dbl_mpn_divrem
#undef mpn_divmod
#define mpn_divmod(qp,np,nsize,dp,dsize) mpn_divrem (qp,0,np,nsize,dp,dsize)

static mp_limb_t
vstr__fmt_dbl_mpn_mul_1 (mp_ptr res_ptr, mp_srcptr s1_ptr,
                         mp_size_t s1_size, mp_limb_t s2_limb)
{
  register mp_limb_t cy_limb;
  register mp_size_t j;
  register mp_limb_t prod_high, prod_low;

  /* The loop counter and index J goes from -S1_SIZE to -1.  This way
     the loop becomes faster.  */
  j = -s1_size;

  /* Offset the base pointers to compensate for the negative indices.  */
  s1_ptr -= j;
  res_ptr -= j;

  cy_limb = 0;
  do
    {
      umul_ppmm (prod_high, prod_low, s1_ptr[j], s2_limb);

      prod_low += cy_limb;
      cy_limb = (prod_low < cy_limb) + prod_high;

      res_ptr[j] = prod_low;
    }
  while (++j != 0);

  return cy_limb;
}

#undef __mpn_mul_1
#define __mpn_mul_1 vstr__fmt_dbl_mpn_mul_1
#undef mpn_mul_1
#define mpn_mul_1 vstr__fmt_dbl_mpn_mul_1

/* for below ... */
#define TMP_DECL(m)
#define TMP_ALLOC(x) alloca(x)
#define TMP_MARK(m)
#define TMP_FREE(m)

/* Handle simple cases with traditional multiplication.

   This is the most critical code of multiplication.  All multiplies rely
   on this, both small and huge.  Small ones arrive here immediately.  Huge
   ones arrive here as this is the base case for Karatsuba's recursive
   algorithm below.  */

static void
vstr__fmt_dbl_impn_mul_n_basecase (mp_ptr prodp, mp_srcptr up,
                                   mp_srcptr vp, mp_size_t size)
{
  mp_size_t i;
  mp_limb_t cy_limb;
  mp_limb_t v_limb;

  /* Multiply by the first limb in V separately, as the result can be
     stored (not added) to PROD.  We also avoid a loop for zeroing.  */
  v_limb = vp[0];
  if (v_limb <= 1)
    {
      if (v_limb == 1)
        MPN_COPY (prodp, up, size);
      else
        MPN_ZERO (prodp, size);
      cy_limb = 0;
    }
  else
    cy_limb = mpn_mul_1 (prodp, up, size, v_limb);

  prodp[size] = cy_limb;
  prodp++;

  /* For each iteration in the outer loop, multiply one limb from
     U with one limb from V, and add it to PROD.  */
  for (i = 1; i < size; i++)
    {
      v_limb = vp[i];
      if (v_limb <= 1)
        {
          cy_limb = 0;
          if (v_limb == 1)
            cy_limb = mpn_add_n (prodp, prodp, up, size);
        }
      else
        cy_limb = mpn_addmul_1 (prodp, up, size, v_limb);

      prodp[size] = cy_limb;
      prodp++;
    }
}

#undef impn_mul_n_basecase
#define impn_mul_n_basecase vstr__fmt_dbl_impn_mul_n_basecase

/* If KARATSUBA_THRESHOLD is not already defined, define it to a
   value which is good on most machines.  */
#ifndef KARATSUBA_THRESHOLD
#define KARATSUBA_THRESHOLD 32
#endif

/* recursive */
#undef impn_mul_n
#define impn_mul_n vstr__fmt_dbl_impn_mul_n

static void
vstr__fmt_dbl_impn_mul_n (mp_ptr prodp,
                          mp_srcptr up, mp_srcptr vp, mp_size_t size,
                          mp_ptr tspace)
{
  if ((size & 1) != 0)
    {
      /* The size is odd, the code code below doesn't handle that.
         Multiply the least significant (size - 1) limbs with a recursive
         call, and handle the most significant limb of S1 and S2
         separately.  */
      /* A slightly faster way to do this would be to make the Karatsuba
         code below behave as if the size were even, and let it check for
         odd size in the end.  I.e., in essence move this code to the end.
         Doing so would save us a recursive call, and potentially make the
         stack grow a lot less.  */

      mp_size_t esize = size - 1;       /* even size */
      mp_limb_t cy_limb;

      MPN_MUL_N_RECURSE (prodp, up, vp, esize, tspace);
      cy_limb = mpn_addmul_1 (prodp + esize, up, esize, vp[esize]);
      prodp[esize + esize] = cy_limb;
      cy_limb = mpn_addmul_1 (prodp + esize, vp, size, up[esize]);

      prodp[esize + size] = cy_limb;
    }
  else
    {
      /* Anatolij Alekseevich Karatsuba's divide-and-conquer algorithm.

         Split U in two pieces, U1 and U0, such that
         U = U0 + U1*(B**n),
         and V in V1 and V0, such that
         V = V0 + V1*(B**n).

         UV is then computed recursively using the identity

                2n   n          n                     n
         UV = (B  + B )U V  +  B (U -U )(V -V )  +  (B + 1)U V
                        1 1        1  0   0  1              0 0

         Where B = 2**BITS_PER_MP_LIMB.  */

      mp_size_t hsize = size >> 1;
      mp_limb_t cy;
      int negflg;

      /*** Product H.    ________________  ________________
                        |_____U1 x V1____||____U0 x V0_____|  */
      if (mpn_cmp (up + hsize, up, hsize) >= 0)
        {
          mpn_sub_n (prodp, up + hsize, up, hsize);
          negflg = 0;
        }
      else
        {
          mpn_sub_n (prodp, up, up + hsize, hsize);
          negflg = 1;
        }
      if (mpn_cmp (vp + hsize, vp, hsize) >= 0)
        {
          mpn_sub_n (prodp + hsize, vp + hsize, vp, hsize);
          negflg ^= 1;
        }
      else
        {
          mpn_sub_n (prodp + hsize, vp, vp + hsize, hsize);
          /* No change of NEGFLG.  */
        }
      /* Read temporary operands from low part of PROD.
         Put result in low part of TSPACE using upper part of TSPACE
         as new TSPACE.  */
      MPN_MUL_N_RECURSE (tspace, prodp, prodp + hsize, hsize, tspace + size);

      /*** Add/copy product H.  */
      MPN_COPY (prodp + hsize, prodp + size, hsize);
      cy = mpn_add_n (prodp + size, prodp + size, prodp + size + hsize, hsize);

      /*** Add product M (if NEGFLG M is a negative number).  */
      if (negflg)
        cy -= mpn_sub_n (prodp + hsize, prodp + hsize, tspace, size);
      else
        cy += mpn_add_n (prodp + hsize, prodp + hsize, tspace, size);

      /*** Product L.    ________________  ________________
                        |________________||____U0 x V0_____|  */
      /* Read temporary operands from low part of PROD.
         Put result in low part of TSPACE using upper part of TSPACE
         as new TSPACE.  */
      MPN_MUL_N_RECURSE (tspace, up, vp, hsize, tspace + size);

      /*** Add/copy Product L (twice).  */

      cy += mpn_add_n (prodp + hsize, prodp + hsize, tspace, size);
      if (cy)
        mpn_add_1 (prodp + hsize + size, prodp + hsize + size, hsize, cy);

      MPN_COPY (prodp, tspace, hsize);
      cy = mpn_add_n (prodp + hsize, prodp + hsize, tspace + hsize, hsize);
      if (cy)
        mpn_add_1 (prodp + size, prodp + size, size, 1);
    }
}

/* recursive */
#undef __mpn_mul
#define __mpn_mul vstr__fmt_dbl_mpn_mul
#undef mpn_mul
#define mpn_mul vstr__fmt_dbl_mpn_mul

/* Multiply the natural numbers u (pointed to by UP, with USIZE limbs)
   and v (pointed to by VP, with VSIZE limbs), and store the result at
   PRODP.  USIZE + VSIZE limbs are always stored, but if the input
   operands are normalized.  Return the most significant limb of the
   result.

   NOTE: The space pointed to by PRODP is overwritten before finished
   with U and V, so overlap is an error.

   Argument constraints:
   1. USIZE >= VSIZE.
   2. PRODP != UP and PRODP != VP, i.e. the destination
      must be distinct from the multiplier and the multiplicand.  */

static mp_limb_t
vstr__fmt_dbl_mpn_mul (mp_ptr prodp,
                       mp_srcptr up, mp_size_t usize,
                       mp_srcptr vp, mp_size_t vsize)
{
  mp_ptr prod_endp = prodp + usize + vsize - 1;
  mp_limb_t cy;
  mp_ptr tspace;
  TMP_DECL (marker);

  if (vsize < KARATSUBA_THRESHOLD)
    {
      /* Handle simple cases with traditional multiplication.

         This is the most critical code of the entire function.  All
         multiplies rely on this, both small and huge.  Small ones arrive
         here immediately.  Huge ones arrive here as this is the base case
         for Karatsuba's recursive algorithm below.  */
      mp_size_t i;
      mp_limb_t cy_limb;
      mp_limb_t v_limb;

      if (vsize == 0)
        return 0;

      /* Multiply by the first limb in V separately, as the result can be
         stored (not added) to PROD.  We also avoid a loop for zeroing.  */
      v_limb = vp[0];
      if (v_limb <= 1)
        {
          if (v_limb == 1)
            MPN_COPY (prodp, up, usize);
          else
            MPN_ZERO (prodp, usize);
          cy_limb = 0;
        }
      else
        cy_limb = mpn_mul_1 (prodp, up, usize, v_limb);

      prodp[usize] = cy_limb;
      prodp++;

      /* For each iteration in the outer loop, multiply one limb from
         U with one limb from V, and add it to PROD.  */
      for (i = 1; i < vsize; i++)
        {
          v_limb = vp[i];
          if (v_limb <= 1)
            {
              cy_limb = 0;
              if (v_limb == 1)
                cy_limb = mpn_add_n (prodp, prodp, up, usize);
            }
          else
            cy_limb = mpn_addmul_1 (prodp, up, usize, v_limb);

          prodp[usize] = cy_limb;
          prodp++;
        }
      return cy_limb;
    }

  TMP_MARK (marker);

  tspace = (mp_ptr) TMP_ALLOC (2 * vsize * BYTES_PER_MP_LIMB);
  MPN_MUL_N_RECURSE (prodp, up, vp, vsize, tspace);

  prodp += vsize;
  up += vsize;
  usize -= vsize;
  if (usize >= vsize)
    {
      mp_ptr tp = (mp_ptr) TMP_ALLOC (2 * vsize * BYTES_PER_MP_LIMB);
      do
        {
          MPN_MUL_N_RECURSE (tp, up, vp, vsize, tspace);
          cy = mpn_add_n (prodp, prodp, tp, vsize);
          mpn_add_1 (prodp + vsize, tp + vsize, vsize, cy);
          prodp += vsize;
          up += vsize;
          usize -= vsize;
        }
      while (usize >= vsize);
    }

  /* True: usize < vsize.  */

  /* Make life simple: Recurse.  */

  if (usize != 0)
    {
      mpn_mul (tspace, vp, vsize, up, usize);
      cy = mpn_add_n (prodp, prodp, tspace, vsize);
      mpn_add_1 (prodp + vsize, tspace + vsize, usize, cy);
    }

  TMP_FREE (marker);
  return *prod_endp;
}

/* Shift U (pointed to by UP and USIZE digits long) CNT bits to the left
   and store the USIZE least significant digits of the result at WP.
   Return the bits shifted out from the most significant digit.

   Argument constraints:
   1. 0 < CNT < BITS_PER_MP_LIMB
   2. If the result is to be written over the input, WP must be >= UP.
*/

static mp_limb_t
vstr__fmt_dbl_mpn_lshift (register mp_ptr wp,
                          register mp_srcptr up, mp_size_t usize,
                          register unsigned int cnt)
{
  register mp_limb_t high_limb, low_limb;
  register unsigned sh_1, sh_2;
  register mp_size_t i;
  mp_limb_t retval;

#ifndef NDEBUG
  if (usize == 0 || cnt == 0)
    abort ();
#endif

  sh_1 = cnt;
#if 0
  if (sh_1 == 0)
    {
      if (wp != up)
        {
          /* Copy from high end to low end, to allow specified input/output
             overlapping.  */
          for (i = usize - 1; i >= 0; i--)
            wp[i] = up[i];
        }
      return 0;
    }
#endif

  wp += 1;
  sh_2 = BITS_PER_MP_LIMB - sh_1;
  i = usize - 1;
  low_limb = up[i];
  retval = low_limb >> sh_2;
  high_limb = low_limb;
  while (--i >= 0)
    {
      low_limb = up[i];
      wp[i] = (high_limb << sh_1) | (low_limb >> sh_2);
      high_limb = low_limb;
    }
  wp[i] = high_limb << sh_1;

  return retval;
}

#undef __mpn_lshift
#define __mpn_lshift vstr__fmt_dbl_mpn_lshift

/* Shift U (pointed to by UP and USIZE limbs long) CNT bits to the right
   and store the USIZE least significant limbs of the result at WP.
   The bits shifted out to the right are returned.

   Argument constraints:
   1. 0 < CNT < BITS_PER_MP_LIMB
   2. If the result is to be written over the input, WP must be <= UP.
*/

static mp_limb_t
vstr__fmt_dbl_mpn_rshift (register mp_ptr wp,
                          register mp_srcptr up, mp_size_t usize,
                          register unsigned int cnt)
{
  register mp_limb_t high_limb, low_limb;
  register unsigned sh_1, sh_2;
  register mp_size_t i;
  mp_limb_t retval;

#ifndef NDEBUG
  if (usize == 0 || cnt == 0)
    abort ();
#endif

  sh_1 = cnt;

#if 0
  if (sh_1 == 0)
    {
      if (wp != up)
        {
          /* Copy from low end to high end, to allow specified input/output
             overlapping.  */
          for (i = 0; i < usize; i++)
            wp[i] = up[i];
        }
      return usize;
    }
#endif

  wp -= 1;
  sh_2 = BITS_PER_MP_LIMB - sh_1;
  high_limb = up[0];
  retval = high_limb << sh_2;
  low_limb = high_limb;

  for (i = 1; i < usize; i++)
    {
      high_limb = up[i];
      wp[i] = (low_limb >> sh_1) | (high_limb << sh_2);
      low_limb = high_limb;
    }
  wp[i] = low_limb >> sh_1;

  return retval;
}

#undef __mpn_rshift
#define __mpn_rshift vstr__fmt_dbl_mpn_rshift

#else /* USE_MPN_C_VERSION */

extern int
vstr__fmt_dbl_mpn_cmp (mp_srcptr op1_ptr, mp_srcptr op2_ptr, mp_size_t size);
#undef __mpn_cmp
#define __mpn_cmp vstr__fmt_dbl_mpn_cmp
#undef mpn_cmp
#define mpn_cmp vstr__fmt_dbl_mpn_cmp

extern mp_limb_t
vstr__fmt_dbl_mpn_add_1 (register mp_ptr res_ptr,
                         register mp_srcptr s1_ptr,
                         register mp_size_t s1_size,
                         register mp_limb_t s2_limb);
#undef mpn_add_1
#define mpn_add_1 vstr__fmt_dbl_mpn_add_1

extern mp_limb_t
vstr__fmt_dbl_mpn_add_n (mp_ptr res_ptr, mp_srcptr s1_ptr,
                         mp_srcptr s2_ptr, mp_size_t size);
#undef mpn_add_n
#define mpn_add_n vstr__fmt_dbl_mpn_add_n

extern mp_limb_t
vstr__fmt_dbl_mpn_sub_n (mp_ptr res_ptr, mp_srcptr s1_ptr,
                         mp_srcptr s2_ptr, mp_size_t size);
#undef mpn_sub_n
#define mpn_sub_n vstr__fmt_dbl_mpn_sub_n

extern mp_limb_t
vstr__fmt_dbl_mpn_addmul_1(mp_ptr res_ptr, mp_srcptr s1_ptr,
                           mp_size_t s1_size, mp_limb_t s2_limb);
#undef mpn_addmul_1
#define mpn_addmul_1 vstr__fmt_dbl_mpn_addmul_1

extern mp_limb_t
vstr__fmt_dbl_mpn_submul_1 (mp_ptr res_ptr, mp_srcptr s1_ptr,
                            mp_size_t s1_size, mp_limb_t s2_limb);
#undef mpn_submul_1
#define mpn_submul_1 vstr__fmt_dbl_mpn_submul_1

extern mp_limb_t
vstr__fmt_dbl_mpn_divrem (mp_ptr qp, mp_size_t qextra_limbs,
                          mp_ptr np, mp_size_t nsize,
                          mp_srcptr dp, mp_size_t dsize);
#undef mpn_divrem
#define mpn_divrem vstr__fmt_dbl_mpn_divrem
#undef mpn_divmod
#define mpn_divmod(qp,np,nsize,dp,dsize) mpn_divrem (qp,0,np,nsize,dp,dsize)

extern mp_limb_t
vstr__fmt_dbl_mpn_mul_1 (mp_ptr res_ptr, mp_srcptr s1_ptr,
                         mp_size_t s1_size, mp_limb_t s2_limb);
#undef __mpn_mul_1
#define __mpn_mul_1 vstr__fmt_dbl_mpn_mul_1
#undef mpn_mul_1
#define mpn_mul_1 vstr__fmt_dbl_mpn_mul_1

extern void
vstr__fmt_dbl_impn_mul_n_basecase (mp_ptr prodp, mp_srcptr up,
                                   mp_srcptr vp, mp_size_t size);
#undef impn_mul_n_basecase
#define impn_mul_n_basecase vstr__fmt_dbl_impn_mul_n_basecase

extern void
vstr__fmt_dbl_impn_mul_n (mp_ptr prodp,
                          mp_srcptr up, mp_srcptr vp, mp_size_t size,
                          mp_ptr tspace);
#undef impn_mul_n
#define impn_mul_n vstr__fmt_dbl_impn_mul_n

extern mp_limb_t
vstr__fmt_dbl_mpn_mul (mp_ptr prodp,
                       mp_srcptr up, mp_size_t usize,
                       mp_srcptr vp, mp_size_t vsize);
#undef __mpn_mul
#define __mpn_mul vstr__fmt_dbl_mpn_mul
#undef mpn_mul
#define mpn_mul vstr__fmt_dbl_mpn_mul

extern mp_limb_t
vstr__fmt_dbl_mpn_lshift (register mp_ptr wp,
                          register mp_srcptr up, mp_size_t usize,
                          register unsigned int cnt);
#undef __mpn_lshift
#define __mpn_lshift vstr__fmt_dbl_mpn_lshift

extern mp_limb_t
vstr__fmt_dbl_mpn_rshift (register mp_ptr wp,
                          register mp_srcptr up, mp_size_t usize,
                          register unsigned int cnt);
#undef __mpn_rshift
#define __mpn_rshift vstr__fmt_dbl_mpn_rshift

#endif /* USE_MPN_C_VERSION */

#undef _itoa
#define _itoa(a, b, c, d) vstr__fmt_dbl_itoa(a, b, c, d)
#undef _itowa
#define _itowa(a, b, c, d) vstr__fmt_dbl_itowa(a, b, c, d)

#undef inline                    


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


#define __wmemmove(x, y, z) memmove(x, y, (z) * sizeof (wchar_t))
/* internal use function ... we aren't crap like glibc */
#define INTUSE(x) x

/* Return the number of extra grouping characters that will be inserted
   into a number with INTDIG_MAX integer digits.  */
static unsigned int
vstr__fmt_dbl_guess_grouping (unsigned int intdig_max, const char *grouping)
{
  unsigned int groups;

  /* We treat all negative values like CHAR_MAX.  */

  if (*grouping == CHAR_MAX || *grouping <= 0)
    /* No grouping should be done.  */
    return 0;

  groups = 0;
  while (intdig_max > (unsigned int) *grouping)
    {
      ++groups;
      intdig_max -= *grouping++;

      if (*grouping == CHAR_MAX
#if CHAR_MIN < 0
          || *grouping < 0
#endif
          )
        /* No more grouping should be done.  */
        break;
      else if (*grouping == 0)
        {
          /* Same grouping repeats.  */
          groups += (intdig_max - 1) / grouping[-1];
          break;
        }
    }

  return groups;
}

#undef __guess_grouping
#define __guess_grouping vstr__fmt_dbl_guess_grouping
/* Group the INTDIG_NO integer digits of the number in [BUF,BUFEND).
   There is guaranteed enough space past BUFEND to extend it.
   Return the new end of buffer.  */

static wchar_t *
vstr__fmt_dbl_group_number (wchar_t *buf, wchar_t *bufend,
                            unsigned int intdig_no,
                            const char *grouping, wchar_t thousands_sep,
                            int ngroups)
{
  wchar_t *p;

  if (ngroups == 0)
    return bufend;

  /* Move the fractional part down.  */
  __wmemmove (buf + intdig_no + ngroups, buf + intdig_no,
              bufend - (buf + intdig_no));

  p = buf + intdig_no + ngroups - 1;
  do
    {
      unsigned int len = *grouping++;
      do
        *p-- = buf[--intdig_no];
      while (--len > 0);
      *p-- = thousands_sep;

      if (*grouping == CHAR_MAX
#if CHAR_MIN < 0
          || *grouping < 0
#endif
          )
        /* No more grouping should be done.  */
        break;
      else if (*grouping == 0)
        /* Same grouping repeats.  */
        --grouping;
    } while (intdig_no > (unsigned int) *grouping);

  /* Copy the remaining ungrouped digits.  */
  do
    *p-- = buf[--intdig_no];
  while (p > buf);

  return bufend + ngroups;
}

#undef group_number
#define group_number vstr__fmt_dbl_group_number

#define VSTR__FMT_DBL_GLIBC_LOC_DECIMAL_POINT(x) ((x)->base->conf->loc->decimal_point_str)
#define VSTR__FMT_DBL_GLIBC_LOC_MON_DECIMAL_POINT(x) ((x)->base->conf->loc->decimal_point_str)
#define VSTR__FMT_DBL_GLIBC_LOC_GROUPING(x) ((x)->base->conf->loc->grouping)
#define VSTR__FMT_DBL_GLIBC_LOC_MON_GROUPING(x) ((x)->base->conf->loc->grouping)
#define VSTR__FMT_DBL_GLIBC_LOC_THOUSANDS_SEP(x) ((x)->base->conf->loc->thousands_sep_str)
#define VSTR__FMT_DBL_GLIBC_LOC_MON_THOUSANDS_SEP(x) ((x)->base->conf->loc->thousands_sep_str)

#define _NL_CURRENT(ignore, db_loc) VSTR__FMT_DBL_GLIBC_LOC_ ## db_loc (fp)

#define _NL_CURRENT_WORD(x, y) L'.'

#define ABS(x) (x >= 0 ? x : -x)

#ifndef __NO_LONG_DOUBLE_MATH
# define LDBL_MAX_10_EXP_LOG    12 /* = floor(log_2(LDBL_MAX_10_EXP)) */
#else
# define LDBL_MAX_10_EXP_LOG    8 /* = floor(log_2(LDBL_MAX_10_EXP)) */
#endif

/* Table of powers of ten.  This is used by __printf_fp and by
   strtof/strtod/strtold.  */
struct mp_power
  {
    size_t arrayoff;            /* Offset in `__tens'.  */
    mp_size_t arraysize;        /* Size of the array.  */
    int p_expo;                 /* Exponent of the number 10^(2^i).  */
    int m_expo;                 /* Exponent of the number 10^-(2^i-1).  */
  };

/* Table of MP integer constants 10^(2^i), used for floating point <-> decimal.
   First page   : 32-bit limbs
   Second page  : 64-bit limbs
   Last page    : table of pointers
 */
#if BITS_PER_MP_LIMB == 32

/* Table with constants of 10^(2^i), i=0..12 for 32-bit limbs.  */

const mp_limb_t vstr__fmt_dbl_tens[] =
{
#define TENS_P0_IDX	0
#define TENS_P0_SIZE	3
  [TENS_P0_IDX] = 0x00000000, 0x00000000, 0x0000000a,

#define TENS_P1_IDX	(TENS_P0_IDX + TENS_P0_SIZE)
#define TENS_P1_SIZE	3
  [TENS_P1_IDX] = 0x00000000, 0x00000000, 0x00000064,

#define TENS_P2_IDX	(TENS_P1_IDX + TENS_P1_SIZE)
#define TENS_P2_SIZE	3
  [TENS_P2_IDX] = 0x00000000, 0x00000000, 0x00002710,

#define TENS_P3_IDX	(TENS_P2_IDX + TENS_P2_SIZE)
#define TENS_P3_SIZE	3
  [TENS_P3_IDX] = 0x00000000, 0x00000000, 0x05f5e100,

#define TENS_P4_IDX	(TENS_P3_IDX + TENS_P3_SIZE)
#define TENS_P4_SIZE	4
  [TENS_P4_IDX] = 0x00000000, 0x00000000, 0x6fc10000, 0x002386f2,

#define TENS_P5_IDX	(TENS_P4_IDX + TENS_P4_SIZE)
#define TENS_P5_SIZE	6
  [TENS_P5_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x85acef81, 0x2d6d415b,
  0x000004ee,

#define TENS_P6_IDX	(TENS_P5_IDX + TENS_P5_SIZE)
#define TENS_P6_SIZE	9
  [TENS_P6_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xbf6a1f01,
  0x6e38ed64, 0xdaa797ed, 0xe93ff9f4, 0x00184f03,

#define TENS_P7_IDX	(TENS_P6_IDX + TENS_P6_SIZE)
#define TENS_P7_SIZE	16
  [TENS_P7_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x2e953e01, 0x03df9909, 0x0f1538fd, 0x2374e42f, 0xd3cff5ec,
  0xc404dc08, 0xbccdb0da, 0xa6337f19, 0xe91f2603, 0x0000024e,

#define TENS_P8_IDX	(TENS_P7_IDX + TENS_P7_SIZE)
#define TENS_P8_SIZE	29
  [TENS_P8_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x982e7c01,
  0xbed3875b, 0xd8d99f72, 0x12152f87, 0x6bde50c6, 0xcf4a6e70, 0xd595d80f,
  0x26b2716e, 0xadc666b0, 0x1d153624, 0x3c42d35a, 0x63ff540e, 0xcc5573c0,
  0x65f9ef17, 0x55bc28f2, 0x80dcc7f7, 0xf46eeddc, 0x5fdcefce, 0x000553f7,

#ifndef __NO_LONG_DOUBLE_MATH
# define TENS_P9_IDX	(TENS_P8_IDX + TENS_P8_SIZE)
# define TENS_P9_SIZE	56
  [TENS_P9_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0xfc6cf801, 0x77f27267, 0x8f9546dc, 0x5d96976f, 0xb83a8a97,
  0xc31e1ad9, 0x46c40513, 0x94e65747, 0xc88976c1, 0x4475b579, 0x28f8733b,
  0xaa1da1bf, 0x703ed321, 0x1e25cfea, 0xb21a2f22, 0xbc51fb2e, 0x96e14f5d,
  0xbfa3edac, 0x329c57ae, 0xe7fc7153, 0xc3fc0695, 0x85a91924, 0xf95f635e,
  0xb2908ee0, 0x93abade4, 0x1366732a, 0x9449775c, 0x69be5b0e, 0x7343afac,
  0xb099bc81, 0x45a71d46, 0xa2699748, 0x8cb07303, 0x8a0b1f13, 0x8cab8a97,
  0xc1d238d9, 0x633415d4, 0x0000001c,

# define TENS_P10_IDX	(TENS_P9_IDX + TENS_P9_SIZE)
# define TENS_P10_SIZE	109
  [TENS_P10_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x2919f001,
  0xf55b2b72, 0x6e7c215b, 0x1ec29f86, 0x991c4e87, 0x15c51a88, 0x140ac535,
  0x4c7d1e1a, 0xcc2cd819, 0x0ed1440e, 0x896634ee, 0x7de16cfb, 0x1e43f61f,
  0x9fce837d, 0x231d2b9c, 0x233e55c7, 0x65dc60d7, 0xf451218b, 0x1c5cd134,
  0xc9635986, 0x922bbb9f, 0xa7e89431, 0x9f9f2a07, 0x62be695a, 0x8e1042c4,
  0x045b7a74, 0x1abe1de3, 0x8ad822a5, 0xba34c411, 0xd814b505, 0xbf3fdeb3,
  0x8fc51a16, 0xb1b896bc, 0xf56deeec, 0x31fb6bfd, 0xb6f4654b, 0x101a3616,
  0x6b7595fb, 0xdc1a47fe, 0x80d98089, 0x80bda5a5, 0x9a202882, 0x31eb0f66,
  0xfc8f1f90, 0x976a3310, 0xe26a7b7e, 0xdf68368a, 0x3ce3a0b8, 0x8e4262ce,
  0x75a351a2, 0x6cb0b6c9, 0x44597583, 0x31b5653f, 0xc356e38a, 0x35faaba6,
  0x0190fba0, 0x9fc4ed52, 0x88bc491b, 0x1640114a, 0x005b8041, 0xf4f3235e,
  0x1e8d4649, 0x36a8de06, 0x73c55349, 0xa7e6bd2a, 0xc1a6970c, 0x47187094,
  0xd2db49ef, 0x926c3f5b, 0xae6209d4, 0x2d433949, 0x34f4a3c6, 0xd4305d94,
  0xd9d61a05, 0x00000325,

# define TENS_P11_IDX	(TENS_P10_IDX + TENS_P10_SIZE)
# define TENS_P11_SIZE	215
  [TENS_P11_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x1333e001, 0xe3096865, 0xb27d4d3f, 0x49e28dcf, 0xec2e4721,
  0xee87e354, 0xb6067584, 0x368b8abb, 0xa5e5a191, 0x2ed56d55, 0xfd827773,
  0xea50d142, 0x51b78db2, 0x98342c9e, 0xc850dabc, 0x866ed6f1, 0x19342c12,
  0x92794987, 0xd2f869c2, 0x66912e4a, 0x71c7fd8f, 0x57a7842d, 0x235552eb,
  0xfb7fedcc, 0xf3861ce0, 0x38209ce1, 0x9713b449, 0x34c10134, 0x8c6c54de,
  0xa7a8289c, 0x2dbb6643, 0xe3cb64f3, 0x8074ff01, 0xe3892ee9, 0x10c17f94,
  0xa8f16f92, 0xa8281ed6, 0x967abbb3, 0x5a151440, 0x9952fbed, 0x13b41e44,
  0xafe609c3, 0xa2bca416, 0xf111821f, 0xfb1264b4, 0x91bac974, 0xd6c7d6ab,
  0x8e48ff35, 0x4419bd43, 0xc4a65665, 0x685e5510, 0x33554c36, 0xab498697,
  0x0dbd21fe, 0x3cfe491d, 0x982da466, 0xcbea4ca7, 0x9e110c7b, 0x79c56b8a,
  0x5fc5a047, 0x84d80e2e, 0x1aa9f444, 0x730f203c, 0x6a57b1ab, 0xd752f7a6,
  0x87a7dc62, 0x944545ff, 0x40660460, 0x77c1a42f, 0xc9ac375d, 0xe866d7ef,
  0x744695f0, 0x81428c85, 0xa1fc6b96, 0xd7917c7b, 0x7bf03c19, 0x5b33eb41,
  0x5715f791, 0x8f6cae5f, 0xdb0708fd, 0xb125ac8e, 0x785ce6b7, 0x56c6815b,
  0x6f46eadb, 0x4eeebeee, 0x195355d8, 0xa244de3c, 0x9d7389c0, 0x53761abd,
  0xcf99d019, 0xde9ec24b, 0x0d76ce39, 0x70beb181, 0x2e55ecee, 0xd5f86079,
  0xf56d9d4b, 0xfb8886fb, 0x13ef5a83, 0x408f43c5, 0x3f3389a4, 0xfad37943,
  0x58ccf45c, 0xf82df846, 0x415c7f3e, 0x2915e818, 0x8b3d5cf4, 0x6a445f27,
  0xf8dbb57a, 0xca8f0070, 0x8ad803ec, 0xb2e87c34, 0x038f9245, 0xbedd8a6c,
  0xc7c9dee0, 0x0eac7d56, 0x2ad3fa14, 0xe0de0840, 0xf775677c, 0xf1bd0ad5,
  0x92be221e, 0x87fa1fb9, 0xce9d04a4, 0xd2c36fa9, 0x3f6f7024, 0xb028af62,
  0x907855ee, 0xd83e49d6, 0x4efac5dc, 0xe7151aab, 0x77cd8c6b, 0x0a753b7d,
  0x0af908b4, 0x8c983623, 0xe50f3027, 0x94222771, 0x1d08e2d6, 0xf7e928e6,
  0xf2ee5ca6, 0x1b61b93c, 0x11eb962b, 0x9648b21c, 0xce2bcba1, 0x34f77154,
  0x7bbebe30, 0xe526a319, 0x8ce329ac, 0xde4a74d2, 0xb5dc53d5, 0x0009e8b3,

# define TENS_P12_IDX	(TENS_P11_IDX + TENS_P11_SIZE)
# define TENS_P12_SIZE	428
  [TENS_P12_IDX] = 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x2a67c001,
  0xd4724e8d, 0x8efe7ae7, 0xf89a1e90, 0xef084117, 0x54e05154, 0x13b1bb51,
  0x506be829, 0xfb29b172, 0xe599574e, 0xf0da6146, 0x806c0ed3, 0xb86ae5be,
  0x45155e93, 0xc0591cc2, 0x7e1e7c34, 0x7c4823da, 0x1d1f4cce, 0x9b8ba1e8,
  0xd6bfdf75, 0xe341be10, 0xc2dfae78, 0x016b67b2, 0x0f237f1a, 0x3dbeabcd,
  0xaf6a2574, 0xcab3e6d7, 0x142e0e80, 0x61959127, 0x2c234811, 0x87009701,
  0xcb4bf982, 0xf8169c84, 0x88052f8c, 0x68dde6d4, 0xbc131761, 0xff0b0905,
  0x54ab9c41, 0x7613b224, 0x1a1c304e, 0x3bfe167b, 0x441c2d47, 0x4f6cea9c,
  0x78f06181, 0xeb659fb8, 0x30c7ae41, 0x947e0d0e, 0xa1ebcad7, 0xd97d9556,
  0x2130504d, 0x1a8309cb, 0xf2acd507, 0x3f8ec72a, 0xfd82373a, 0x95a842bc,
  0x280f4d32, 0xf3618ac0, 0x811a4f04, 0x6dc3a5b4, 0xd3967a1b, 0x15b8c898,
  0xdcfe388f, 0x454eb2a0, 0x8738b909, 0x10c4e996, 0x2bd9cc11, 0x3297cd0c,
  0x655fec30, 0xae0725b1, 0xf4090ee8, 0x037d19ee, 0x398c6fed, 0x3b9af26b,
  0xc994a450, 0xb5341743, 0x75a697b2, 0xac50b9c1, 0x3ccb5b92, 0xffe06205,
  0xa8329761, 0xdfea5242, 0xeb83cadb, 0xe79dadf7, 0x3c20ee69, 0x1e0a6817,
  0x7021b97a, 0x743074fa, 0x176ca776, 0x77fb8af6, 0xeca19beb, 0x92baf1de,
  0xaf63b712, 0xde35c88b, 0xa4eb8f8c, 0xe137d5e9, 0x40b464a0, 0x87d1cde8,
  0x42923bbd, 0xcd8f62ff, 0x2e2690f3, 0x095edc16, 0x59c89f1b, 0x1fa8fd5d,
  0x5138753d, 0x390a2b29, 0x80152f18, 0x2dd8d925, 0xf984d83e, 0x7a872e74,
  0xc19e1faf, 0xed4d542d, 0xecf9b5d0, 0x9462ea75, 0xc53c0adf, 0x0caea134,
  0x37a2d439, 0xc8fa2e8a, 0x2181327e, 0x6e7bb827, 0x2d240820, 0x50be10e0,
  0x5893d4b8, 0xab312bb9, 0x1f2b2322, 0x440b3f25, 0xbf627ede, 0x72dac789,
  0xb608b895, 0x78787e2a, 0x86deb3f0, 0x6fee7aab, 0xbb9373f4, 0x27ecf57b,
  0xf7d8b57e, 0xfca26a9f, 0x3d04e8d2, 0xc9df13cb, 0x3172826a, 0xcd9e8d7c,
  0xa8fcd8e0, 0xb2c39497, 0x307641d9, 0x1cc939c1, 0x2608c4cf, 0xb6d1c7bf,
  0x3d326a7e, 0xeeaf19e6, 0x8e13e25f, 0xee63302b, 0x2dfe6d97, 0x25971d58,
  0xe41d3cc4, 0x0a80627c, 0xab8db59a, 0x9eea37c8, 0xe90afb77, 0x90ca19cf,
  0x9ee3352c, 0x3613c850, 0xfe78d682, 0x788f6e50, 0x5b060904, 0xb71bd1a4,
  0x3fecb534, 0xb32c450c, 0x20c33857, 0xa6e9cfda, 0x0239f4ce, 0x48497187,
  0xa19adb95, 0xb492ed8a, 0x95aca6a8, 0x4dcd6cd9, 0xcf1b2350, 0xfbe8b12a,
  0x1a67778c, 0x38eb3acc, 0xc32da383, 0xfb126ab1, 0xa03f40a8, 0xed5bf546,
  0xe9ce4724, 0x4c4a74fd, 0x73a130d8, 0xd9960e2d, 0xa2ebd6c1, 0x94ab6feb,
  0x6f233b7c, 0x49126080, 0x8e7b9a73, 0x4b8c9091, 0xd298f999, 0x35e836b5,
  0xa96ddeff, 0x96119b31, 0x6b0dd9bc, 0xc6cc3f8d, 0x282566fb, 0x72b882e7,
  0xd6769f3b, 0xa674343d, 0x00fc509b, 0xdcbf7789, 0xd6266a3f, 0xae9641fd,
  0x4e89541b, 0x11953407, 0x53400d03, 0x8e0dd75a, 0xe5b53345, 0x108f19ad,
  0x108b89bc, 0x41a4c954, 0xe03b2b63, 0x437b3d7f, 0x97aced8e, 0xcbd66670,
  0x2c5508c2, 0x650ebc69, 0x5c4f2ef0, 0x904ff6bf, 0x9985a2df, 0x9faddd9e,
  0x5ed8d239, 0x25585832, 0xe3e51cb9, 0x0ff4f1d4, 0x56c02d9a, 0x8c4ef804,
  0xc1a08a13, 0x13fd01c8, 0xe6d27671, 0xa7c234f4, 0x9d0176cc, 0xd0d73df2,
  0x4d8bfa89, 0x544f10cd, 0x2b17e0b2, 0xb70a5c7d, 0xfd86fe49, 0xdf373f41,
  0x214495bb, 0x84e857fd, 0x00d313d5, 0x0496fcbe, 0xa4ba4744, 0xe8cac982,
  0xaec29e6e, 0x87ec7038, 0x7000a519, 0xaeee333b, 0xff66e42c, 0x8afd6b25,
  0x03b4f63b, 0xbd7991dc, 0x5ab8d9c7, 0x2ed4684e, 0x48741a6c, 0xaf06940d,
  0x2fdc6349, 0xb03d7ecd, 0xe974996f, 0xac7867f9, 0x52ec8721, 0xbcdd9d4a,
  0x8edd2d00, 0x3557de06, 0x41c759f8, 0x3956d4b9, 0xa75409f2, 0x123cd8a1,
  0xb6100fab, 0x3e7b21e2, 0x2e8d623b, 0x92959da2, 0xbca35f77, 0x200c03a5,
  0x35fcb457, 0x1bb6c6e4, 0xf74eb928, 0x3d5d0b54, 0x87cc1d21, 0x4964046f,
  0x18ae4240, 0xd868b275, 0x8bd2b496, 0x1c5563f4, 0xc234d8f5, 0xf868e970,
  0xf9151fff, 0xae7be4a2, 0x271133ee, 0xbb0fd922, 0x25254932, 0xa60a9fc0,
  0x104bcd64, 0x30290145, 0x00000062
#endif	/* !__NO_LONG_DOUBLE_MATH */
};

#elif BITS_PER_MP_LIMB == 64

/* Table with constants of 10^(2^i), i=0..12 for 64-bit limbs.	*/

const mp_limb_t vstr__fmt_dbl_tens[] =
{
#define TENS_P0_IDX	0
#define TENS_P0_SIZE	2
  [TENS_P0_IDX] = 0x0000000000000000, 0x000000000000000a,

#define TENS_P1_IDX	(TENS_P0_IDX + TENS_P0_SIZE)
#define TENS_P1_SIZE	2
  [TENS_P1_IDX] = 0x0000000000000000, 0x0000000000000064,

#define TENS_P2_IDX	(TENS_P1_IDX + TENS_P1_SIZE)
#define TENS_P2_SIZE	2
  [TENS_P2_IDX] = 0x0000000000000000, 0x0000000000002710,

#define TENS_P3_IDX	(TENS_P2_IDX + TENS_P2_SIZE)
#define TENS_P3_SIZE	2
  [TENS_P3_IDX] = 0x0000000000000000, 0x0000000005f5e100,

#define TENS_P4_IDX	(TENS_P3_IDX + TENS_P3_SIZE)
#define TENS_P4_SIZE	2
  [TENS_P4_IDX] = 0x0000000000000000, 0x002386f26fc10000,

#define TENS_P5_IDX	(TENS_P4_IDX + TENS_P4_SIZE)
#define TENS_P5_SIZE	3
  [TENS_P5_IDX] = 0x0000000000000000, 0x85acef8100000000, 0x000004ee2d6d415b,

#define TENS_P6_IDX	(TENS_P5_IDX + TENS_P5_SIZE)
#define TENS_P6_SIZE	5
  [TENS_P6_IDX] = 0x0000000000000000, 0x0000000000000000, 0x6e38ed64bf6a1f01,
  0xe93ff9f4daa797ed, 0x0000000000184f03,

#define TENS_P7_IDX	(TENS_P6_IDX + TENS_P6_SIZE)
#define TENS_P7_SIZE	8
  [TENS_P7_IDX] = 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x03df99092e953e01, 0x2374e42f0f1538fd, 0xc404dc08d3cff5ec,
  0xa6337f19bccdb0da, 0x0000024ee91f2603,

#define TENS_P8_IDX	(TENS_P7_IDX + TENS_P7_SIZE)
#define TENS_P8_SIZE	15
  [TENS_P8_IDX] = 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0xbed3875b982e7c01,
  0x12152f87d8d99f72, 0xcf4a6e706bde50c6, 0x26b2716ed595d80f,
  0x1d153624adc666b0, 0x63ff540e3c42d35a, 0x65f9ef17cc5573c0,
  0x80dcc7f755bc28f2, 0x5fdcefcef46eeddc, 0x00000000000553f7,
#ifndef __NO_LONG_DOUBLE_MATH
# define TENS_P9_IDX	(TENS_P8_IDX + TENS_P8_SIZE)
# define TENS_P9_SIZE	28
  [TENS_P9_IDX] = 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x77f27267fc6cf801, 0x5d96976f8f9546dc, 0xc31e1ad9b83a8a97,
  0x94e6574746c40513, 0x4475b579c88976c1, 0xaa1da1bf28f8733b,
  0x1e25cfea703ed321, 0xbc51fb2eb21a2f22, 0xbfa3edac96e14f5d,
  0xe7fc7153329c57ae, 0x85a91924c3fc0695, 0xb2908ee0f95f635e,
  0x1366732a93abade4, 0x69be5b0e9449775c, 0xb099bc817343afac,
  0xa269974845a71d46, 0x8a0b1f138cb07303, 0xc1d238d98cab8a97,
  0x0000001c633415d4,

# define TENS_P10_IDX	(TENS_P9_IDX + TENS_P9_SIZE)
# define TENS_P10_SIZE	55
  [TENS_P10_IDX] = 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0xf55b2b722919f001,
  0x1ec29f866e7c215b, 0x15c51a88991c4e87, 0x4c7d1e1a140ac535,
  0x0ed1440ecc2cd819, 0x7de16cfb896634ee, 0x9fce837d1e43f61f,
  0x233e55c7231d2b9c, 0xf451218b65dc60d7, 0xc96359861c5cd134,
  0xa7e89431922bbb9f, 0x62be695a9f9f2a07, 0x045b7a748e1042c4,
  0x8ad822a51abe1de3, 0xd814b505ba34c411, 0x8fc51a16bf3fdeb3,
  0xf56deeecb1b896bc, 0xb6f4654b31fb6bfd, 0x6b7595fb101a3616,
  0x80d98089dc1a47fe, 0x9a20288280bda5a5, 0xfc8f1f9031eb0f66,
  0xe26a7b7e976a3310, 0x3ce3a0b8df68368a, 0x75a351a28e4262ce,
  0x445975836cb0b6c9, 0xc356e38a31b5653f, 0x0190fba035faaba6,
  0x88bc491b9fc4ed52, 0x005b80411640114a, 0x1e8d4649f4f3235e,
  0x73c5534936a8de06, 0xc1a6970ca7e6bd2a, 0xd2db49ef47187094,
  0xae6209d4926c3f5b, 0x34f4a3c62d433949, 0xd9d61a05d4305d94,
  0x0000000000000325,

# define TENS_P11_IDX	(TENS_P10_IDX + TENS_P10_SIZE)
# define TENS_P11_SIZE	108
  [TENS_P11_IDX] = 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0xe30968651333e001, 0x49e28dcfb27d4d3f, 0xee87e354ec2e4721,
  0x368b8abbb6067584, 0x2ed56d55a5e5a191, 0xea50d142fd827773,
  0x98342c9e51b78db2, 0x866ed6f1c850dabc, 0x9279498719342c12,
  0x66912e4ad2f869c2, 0x57a7842d71c7fd8f, 0xfb7fedcc235552eb,
  0x38209ce1f3861ce0, 0x34c101349713b449, 0xa7a8289c8c6c54de,
  0xe3cb64f32dbb6643, 0xe3892ee98074ff01, 0xa8f16f9210c17f94,
  0x967abbb3a8281ed6, 0x9952fbed5a151440, 0xafe609c313b41e44,
  0xf111821fa2bca416, 0x91bac974fb1264b4, 0x8e48ff35d6c7d6ab,
  0xc4a656654419bd43, 0x33554c36685e5510, 0x0dbd21feab498697,
  0x982da4663cfe491d, 0x9e110c7bcbea4ca7, 0x5fc5a04779c56b8a,
  0x1aa9f44484d80e2e, 0x6a57b1ab730f203c, 0x87a7dc62d752f7a6,
  0x40660460944545ff, 0xc9ac375d77c1a42f, 0x744695f0e866d7ef,
  0xa1fc6b9681428c85, 0x7bf03c19d7917c7b, 0x5715f7915b33eb41,
  0xdb0708fd8f6cae5f, 0x785ce6b7b125ac8e, 0x6f46eadb56c6815b,
  0x195355d84eeebeee, 0x9d7389c0a244de3c, 0xcf99d01953761abd,
  0x0d76ce39de9ec24b, 0x2e55ecee70beb181, 0xf56d9d4bd5f86079,
  0x13ef5a83fb8886fb, 0x3f3389a4408f43c5, 0x58ccf45cfad37943,
  0x415c7f3ef82df846, 0x8b3d5cf42915e818, 0xf8dbb57a6a445f27,
  0x8ad803ecca8f0070, 0x038f9245b2e87c34, 0xc7c9dee0bedd8a6c,
  0x2ad3fa140eac7d56, 0xf775677ce0de0840, 0x92be221ef1bd0ad5,
  0xce9d04a487fa1fb9, 0x3f6f7024d2c36fa9, 0x907855eeb028af62,
  0x4efac5dcd83e49d6, 0x77cd8c6be7151aab, 0x0af908b40a753b7d,
  0xe50f30278c983623, 0x1d08e2d694222771, 0xf2ee5ca6f7e928e6,
  0x11eb962b1b61b93c, 0xce2bcba19648b21c, 0x7bbebe3034f77154,
  0x8ce329ace526a319, 0xb5dc53d5de4a74d2, 0x000000000009e8b3,

# define TENS_P12_IDX	(TENS_P11_IDX + TENS_P11_SIZE)
# define TENS_P12_SIZE	214
  [TENS_P12_IDX] = 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
  0x0000000000000000, 0x0000000000000000, 0xd4724e8d2a67c001,
  0xf89a1e908efe7ae7, 0x54e05154ef084117, 0x506be82913b1bb51,
  0xe599574efb29b172, 0x806c0ed3f0da6146, 0x45155e93b86ae5be,
  0x7e1e7c34c0591cc2, 0x1d1f4cce7c4823da, 0xd6bfdf759b8ba1e8,
  0xc2dfae78e341be10, 0x0f237f1a016b67b2, 0xaf6a25743dbeabcd,
  0x142e0e80cab3e6d7, 0x2c23481161959127, 0xcb4bf98287009701,
  0x88052f8cf8169c84, 0xbc13176168dde6d4, 0x54ab9c41ff0b0905,
  0x1a1c304e7613b224, 0x441c2d473bfe167b, 0x78f061814f6cea9c,
  0x30c7ae41eb659fb8, 0xa1ebcad7947e0d0e, 0x2130504dd97d9556,
  0xf2acd5071a8309cb, 0xfd82373a3f8ec72a, 0x280f4d3295a842bc,
  0x811a4f04f3618ac0, 0xd3967a1b6dc3a5b4, 0xdcfe388f15b8c898,
  0x8738b909454eb2a0, 0x2bd9cc1110c4e996, 0x655fec303297cd0c,
  0xf4090ee8ae0725b1, 0x398c6fed037d19ee, 0xc994a4503b9af26b,
  0x75a697b2b5341743, 0x3ccb5b92ac50b9c1, 0xa8329761ffe06205,
  0xeb83cadbdfea5242, 0x3c20ee69e79dadf7, 0x7021b97a1e0a6817,
  0x176ca776743074fa, 0xeca19beb77fb8af6, 0xaf63b71292baf1de,
  0xa4eb8f8cde35c88b, 0x40b464a0e137d5e9, 0x42923bbd87d1cde8,
  0x2e2690f3cd8f62ff, 0x59c89f1b095edc16, 0x5138753d1fa8fd5d,
  0x80152f18390a2b29, 0xf984d83e2dd8d925, 0xc19e1faf7a872e74,
  0xecf9b5d0ed4d542d, 0xc53c0adf9462ea75, 0x37a2d4390caea134,
  0x2181327ec8fa2e8a, 0x2d2408206e7bb827, 0x5893d4b850be10e0,
  0x1f2b2322ab312bb9, 0xbf627ede440b3f25, 0xb608b89572dac789,
  0x86deb3f078787e2a, 0xbb9373f46fee7aab, 0xf7d8b57e27ecf57b,
  0x3d04e8d2fca26a9f, 0x3172826ac9df13cb, 0xa8fcd8e0cd9e8d7c,
  0x307641d9b2c39497, 0x2608c4cf1cc939c1, 0x3d326a7eb6d1c7bf,
  0x8e13e25feeaf19e6, 0x2dfe6d97ee63302b, 0xe41d3cc425971d58,
  0xab8db59a0a80627c, 0xe90afb779eea37c8, 0x9ee3352c90ca19cf,
  0xfe78d6823613c850, 0x5b060904788f6e50, 0x3fecb534b71bd1a4,
  0x20c33857b32c450c, 0x0239f4cea6e9cfda, 0xa19adb9548497187,
  0x95aca6a8b492ed8a, 0xcf1b23504dcd6cd9, 0x1a67778cfbe8b12a,
  0xc32da38338eb3acc, 0xa03f40a8fb126ab1, 0xe9ce4724ed5bf546,
  0x73a130d84c4a74fd, 0xa2ebd6c1d9960e2d, 0x6f233b7c94ab6feb,
  0x8e7b9a7349126080, 0xd298f9994b8c9091, 0xa96ddeff35e836b5,
  0x6b0dd9bc96119b31, 0x282566fbc6cc3f8d, 0xd6769f3b72b882e7,
  0x00fc509ba674343d, 0xd6266a3fdcbf7789, 0x4e89541bae9641fd,
  0x53400d0311953407, 0xe5b533458e0dd75a, 0x108b89bc108f19ad,
  0xe03b2b6341a4c954, 0x97aced8e437b3d7f, 0x2c5508c2cbd66670,
  0x5c4f2ef0650ebc69, 0x9985a2df904ff6bf, 0x5ed8d2399faddd9e,
  0xe3e51cb925585832, 0x56c02d9a0ff4f1d4, 0xc1a08a138c4ef804,
  0xe6d2767113fd01c8, 0x9d0176cca7c234f4, 0x4d8bfa89d0d73df2,
  0x2b17e0b2544f10cd, 0xfd86fe49b70a5c7d, 0x214495bbdf373f41,
  0x00d313d584e857fd, 0xa4ba47440496fcbe, 0xaec29e6ee8cac982,
  0x7000a51987ec7038, 0xff66e42caeee333b, 0x03b4f63b8afd6b25,
  0x5ab8d9c7bd7991dc, 0x48741a6c2ed4684e, 0x2fdc6349af06940d,
  0xe974996fb03d7ecd, 0x52ec8721ac7867f9, 0x8edd2d00bcdd9d4a,
  0x41c759f83557de06, 0xa75409f23956d4b9, 0xb6100fab123cd8a1,
  0x2e8d623b3e7b21e2, 0xbca35f7792959da2, 0x35fcb457200c03a5,
  0xf74eb9281bb6c6e4, 0x87cc1d213d5d0b54, 0x18ae42404964046f,
  0x8bd2b496d868b275, 0xc234d8f51c5563f4, 0xf9151ffff868e970,
  0x271133eeae7be4a2, 0x25254932bb0fd922, 0x104bcd64a60a9fc0,
  0x0000006230290145
#endif
};

#else
# error "mp_limb_t size " BITS_PER_MP_LIMB "not accounted for"
#endif

#undef __tens
#define __tens vstr__fmt_dbl_tens

/* Each of array variable above defines one mpn integer which is a power of 10.
   This table points to those variables, indexed by the exponent.  */

static const struct mp_power vstr__fmt_dbl_fpioconst_pow10[LDBL_MAX_10_EXP_LOG + 1] =
{
  { TENS_P0_IDX, TENS_P0_SIZE,          4,         0 },
  { TENS_P1_IDX, TENS_P1_SIZE,          7,         4 },
  { TENS_P2_IDX, TENS_P2_SIZE,          14,       10 },
  { TENS_P3_IDX, TENS_P3_SIZE,          27,       24 },
  { TENS_P4_IDX, TENS_P4_SIZE,          54,       50 },
  { TENS_P5_IDX, TENS_P5_SIZE,          107,     103 },
  { TENS_P6_IDX, TENS_P6_SIZE,          213,     210 },
  { TENS_P7_IDX, TENS_P7_SIZE,          426,     422 },
  { TENS_P8_IDX, TENS_P8_SIZE,          851,     848 },
#ifndef __NO_LONG_DOUBLE_MATH
  { TENS_P9_IDX, TENS_P9_SIZE,          1701,   1698 },
  { TENS_P10_IDX, TENS_P10_SIZE,        3402,   3399 },
  { TENS_P11_IDX, TENS_P11_SIZE,        6804,   6800 },
  { TENS_P12_IDX, TENS_P12_SIZE,        13607, 13604 }
#endif
};

#undef _fpioconst_pow10
#define _fpioconst_pow10 vstr__fmt_dbl_fpioconst_pow10

/* NOTE: not defined ... */
/* #if LAST_POW10 > _LAST_POW10
   # error "Need to expand 10^(2^i) table for i up to" LAST_POW10
   #endif */

  
/* The constants in the array `_fpioconst_pow10' have an offset.  */
#if BITS_PER_MP_LIMB == 32
# define _FPIO_CONST_OFFSET     2
#else
# define _FPIO_CONST_OFFSET     1
#endif


/* BEG: ==== printf_fp ==== */
static int
vstr__fmt_printf_fp (FILE *fp,
                     const struct vstr__fmt_printf_info *info,
                     const void *const *args)
{
  /* The floating-point value to output.  */
  union
    {
      double dbl;
      __long_double_t ldbl;
    }
  fpnum;

  /* Locale-dependent representation of decimal point.  */
  const char *decimal;
  wchar_t decimalwc;

  /* Locale-dependent thousands separator and grouping specification.  */
  const char *thousands_sep = NULL;
  wchar_t thousands_sepwc = 0;
  const char *grouping;

  /* "NaN" or "Inf" for the special cases.  */
  const char *special = NULL;
  const wchar_t *wspecial = NULL;

  /* We need just a few limbs for the input before shifting to the right
     position.  */
  mp_limb_t fp_input[(LDBL_MANT_DIG + BITS_PER_MP_LIMB - 1) / BITS_PER_MP_LIMB];  /* We need to shift the contents of fp_input by this amount of bits.  */
  int to_shift = 0;

  /* The fraction of the floting-point value in question  */
  MPN_VAR(frac);
  /* and the exponent.  */
  int exponent;
  /* Sign of the exponent.  */
  int expsign = 0;
  /* Sign of float number.  */
  int is_neg = 0;

  /* Scaling factor.  */
  MPN_VAR(scale);

  /* Temporary bignum value.  */
  MPN_VAR(tmp);

  /* Digit which is result of last hack_digit() call.  */
  wchar_t digit;

  /* The type of output format that will be used: 'e'/'E' or 'f'.  */
  int type;

  /* Counter for number of written characters.  */
  int done = 0;

  /* General helper (carry limb).  */
  mp_limb_t cy;

  /* Nonzero if this is output on a wide character stream.  */
  int wide = info->wide;

  auto wchar_t hack_digit (void);

  wchar_t hack_digit (void)
    {
      mp_limb_t hi;

      if (expsign != 0 && type == 'f' && exponent-- > 0)
        hi = 0;
      else if (scalesize == 0)
        {
          hi = frac[fracsize - 1];
          cy = __mpn_mul_1 (frac, frac, fracsize - 1, 10);
          frac[fracsize - 1] = cy;
        }
      else
        {
          if (fracsize < scalesize)
            hi = 0;
          else
            {
              hi = mpn_divmod (tmp, frac, fracsize, scale, scalesize);
              tmp[fracsize - scalesize] = hi;
              hi = tmp[0];

              fracsize = scalesize;
              while (fracsize != 0 && frac[fracsize - 1] == 0)
                --fracsize;
              if (fracsize == 0)
                {
                  /* We're not prepared for an mpn variable with zero
                     limbs.  */
                  fracsize = 1;
                  return L'0' + hi;
                }
            }

          cy = __mpn_mul_1 (frac, frac, fracsize, 10);
          if (cy != 0)
            frac[fracsize++] = cy;
        }

      return L'0' + hi;
    }


  /* Figure out the decimal point character.  */
  if (info->extra == 0)
    {
      decimal = _NL_CURRENT (LC_NUMERIC, DECIMAL_POINT);
      decimalwc = _NL_CURRENT_WORD (LC_NUMERIC, _NL_NUMERIC_DECIMAL_POINT_WC);
    }
  else
    {
      decimal = _NL_CURRENT (LC_MONETARY, MON_DECIMAL_POINT);
      if (*decimal == '\0')
        decimal = _NL_CURRENT (LC_NUMERIC, DECIMAL_POINT);
      decimalwc = _NL_CURRENT_WORD (LC_MONETARY,
                                    _NL_MONETARY_DECIMAL_POINT_WC);
      if (decimalwc == L'\0')
        decimalwc = _NL_CURRENT_WORD (LC_NUMERIC,
                                      _NL_NUMERIC_DECIMAL_POINT_WC);
    }
  /* The decimal point character must not be zero.  */
  assert (*decimal != '\0');
  assert (decimalwc != L'\0');

  if (info->group)
    {
      if (info->extra == 0)
        grouping = _NL_CURRENT (LC_NUMERIC, GROUPING);
      else
        grouping = _NL_CURRENT (LC_MONETARY, MON_GROUPING);

      if (*grouping <= 0 || *grouping == CHAR_MAX)
        grouping = NULL;
      else
        {
          /* Figure out the thousands separator character.  */
          if (wide)
            {
              if (info->extra == 0)
                thousands_sepwc =
                  _NL_CURRENT_WORD (LC_NUMERIC, _NL_NUMERIC_THOUSANDS_SEP_WC);
              else
                thousands_sepwc =
                  _NL_CURRENT_WORD (LC_MONETARY,
                                    _NL_MONETARY_THOUSANDS_SEP_WC);
            }
          else
            {
              if (info->extra == 0)
                thousands_sep = _NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP);
              else
                thousands_sep = _NL_CURRENT (LC_MONETARY, MON_THOUSANDS_SEP);
            }

          if ((wide && thousands_sepwc == L'\0')
              || (! wide && *thousands_sep == '\0'))
            grouping = NULL;
          else if (thousands_sepwc == L'\0')
            /* If we are printing multibyte characters and there is a
               multibyte representation for the thousands separator,
               we must ensure the wide character thousands separator
               is available, even if it is fake.  */
            thousands_sepwc = 0xfffffffe;
        }
    }
  else
    grouping = NULL;

  /* Fetch the argument value.  */
#ifndef __NO_LONG_DOUBLE_MATH
  if (info->is_long_double && sizeof (long double) > sizeof (double))
    {
      fpnum.ldbl = *(const long double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (INTUSE(__isnanl) (fpnum.ldbl))
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
          is_neg = 0;
        }
      else if (INTUSE(__isinfl) (fpnum.ldbl))
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
          is_neg = fpnum.ldbl < 0;
        }
      else
        {
          fracsize = __mpn_extract_long_double (fp_input,
                                                (sizeof (fp_input) /
                                                 sizeof (fp_input[0])),
                                                &exponent, &is_neg,
                                                fpnum.ldbl);
          to_shift = 1 + fracsize * BITS_PER_MP_LIMB - LDBL_MANT_DIG;
        }
    }
  else
#endif  /* no long double */
    {
      fpnum.dbl = *(const double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (INTUSE(__isnan) (fpnum.dbl))
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
          is_neg = 0;
        }
      else if (INTUSE(__isinf) (fpnum.dbl))
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
          is_neg = fpnum.dbl < 0;
        }
      else
        {
          fracsize = __mpn_extract_double (fp_input,
                                           (sizeof (fp_input)
                                            / sizeof (fp_input[0])),
                                           &exponent, &is_neg, fpnum.dbl);
          to_shift = 1 + fracsize * BITS_PER_MP_LIMB - DBL_MANT_DIG;
        }
    }

  if (special)
    {
      int width = info->width;

      if (is_neg || info->showsign || info->space)
        --width;
      width -= 3;

      if (!info->left && width > 0)
        PADN (' ', width);

      if (is_neg)
        outchar ('-');
      else if (info->showsign)
        outchar ('+');
      else if (info->space)
        outchar (' ');

      PRINT (special, wspecial, 3);

      if (info->left && width > 0)
        PADN (' ', width);

      return done;
    }


  /* We need three multiprecision variables.  Now that we have the exponent
     of the number we can allocate the needed memory.  It would be more
     efficient to use variables of the fixed maximum size but because this
     would be really big it could lead to memory problems.  */
  {
    mp_size_t bignum_size = ((ABS (exponent) + BITS_PER_MP_LIMB - 1)
                             / BITS_PER_MP_LIMB + 4) * sizeof (mp_limb_t);
    frac = (mp_limb_t *) alloca (bignum_size);
    tmp = (mp_limb_t *) alloca (bignum_size);
    scale = (mp_limb_t *) alloca (bignum_size);
  }

  /* We now have to distinguish between numbers with positive and negative
     exponents because the method used for the one is not applicable/efficient
     for the other.  */
  scalesize = 0;
  if (exponent > 2)
    {
      /* |FP| >= 8.0.  */
      int scaleexpo = 0;
      int explog = LDBL_MAX_10_EXP_LOG;
      int vstr__fmt_dbl_exp10 = 0;
      const struct mp_power *powers = &_fpioconst_pow10[explog + 1];
      int cnt_h, cnt_l, i;

      if ((exponent + to_shift) % BITS_PER_MP_LIMB == 0)
        {
          MPN_COPY_DECR (frac + (exponent + to_shift) / BITS_PER_MP_LIMB,
                         fp_input, fracsize);
          fracsize += (exponent + to_shift) / BITS_PER_MP_LIMB;
        }
      else
        {
          cy = __mpn_lshift (frac + (exponent + to_shift) / BITS_PER_MP_LIMB,
                             fp_input, fracsize,
                             (exponent + to_shift) % BITS_PER_MP_LIMB);
          fracsize += (exponent + to_shift) / BITS_PER_MP_LIMB;
          if (cy)
            frac[fracsize++] = cy;
        }
      MPN_ZERO (frac, (exponent + to_shift) / BITS_PER_MP_LIMB);

      assert (powers > &_fpioconst_pow10[0]);
      do
        {
          --powers;

          /* The number of the product of two binary numbers with n and m
             bits respectively has m+n or m+n-1 bits.   */
          if (exponent >= scaleexpo + powers->p_expo - 1)
            {
              if (scalesize == 0)
                {
#ifndef __NO_LONG_DOUBLE_MATH
                  if (LDBL_MANT_DIG > _FPIO_CONST_OFFSET * BITS_PER_MP_LIMB
                      && info->is_long_double)
                    {
#define _FPIO_CONST_SHIFT \
  (((LDBL_MANT_DIG + BITS_PER_MP_LIMB - 1) / BITS_PER_MP_LIMB) \
   - _FPIO_CONST_OFFSET)
                      /* 64bit const offset is not enough for
                         IEEE quad long double.  */
                      tmpsize = powers->arraysize + _FPIO_CONST_SHIFT;
                      memcpy (tmp + _FPIO_CONST_SHIFT,
                              &__tens[powers->arrayoff],
                              tmpsize * sizeof (mp_limb_t));
                      MPN_ZERO (tmp, _FPIO_CONST_SHIFT);
                      /* Adjust exponent, as scaleexpo will be this much
                         bigger too.  */
                      exponent += _FPIO_CONST_SHIFT * BITS_PER_MP_LIMB;
                    }
                  else
#endif
                    {
                      tmpsize = powers->arraysize;
                      memcpy (tmp, &__tens[powers->arrayoff],
                              tmpsize * sizeof (mp_limb_t));
                    }
                }
              else
                {
                  cy = __mpn_mul (tmp, scale, scalesize,
                                  &__tens[powers->arrayoff
                                         + _FPIO_CONST_OFFSET],
                                  powers->arraysize - _FPIO_CONST_OFFSET);
                  tmpsize = scalesize + powers->arraysize - _FPIO_CONST_OFFSET;
                  if (cy == 0)
                    --tmpsize;
                }

              if (MPN_GE (frac, tmp))
                {
                  int cnt;
                  MPN_ASSIGN (scale, tmp);
                  count_leading_zeros (cnt, scale[scalesize - 1]);
                  scaleexpo = (scalesize - 2) * BITS_PER_MP_LIMB - cnt - 1;
                  vstr__fmt_dbl_exp10 |= 1 << explog;
                }
            }
          --explog;
        }
      while (powers > &_fpioconst_pow10[0]);
      exponent = vstr__fmt_dbl_exp10;

      /* Optimize number representations.  We want to represent the numbers
         with the lowest number of bytes possible without losing any
         bytes. Also the highest bit in the scaling factor has to be set
         (this is a requirement of the MPN division routines).  */
      if (scalesize > 0)
        {
          /* Determine minimum number of zero bits at the end of
             both numbers.  */
          for (i = 0; scale[i] == 0 && frac[i] == 0; i++)
            ;

          /* Determine number of bits the scaling factor is misplaced.  */
          count_leading_zeros (cnt_h, scale[scalesize - 1]);

          if (cnt_h == 0)
            {
              /* The highest bit of the scaling factor is already set.  So
                 we only have to remove the trailing empty limbs.  */
              if (i > 0)
                {
                  MPN_COPY_INCR (scale, scale + i, scalesize - i);
                  scalesize -= i;
                  MPN_COPY_INCR (frac, frac + i, fracsize - i);
                  fracsize -= i;
                }
            }
          else
            {
              if (scale[i] != 0)
                {
                  count_trailing_zeros (cnt_l, scale[i]);
                  if (frac[i] != 0)
                    {
                      int cnt_l2;
                      count_trailing_zeros (cnt_l2, frac[i]);
                      if (cnt_l2 < cnt_l)
                        cnt_l = cnt_l2;
                    }
                }
              else
                count_trailing_zeros (cnt_l, frac[i]);

              /* Now shift the numbers to their optimal position.  */
              if (i == 0 && BITS_PER_MP_LIMB - cnt_h > cnt_l)
                {
                  /* We cannot save any memory.  So just roll both numbers
                     so that the scaling factor has its highest bit set.  */

                  (void) __mpn_lshift (scale, scale, scalesize, cnt_h);
                  cy = __mpn_lshift (frac, frac, fracsize, cnt_h);
                  if (cy != 0)
                    frac[fracsize++] = cy;
                }
              else if (BITS_PER_MP_LIMB - cnt_h <= cnt_l)
                {
                  /* We can save memory by removing the trailing zero limbs
                     and by packing the non-zero limbs which gain another
                     free one. */

                  (void) __mpn_rshift (scale, scale + i, scalesize - i,
                                       BITS_PER_MP_LIMB - cnt_h);
                  scalesize -= i + 1;
                  (void) __mpn_rshift (frac, frac + i, fracsize - i,
                                       BITS_PER_MP_LIMB - cnt_h);
                  fracsize -= frac[fracsize - i - 1] == 0 ? i + 1 : i;
                }
              else
                {
                  /* We can only save the memory of the limbs which are zero.
                     The non-zero parts occupy the same number of limbs.  */

                  (void) __mpn_rshift (scale, scale + (i - 1),
                                       scalesize - (i - 1),
                                       BITS_PER_MP_LIMB - cnt_h);
                  scalesize -= i;
                  (void) __mpn_rshift (frac, frac + (i - 1),
                                       fracsize - (i - 1),
                                       BITS_PER_MP_LIMB - cnt_h);
                  fracsize -= frac[fracsize - (i - 1) - 1] == 0 ? i : i - 1;
                }
            }
        }
    }
  else if (exponent < 0)
    {
      /* |FP| < 1.0.  */
      int vstr__fmt_dbl_exp10 = 0;
      int explog = LDBL_MAX_10_EXP_LOG;
      const struct mp_power *powers = &_fpioconst_pow10[explog + 1];
      mp_size_t used_limbs = fracsize - 1;

      /* Now shift the input value to its right place.  */
      cy = __mpn_lshift (frac, fp_input, fracsize, to_shift);
      frac[fracsize++] = cy;
      assert (cy == 1 || (frac[fracsize - 2] == 0 && frac[0] == 0));

      expsign = 1;
      exponent = -exponent;

      assert (powers != &_fpioconst_pow10[0]);
      do
        {
          --powers;

          if (exponent >= powers->m_expo)
            {
              int i, incr, cnt_h, cnt_l;
              mp_limb_t topval[2];

              /* The __mpn_mul function expects the first argument to be
                 bigger than the second.  */
              if (fracsize < powers->arraysize - _FPIO_CONST_OFFSET)
                cy = __mpn_mul (tmp, &__tens[powers->arrayoff
                                            + _FPIO_CONST_OFFSET],
                                powers->arraysize - _FPIO_CONST_OFFSET,
                                frac, fracsize);
              else
                cy = __mpn_mul (tmp, frac, fracsize,
                                &__tens[powers->arrayoff + _FPIO_CONST_OFFSET],
                                powers->arraysize - _FPIO_CONST_OFFSET);
              tmpsize = fracsize + powers->arraysize - _FPIO_CONST_OFFSET;
              if (cy == 0)
                --tmpsize;

              count_leading_zeros (cnt_h, tmp[tmpsize - 1]);
              incr = (tmpsize - fracsize) * BITS_PER_MP_LIMB
                     + BITS_PER_MP_LIMB - 1 - cnt_h;

              assert (incr <= powers->p_expo);

              /* If we increased the exponent by exactly 3 we have to test
                 for overflow.  This is done by comparing with 10 shifted
                 to the right position.  */
              if (incr == exponent + 3)
                {
                  if (cnt_h <= BITS_PER_MP_LIMB - 4)
                    {
                      topval[0] = 0;
                      topval[1]
                        = ((mp_limb_t) 10) << (BITS_PER_MP_LIMB - 4 - cnt_h);
                    }
                  else
                    {
                      topval[0] = ((mp_limb_t) 10) << (BITS_PER_MP_LIMB - 4);
                      topval[1] = 0;
                      (void) __mpn_lshift (topval, topval, 2,
                                           BITS_PER_MP_LIMB - cnt_h);
                    }
                }

              /* We have to be careful when multiplying the last factor.
                 If the result is greater than 1.0 be have to test it
                 against 10.0.  If it is greater or equal to 10.0 the
                 multiplication was not valid.  This is because we cannot
                 determine the number of bits in the result in advance.  */
              if (incr < exponent + 3
                  || (incr == exponent + 3 &&
                      (tmp[tmpsize - 1] < topval[1]
                       || (tmp[tmpsize - 1] == topval[1]
                           && tmp[tmpsize - 2] < topval[0]))))
                {
                  /* The factor is right.  Adapt binary and decimal
                     exponents.  */
                  exponent -= incr;
                  vstr__fmt_dbl_exp10 |= 1 << explog;

                  /* If this factor yields a number greater or equal to
                     1.0, we must not shift the non-fractional digits down. */
                  if (exponent < 0)
                    cnt_h += -exponent;

                  /* Now we optimize the number representation.  */
                  for (i = 0; tmp[i] == 0; ++i);
                  if (cnt_h == BITS_PER_MP_LIMB - 1)
                    {
                      MPN_COPY (frac, tmp + i, tmpsize - i);
                      fracsize = tmpsize - i;
                    }
                  else
                    {
                      count_trailing_zeros (cnt_l, tmp[i]);

                      /* Now shift the numbers to their optimal position.  */
                      if (i == 0 && BITS_PER_MP_LIMB - 1 - cnt_h > cnt_l)
                        {
                          /* We cannot save any memory.  Just roll the
                             number so that the leading digit is in a
                             separate limb.  */

                          cy = __mpn_lshift (frac, tmp, tmpsize, cnt_h + 1);
                          fracsize = tmpsize + 1;
                          frac[fracsize - 1] = cy;
                        }
                      else if (BITS_PER_MP_LIMB - 1 - cnt_h <= cnt_l)
                        {
                          (void) __mpn_rshift (frac, tmp + i, tmpsize - i,
                                               BITS_PER_MP_LIMB - 1 - cnt_h);
                          fracsize = tmpsize - i;
                        }
                      else
                        {
                          /* We can only save the memory of the limbs which
                             are zero.  The non-zero parts occupy the same
                             number of limbs.  */

                          (void) __mpn_rshift (frac, tmp + (i - 1),
                                               tmpsize - (i - 1),
                                               BITS_PER_MP_LIMB - 1 - cnt_h);
                          fracsize = tmpsize - (i - 1);
                        }
                    }
                  used_limbs = fracsize - 1;
                }
            }
          --explog;
        }
      while (powers != &_fpioconst_pow10[1] && exponent > 0);
      /* All factors but 10^-1 are tested now.  */
      if (exponent > 0)
        {
          int cnt_l;

          cy = __mpn_mul_1 (tmp, frac, fracsize, 10);
          tmpsize = fracsize;
          assert (cy == 0 || tmp[tmpsize - 1] < 20);

          count_trailing_zeros (cnt_l, tmp[0]);
          if (cnt_l < MIN (4, exponent))
            {
              cy = __mpn_lshift (frac, tmp, tmpsize,
                                 BITS_PER_MP_LIMB - MIN (4, exponent));
              if (cy != 0)
                frac[tmpsize++] = cy;
            }
          else
            (void) __mpn_rshift (frac, tmp, tmpsize, MIN (4, exponent));
          fracsize = tmpsize;
          vstr__fmt_dbl_exp10 |= 1;
          assert (frac[fracsize - 1] < 10);
        }
      exponent = vstr__fmt_dbl_exp10;
    }
  else
    {
      /* This is a special case.  We don't need a factor because the
         numbers are in the range of 0.0 <= fp < 8.0.  We simply
         shift it to the right place and divide it by 1.0 to get the
         leading digit.  (Of course this division is not really made.)  */
      assert (0 <= exponent && exponent < 3 &&
              exponent + to_shift < BITS_PER_MP_LIMB);

      /* Now shift the input value to its right place.  */
      cy = __mpn_lshift (frac, fp_input, fracsize, (exponent + to_shift));
      frac[fracsize++] = cy;
      exponent = 0;
    }

  {
    int width = info->width;
    wchar_t *wbuffer, *wstartp, *wcp;
    int buffer_malloced;
    int chars_needed;
    int expscale;
    int intdig_max, intdig_no = 0;
    int fracdig_min, fracdig_max, fracdig_no = 0;
    int dig_max;
    int significant;
    int ngroups = 0;

    if (_tolower (info->spec) == 'e')
      {
        type = info->spec;
        intdig_max = 1;
        fracdig_min = fracdig_max = info->prec < 0 ? 6 : info->prec;
        chars_needed = 1 + 1 + fracdig_max + 1 + 1 + 4;
        /*             d   .     ddd         e   +-  ddd  */
        dig_max = INT_MAX;              /* Unlimited.  */
        significant = 1;                /* Does not matter here.  */
      }
    else if (_tolower (info->spec) == 'f')
      {
        type = 'f';
        fracdig_min = fracdig_max = info->prec < 0 ? 6 : info->prec;
        if (expsign == 0)
          {
            intdig_max = exponent + 1;
            /* This can be really big!  */  /* XXX Maybe malloc if too big? */
            chars_needed = exponent + 1 + 1 + fracdig_max;
          }
        else
          {
            intdig_max = 1;
            chars_needed = 1 + 1 + fracdig_max;
          }
        dig_max = INT_MAX;              /* Unlimited.  */
        significant = 1;                /* Does not matter here.  */
      }
    else
      {
        dig_max = info->prec < 0 ? 6 : (info->prec == 0 ? 1 : info->prec);
        if ((expsign == 0 && exponent >= dig_max)
            || (expsign != 0 && exponent > 4))
          {
            if ('g' - 'G' == 'e' - 'E')
              type = 'E' + (info->spec - 'G');
            else
              type = isupper (info->spec) ? 'E' : 'e';
            fracdig_max = dig_max - 1;
            intdig_max = 1;
            chars_needed = 1 + 1 + fracdig_max + 1 + 1 + 4;
          }
        else
          {
            type = 'f';
            intdig_max = expsign == 0 ? exponent + 1 : 0;
            fracdig_max = dig_max - intdig_max;
            /* We need space for the significant digits and perhaps
               for leading zeros when < 1.0.  The number of leading
               zeros can be as many as would be required for
               exponential notation with a negative two-digit
               exponent, which is 4.  */
            chars_needed = dig_max + 1 + 4;
          }
        fracdig_min = info->alt ? fracdig_max : 0;
        significant = 0;                /* We count significant digits.  */
      }

    if (grouping)
      {
        /* Guess the number of groups we will make, and thus how
           many spaces we need for separator characters.  */
        ngroups = __guess_grouping (intdig_max, grouping);
        chars_needed += ngroups;
      }

    /* Allocate buffer for output.  We need two more because while rounding
       it is possible that we need two more characters in front of all the
       other output.  If the amount of memory we have to allocate is too
       large use `malloc' instead of `alloca'.  */
    buffer_malloced = chars_needed > 5000;
    if (buffer_malloced)
      {
        wbuffer = (wchar_t *) malloc ((2 + chars_needed) * sizeof (wchar_t));
        if (wbuffer == NULL)
          /* Signal an error to the caller.  */
          return -1;
      }
    else
      wbuffer = (wchar_t *) alloca ((2 + chars_needed) * sizeof (wchar_t));
    wcp = wstartp = wbuffer + 2;        /* Let room for rounding.  */

    /* Do the real work: put digits in allocated buffer.  */
    if (expsign == 0 || type != 'f')
      {
        assert (expsign == 0 || intdig_max == 1);
        while (intdig_no < intdig_max)
          {
            ++intdig_no;
            *wcp++ = hack_digit ();
          }
        significant = 1;
        if (info->alt
            || fracdig_min > 0
            || (fracdig_max > 0 && (fracsize > 1 || frac[0] != 0)))
          *wcp++ = decimalwc;
      }
    else
      {
        /* |fp| < 1.0 and the selected type is 'f', so put "0."
           in the buffer.  */
        *wcp++ = L'0';
        --exponent;
        *wcp++ = decimalwc;
      }

    /* Generate the needed number of fractional digits.  */
    while (fracdig_no < fracdig_min
           || (fracdig_no < fracdig_max && (fracsize > 1 || frac[0] != 0)))
      {
        ++fracdig_no;
        *wcp = hack_digit ();
        if (*wcp != L'0')
          significant = 1;
        else if (significant == 0)
          {
            ++fracdig_max;
            if (fracdig_min > 0)
              ++fracdig_min;
          }
        ++wcp;
      }

    /* Do rounding.  */
    digit = hack_digit ();
    if (digit > L'4')
      {
        wchar_t *wtp = wcp;

        if (digit == L'5'
            && ((*(wcp - 1) != decimalwc && (*(wcp - 1) & 1) == 0)
                || ((*(wcp - 1) == decimalwc && (*(wcp - 2) & 1) == 0))))
          {
            /* This is the critical case.        */
            if (fracsize == 1 && frac[0] == 0)
              /* Rest of the number is zero -> round to even.
                 (IEEE 754-1985 4.1 says this is the default rounding.)  */
              goto do_expo;
            else if (scalesize == 0)
              {
                /* Here we have to see whether all limbs are zero since no
                   normalization happened.  */
                size_t lcnt = fracsize;
                while (lcnt >= 1 && frac[lcnt - 1] == 0)
                  --lcnt;
                if (lcnt == 0)
                  /* Rest of the number is zero -> round to even.
                     (IEEE 754-1985 4.1 says this is the default rounding.)  */
                  goto do_expo;
              }
          }

        if (fracdig_no > 0)
          {
            /* Process fractional digits.  Terminate if not rounded or
               radix character is reached.  */
            while (*--wtp != decimalwc && *wtp == L'9')
              *wtp = '0';
            if (*wtp != decimalwc)
              /* Round up.  */
              (*wtp)++;
          }

        if (fracdig_no == 0 || *wtp == decimalwc)
          {
            /* Round the integer digits.  */
            if (*(wtp - 1) == decimalwc)
              --wtp;

            while (--wtp >= wstartp && *wtp == L'9')
              *wtp = L'0';

            if (wtp >= wstartp)
              /* Round up.  */
              (*wtp)++;
            else
              /* It is more critical.  All digits were 9's.  */
              {
                if (type != 'f')
                  {
                    *wstartp = '1';
                    exponent += expsign == 0 ? 1 : -1;
                  }
                else if (intdig_no == dig_max)
                  {
                    /* This is the case where for type %g the number fits
                       really in the range for %f output but after rounding
                       the number of digits is too big.  */
                    *--wstartp = decimalwc;
                    *--wstartp = L'1';

                    if (info->alt || fracdig_no > 0)
                      {
                        /* Overwrite the old radix character.  */
                        wstartp[intdig_no + 2] = L'0';
                        ++fracdig_no;
                      }

                    fracdig_no += intdig_no;
                    intdig_no = 1;
                    fracdig_max = intdig_max - intdig_no;
                    ++exponent;
                    /* Now we must print the exponent.  */
                    type = isupper (info->spec) ? 'E' : 'e';
                  }
                else
                  {
                    /* We can simply add another another digit before the
                       radix.  */
                    *--wstartp = L'1';
                    ++intdig_no;
                  }

                /* While rounding the number of digits can change.
                   If the number now exceeds the limits remove some
                   fractional digits.  */
                if (intdig_no + fracdig_no > dig_max)
                  {
                    wcp -= intdig_no + fracdig_no - dig_max;
                    fracdig_no -= intdig_no + fracdig_no - dig_max;
                  }
              }
          }
      }

  do_expo:
    /* Now remove unnecessary '0' at the end of the string.  */
    while (fracdig_no > fracdig_min && *(wcp - 1) == L'0')
      {
        --wcp;
        --fracdig_no;
      }
    /* If we eliminate all fractional digits we perhaps also can remove
       the radix character.  */
    if (fracdig_no == 0 && !info->alt && *(wcp - 1) == decimalwc)
      --wcp;

    if (grouping)
      /* Add in separator characters, overwriting the same buffer.  */
      wcp = group_number (wstartp, wcp, intdig_no, grouping, thousands_sepwc,
                          ngroups);

    /* Write the exponent if it is needed.  */
    if (type != 'f')
      {
        *wcp++ = (wchar_t) type;
        *wcp++ = expsign ? L'-' : L'+';

        /* Find the magnitude of the exponent.  */
        expscale = 10;
        while (expscale <= exponent)
          expscale *= 10;

        if (exponent < 10)
          /* Exponent always has at least two digits.  */
          *wcp++ = L'0';
        else
          do
            {
              expscale /= 10;
              *wcp++ = L'0' + (exponent / expscale);
              exponent %= expscale;
            }
          while (expscale > 10);
        *wcp++ = L'0' + exponent;
      }

    /* Compute number of characters which must be filled with the padding
       character.  */
    if (is_neg || info->showsign || info->space)
      --width;
    width -= wcp - wstartp;

    if (!info->left && info->pad != '0' && width > 0)
      PADN (info->pad, width);

    if (is_neg)
      outchar ('-');
    else if (info->showsign)
      outchar ('+');
    else if (info->space)
      outchar (' ');

    if (!info->left && info->pad == '0' && width > 0)
      PADN ('0', width);

    {
      char *buffer = NULL;
      char *cp = NULL;
      char *tmpptr;

      if (! wide)
        {
          /* Create the single byte string.  */
          size_t decimal_len;
          size_t thousands_sep_len;
          wchar_t *copywc;

          decimal_len = strlen (decimal);

          if (thousands_sep == NULL)
            thousands_sep_len = 0;
          else
            thousands_sep_len = strlen (thousands_sep);

          if (buffer_malloced)
            {
              buffer = (char *) malloc (2 + chars_needed + decimal_len
                                        + ngroups * thousands_sep_len);
              if (buffer == NULL)
                /* Signal an error to the caller.  */
                return -1;
            }
          else
            buffer = (char *) alloca (2 + chars_needed + decimal_len
                                      + ngroups * thousands_sep_len);

          /* Now copy the wide character string.  Since the character
             (except for the decimal point and thousands separator) must
             be coming from the ASCII range we can esily convert the
             string without mapping tables.  */
          for (cp = buffer, copywc = wstartp; copywc < wcp; ++copywc)
            if (*copywc == decimalwc)
              cp = (char *) __mempcpy (cp, decimal, decimal_len);
            else if (*copywc == thousands_sepwc)
              cp = (char *) __mempcpy (cp, thousands_sep, thousands_sep_len);
            else
              *cp++ = (char) *copywc;
        }

      tmpptr = buffer;
      PRINT (tmpptr, wstartp, wide ? wcp - wstartp : cp - tmpptr);

      /* Free the memory if necessary.  */
      if (buffer_malloced)
        {
          free (buffer);
          free (wbuffer);
        }
    }

    if (info->left && width > 0)
      PADN (info->pad, width);
  }
  return done;
}
/* END: ==== printf_fp ==== */


/* BEG: ==== printf_fphex ==== */
#undef wide
#define wide 0 /* no wide output device -- should do for printf_fp too */
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

	  negative = signbitl (fpnum.ldbl.d);
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
      width = info->width;

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
  /* #ifdef PRINT_FPHEX_LONG_DOUBLE */
 else
    PRINT_FPHEX_LONG_DOUBLE;
  /* #endif */

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

#undef wide /* part of struct */
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

  if (spec->field_width_usr)
    info.width = spec->field_width;
  else
    info.width = 0;
  
  info.spec = spec->fmt_code;

  info.extra = 0;
  info.wide = 0;
  
  info.is_long_double = (spec->int_type == VSTR_TYPE_FMT_ULONG_LONG);
  info.alt =      !!(spec->flags & SPECIAL);
  info.space =    !!(spec->flags & SPACE);
  info.left =     !!(spec->flags & LEFT);
  info.showsign = !!(spec->flags & PLUS);
  info.group =    !!(spec->flags & THOUSAND_SEP);

  if (spec->flags & ZEROPAD)
    info.pad = '0';
  else
    info.pad = ' ';
  
  if (spec->int_type == VSTR_TYPE_FMT_ULONG_LONG)
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
    case 'f':
    case 'F':
    case 'g':
    case 'G':
      return (vstr__fmt_printf_fp(&wrap_fp, &info, &dbl_ptr));

    default:
      assert(FALSE);
  }
  
  return (FALSE);
}

#undef FILE

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
#undef signbitl
 
#undef _itoa_word
#undef _itowa_word


