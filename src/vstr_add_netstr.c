#define VSTR_ADD_NETSTR_C
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
/* netstr (http://cr.yp.to/proto/netstrings.txt). This is basically
 * <num ':' data ','>
 * where
 * num is an ascii number (base 10)
 * data is 8 bit binary data (Ie. any value 0 - 255 is allowed).
 */
/* netstr2 like netstr (http://cr.yp.to/proto/netstrings.txt)
 * but allows leading '0' characters */

#include "main.h"

size_t netstr2_num_len = 0;


static size_t vstr__netstr2_zero(char *str, size_t len, size_t *zeros)
{
 size_t tmp = *zeros;
 
 if (tmp > len)
   tmp = len;

 memset(str, '0', tmp);
 *zeros -= tmp;

 if (!*zeros && (tmp != len))
   return (tmp);

 return (0);
}

static size_t vstr__netstr2_copy(char *str, char *buf, size_t len, size_t count)
{
 size_t tmp = netstr2_num_len - count;
 
 if (tmp > len)
   tmp = len;
 
 memcpy(str, buf + count, tmp);
 count += tmp;
 
 return (count);
}

size_t vstr_add_netstr2_beg(Vstr_base *base)
{
 size_t tmp = 0;
 size_t ret = base->len + 1;
 
 tmp = vstr_add_fmt(base, base->len, "%lu:", ULONG_MAX);

 if (!tmp)
    return (0);

 --tmp; /* remove comma from len */
 
 assert(!netstr2_num_len || (tmp == netstr2_num_len));
 netstr2_num_len = tmp;

 return (ret);
}

static int vstr__netstr_end_start(Vstr_base *base,
                                  size_t netstr_pos,
                                  size_t *count, size_t *netstr_len, char *buf)
{
 if (!netstr2_num_len)
   return (FALSE);

 if (netstr_pos >= (base->len - netstr2_num_len + 1))
   return (FALSE);

 assert(vstr_export_chr(base, netstr_pos + netstr2_num_len) == ':');
 
 /* + 1 because of the ':' */
 *netstr_len = base->len - (netstr_pos + netstr2_num_len);

 if (TRUE)
 {
  size_t pos = netstr_pos + netstr2_num_len + *netstr_len;
  size_t len = 1;
  Vstr_node *scan = NULL;
  unsigned int num = 0;
  char *scan_str = NULL;
  size_t scan_len = 0;

  assert(pos <= base->len);
  
  scan = vstr__base_scan_fwd_beg(base, pos, &len, &num, &scan_str, &scan_len);
  assert(scan);

  if (scan->type == VSTR_TYPE_NODE_BUF)
  {
   if (!vstr_add_buf(base, base->len, ",", 1))
     return (FALSE);
  }
  else
  {
   if (!vstr_add_ptr(base, base->len, ",", 1))
     return (FALSE);
  }
 }
 
 *count = netstr2_num_len;
 while (*netstr_len)
 {
  int off = *netstr_len % 10;
  
  buf[--*count] = '0' + off;
  
  *netstr_len /= 10;
 }
 
 return (TRUE);
}

int vstr_add_netstr2_end(Vstr_base *base, size_t netstr_pos)
{
 size_t netstr_len = 0;
 size_t count = 0;
 char buf[BUF_NUM_TYPE_SZ(unsigned long)]; 

 assert(sizeof(buf) >= netstr2_num_len);
 assert(netstr_pos);
 
 if (!vstr__netstr_end_start(base, netstr_pos, &count, &netstr_len, buf))
   return (FALSE);

 --netstr_pos;
 
 /*
  * count == num of zero's needed
  * buf[count] == Highest digit in number
  */

#if 0
 /* try the iovec's first as that will be better for cache hits when walking
  * to the right place
  *
  * Means big chunck of code though, but it's all pretty simple
  */
 if (base->iovec_upto_date)
 {
  struct iovec *scan = base->vec.v + base->vec.off;
  size_t skip = 0;
  char *str = NULL;
  size_t len = 0;
  size_t zeros = 0;
  
  while ((skip + scan->iov_len) < netstr_pos)
  {
   skip += scan->iov_len;
   ++scan;
  }

  len = scan->iov_len;
  str = (char *)scan->iov_base;

  zeros = count;
  
  while (zeros > 0)
  {
   size_t tmp = 0;
   
   if ((tmp = vstr__netstr2_zero(str, len, &zeros)))
   {
    len -= tmp;
    str += tmp;
    break;
   }
   
   ++scan;
   
   len = scan->iov_len;
   str = (char *)scan->iov_base;
  }
  
  while (count < netstr2_num_len)
  {
   count = vstr__netstr2_copy(str, len, count);
   
   if (count == netstr2_num_len)
     break;
   
   ++scan;
   
   len = scan->iov_len;
   str = (char *)scan->iov_base;
  }
 }
 else
#endif
 {
  Vstr_node *scan = base->beg;
  size_t skip = 0;
  char *str = NULL;
  size_t len = 0;
  size_t zeros = 0;

  netstr_pos += base->used;
  
  while ((skip + scan->len) <= netstr_pos)
  {
   skip += scan->len;
   scan = scan->next;
  }

  assert(scan->type == VSTR_TYPE_NODE_BUF);
  
  len = scan->len - (netstr_pos - skip);
  str = vstr__export_node_ptr(scan) + (netstr_pos - skip);
  
  zeros = count;
  
  while (zeros > 0)
  {
   size_t tmp = 0;

   assert(scan->type == VSTR_TYPE_NODE_BUF);
   if ((tmp = vstr__netstr2_zero(str, len, &zeros)))
   {
    len -= tmp;
    str += tmp;
    break;
   }
   
   scan = scan->next;
   
   len = scan->len;
   str = vstr__export_node_ptr(scan);
  }
  
  while (count < netstr2_num_len)
  {
   assert(scan->type == VSTR_TYPE_NODE_BUF);
   count = vstr__netstr2_copy(str, buf, len, count);
   
   if (count == netstr2_num_len)
     break;
   
   scan = scan->next;
   
   len = scan->len;
   str = vstr__export_node_ptr(scan);
  }
 }

 return (TRUE);
}

/* might want to use vstr_add_pos_buf()/_ref() eventually */
size_t vstr_add_netstr_beg(Vstr_base *base)
{
 return (vstr_add_netstr2_beg(base));
}

int vstr_add_netstr_end(Vstr_base *base, size_t netstr_pos)
{
 size_t netstr_len = 0;
 size_t count = 0;
 char buf[BUF_NUM_TYPE_SZ(unsigned long)];
 
 assert(sizeof(buf) >= netstr2_num_len);
 assert(netstr_pos);
 
 if (!vstr__netstr_end_start(base, netstr_pos, &count, &netstr_len, buf))
   return (FALSE);

 --netstr_pos;
 
 if (count == netstr2_num_len)
 { /* here we delete, so need to keep something */
  buf[--count] = '0';
 }

 /*
  * count == num of zero's needed
  * buf[count] == Highest digit in number
  */
 /* can't do iovec as the delete will kill the upto date bit anyway */ 
 {
  Vstr_node *scan = NULL;
  size_t skip = 0;
  char *str = NULL;
  size_t len = 0;

  if (count && !vstr_del(base, netstr_pos + 1, count))
  {
   vstr_del(base, base->len, 1); /* remove comma, always works */
   return (FALSE);
  }
  
  netstr_pos += base->used;

  scan = base->beg;
  while ((skip + scan->len) <= netstr_pos)
  {
   skip += scan->len;
   scan = scan->next;
  }

  assert(scan->type == VSTR_TYPE_NODE_BUF);
  
  len = scan->len - (netstr_pos - skip);
  str = vstr__export_node_ptr(scan) + (netstr_pos - skip);
  
  while (count < netstr2_num_len)
  {
   assert(scan->type == VSTR_TYPE_NODE_BUF);
   count = vstr__netstr2_copy(str, buf, len, count);
   
   if (count == netstr2_num_len)
     break;
   
   scan = scan->next;
   
   len = scan->len;
   str = vstr__export_node_ptr(scan);
  }
 }

 return (TRUE);
}
