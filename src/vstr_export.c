#define VSTR_EXPORT_C
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
/* functions for exporting a vstr to other types */
#include "main.h"


size_t vstr_export_iovec_ptr_all(const Vstr_base *base,
                                 struct iovec **iovs, unsigned int *ret_num)
{ 
 if (!base->num || !iovs || !ret_num)
   return (0);

 if (!vstr__base_iovec_valid((Vstr_base *)base))
   return (0);

 *iovs = base->vec.v + base->vec.off;

 *ret_num = base->num;
 
 return (base->len);
}

size_t vstr_export_buf(const Vstr_base *base, size_t pos, size_t len, void *buf)
{
 Vstr_node *scan = NULL;
 unsigned int num = 0;
 size_t ret = 0;
 char *scan_str = NULL;
 size_t scan_len = 0;

 scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
 if (!scan)
   return (0);

 ret = len + scan_len;
 
 do
 {
  if (scan->type != VSTR_TYPE_NODE_NON)
    memcpy(buf, scan_str, scan_len);
   
  buf = ((char *)buf) + scan_len;
 } while ((scan = vstr__base_scan_fwd_nxt(base, &len, &num,
                                          scan, &scan_str, &scan_len)));
 
 return (ret);
}

char vstr_export_chr(const Vstr_base *base, size_t pos)
{
 char buf[1] = {0};

 /* errors, requests for data from NON nodes and real data are all == 0 */
 vstr_export_buf(base, pos, 1, buf);
 
 return (buf[0]);
}

