#define VSTR_CONV_C
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
/* functions for converting data in vstrs */
#include "main.h"

int vstr_conv_lowercase(Vstr_base *base, size_t pos, size_t len)
{ /* FIXME: not atomic */
  while (len)
  {
    char tmp = vstr_export_chr(base, pos);
    
    if (VSTR__IS_ASCII_UPPER(tmp))
    {
      tmp = VSTR__TO_ASCII_LOWER(tmp);
      
      if (!vstr_sub_buf(base, pos, 1, &tmp, 1))
        return (FALSE);
    }
        
    ++pos;
    --len;
  }

  return (TRUE);
}

int vstr_conv_uppercase(Vstr_base *base, size_t pos, size_t len)
{ /* FIXME: not atomic */
  while (len)
  {
    char tmp = vstr_export_chr(base, pos);
    
    if (VSTR__IS_ASCII_LOWER(tmp))
    {
      tmp = VSTR__TO_ASCII_UPPER(tmp);
      
      if (!vstr_sub_buf(base, pos, 1, &tmp, 1))
        return (FALSE);
    }
        
    ++pos;
    --len;
  }

  return (TRUE);
}

int vstr_conv_unprintable(Vstr_base *base, size_t pos, size_t len,
                          unsigned int flags, char swp)
{ /* FIXME: not atomic */
  while (len)
  {
    unsigned char tmp = vstr_export_chr(base, pos);

    if (!tmp)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_NUL) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0x7)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BEL) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0x8)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_BS) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0x9)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HT) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0xA)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_LF) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0xB)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_VT) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0xC)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_FF) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0xD)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_CR) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp == 0x7F)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_DEL) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (tmp > 0xA1)
    {
      if (!(flags & VSTR_FLAG_CONV_UNPRINTABLE_ALLOW_HIGH) &&
          !vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
    else if (!((tmp >= 0x20) && (tmp <= 0x7E)))
    {
      if (!vstr_sub_buf(base, pos, 1, &swp, 1))
        return (FALSE);
    }
        
    ++pos;
    --len;
  }

  return (TRUE);
}
