#define VSTR_MOV_C
/*
 *  Copyright (C) 2001  James Antill
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

int vstr_mov(Vstr_base *base, size_t pos,
             Vstr_base *from_base, size_t from_pos, size_t len)
{ /* FIXME: move data when you can, should require at most 2 splits
   * possibly none */
  int ret = vstr_add_vstr(base, pos,
                          from_base, from_pos, len, VSTR_TYPE_ADD_DEF);
  if (!ret)
    return (FALSE);

  ret = vstr_del(from_base, from_pos, len);
  if (!ret)
  {
    ret = vstr_del(base, pos, len);
    assert(ret); /* this must work as a split can't happen */
    return (FALSE);
  }
  
  return (ret);
}
