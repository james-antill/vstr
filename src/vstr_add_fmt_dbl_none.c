/*
 *  Copyright (C) 2002  James Antill
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
/* This code does the %F, %f, %G, %g, %A, %a, %E, %e from *printf by always
 * treating the float as a zero, useful if you never need to do fp stuff */
/* Note that this file is #include'd */

static int vstr__add_fmt_dbl(Vstr_base *base, size_t pos_diff,
                             struct Vstr__fmt_spec *spec)
{
  unsigned int size = spec->field_width;
  unsigned int precision = spec->precision;
  char sign = 0;
  int big = FALSE;
  
  if (spec->flags & LEFT)
    spec->flags &= ~ZEROPAD;
  
  if (spec->flags & SIGN)
  {
    if (spec->flags & IS_NEGATIVE)
      sign = '-';
    else
    {
      if (spec->flags & PLUS)
        sign = '+';
      else
        if (spec->flags & SPACE)
          sign = ' ';
        else
          ++size;
    }
    
    if (size) --size;
  }

  if (!spec->field_width_usr)
    assert(!size);

  if (size) --size; /* size of number */

  if (size > precision)
    size -= precision;
  else
    size = 0;

  if ((spec->flags & SPECIAL) || precision)
    if (size) --size; /* "." */

  switch (spec->fmt_code)
  {
    case 'a':
    case 'A': /* "0x" ... "p+0"  == 5 extra */
      if (size) --size;
    case 'e':
    case 'E': /* "e+00" == 4 extra */
      if (size) --size;
      if (size) --size;
      if (size) --size;
      if (size) --size;
    case 'f':
    case 'F':
    case 'g':
    case 'G':
      break;

    default:
      assert(FALSE);
  }
  
  if (!(spec->flags & (ZEROPAD | LEFT)))
    if (size > 0)
    { /* right justify number, with spaces -- zeros done after sign/spacials */
      if (!vstr__add_spaces(base, pos_diff, size))
        goto failed_alloc;
      size = 0;
    }
  
  if (sign)
    if (!VSTR__FMT_ADD(base, &sign, 1))
      goto failed_alloc;
  
  if (spec->fmt_code == 'a')
    if (!VSTR__FMT_ADD(base, "0x", 2))
      goto failed_alloc;
  if (spec->fmt_code == 'A')
    if (!VSTR__FMT_ADD(base, "0X", 2))
      goto failed_alloc;
  
  if (!(spec->flags & LEFT))
    if (size > 0)
    {
      assert(spec->flags & ZEROPAD);
      if (!vstr__add_zeros(base, pos_diff, size))
        goto failed_alloc;
      size = 0;
    }
  
  if (!vstr__add_zeros(base, pos_diff, 1)) /* number */
    goto failed_alloc;

  if ((spec->flags & SPECIAL) || precision)
    if (!VSTR__FMT_ADD(base, ".", 1))
      goto failed_alloc;
  
  if (precision && !vstr__add_zeros(base, pos_diff, precision))
    goto failed_alloc;
      
  switch (spec->fmt_code)
  {
    case 'e':
    case 'E':
      if (!VSTR__FMT_ADD(base, &spec->fmt_code, 1))
        goto failed_alloc;

      if (!VSTR__FMT_ADD(base, "+", 1))
        goto failed_alloc;

      if (!vstr__add_zeros(base, pos_diff, 2))
        goto failed_alloc;
      break;
      
    case 'F':
    case 'G':
      big = TRUE;
    case 'f':
    case 'g': /* always use 0.0, so %f is always smaller for %g */
        break;
        
    case 'A':
      big = TRUE;
    case 'a':
      if (!VSTR__FMT_ADD(base, big ? "P" : "p", 1))
        goto failed_alloc;

      if (!VSTR__FMT_ADD(base, "+", 1))
        goto failed_alloc;

      if (!vstr__add_zeros(base, pos_diff, 1))
        goto failed_alloc;
      break;
      
    default:
      assert(FALSE);
  }

  if (size > 0)
  {
    assert(spec->flags & LEFT);
    if (!vstr__add_spaces(base, pos_diff, size))
      goto failed_alloc;
    size = 0;
  }  

  return (TRUE);

 failed_alloc:
  return (FALSE);
}
