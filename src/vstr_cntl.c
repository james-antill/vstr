#define VSTR_CNTL_C
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
/* functions for the "misc" stuff. like reading struct values etc. */
#include "main.h"


Vstr__options vstr__options =
{
 NULL,
};

int vstr_cntl_opt(int option, ...)
{
 int ret = 0;
 
 va_list ap;

 va_start(ap, option);
 
 switch (option)
 {
  case VSTR_CNTL_OPT_GET_CONF:
  {
   Vstr_conf **val = va_arg(ap, Vstr_conf **);
   
   *val = vstr__options.def;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_OPT_SET_CONF:
  {
   Vstr_conf *val = va_arg(ap, Vstr_conf *);

   vstr__del_conf(vstr__options.def);
   vstr__add_conf(vstr__options.def = val);
   
   ret = TRUE;
  }
  break;
  
  default:
    break;
 }
 
 va_end(ap);
 
 return (ret);
}

int vstr_cntl_base(Vstr_base *base, int option, ...)
{
 int ret = 0;
 
 va_list ap;

 va_start(ap, option);
 
 switch (option)
 {
  case VSTR_CNTL_BASE_GET_CONF:
  {
   Vstr_conf **val = va_arg(ap, Vstr_conf **);
   
   *val = base->conf;
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_BASE_SET_CONF:
  {
   Vstr_conf *val = va_arg(ap, Vstr_conf *);

   vstr__del_conf(base->conf);
   vstr__base_add_conf(base, val);
   
   ret = TRUE;
  }
  break;

  default:
    break;
 }

 va_end(ap);
 
 return (ret);
}

int vstr_cntl_conf(Vstr_conf *conf, int option, ...)
{
 int ret = 0;
 
 va_list ap;

 va_start(ap, option);
 
 switch (option)
 {
  case VSTR_CNTL_CONF_GET_NUM_REF:
  {
   int *val = va_arg(ap, int *);
   
   *val = conf->ref;
   ret = TRUE;
  }
  break;
  
  /* case VSTR_CNTL_BASE_SET_REF: */

  case VSTR_CNTL_CONF_GET_NUM_IOV_MIN_ALLOC:
  {
   int *val = va_arg(ap, int *);
   
   *val = conf->iov_min_alloc;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_CONF_SET_NUM_IOV_MIN_ALLOC:
  {
   int val = va_arg(ap, int);
   
   conf->iov_min_alloc = val;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_CONF_GET_NUM_IOV_MIN_OFFSET:
  {
   int *val = va_arg(ap, int *);
   
   *val = conf->iov_min_offset;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_CONF_SET_NUM_IOV_MIN_OFFSET:
  {
   int val = va_arg(ap, int);
   
   conf->iov_min_offset = val;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_CONF_GET_NUM_BUF_SZ:
  {
   int *val = va_arg(ap, int *);
   
   *val = conf->buf_sz;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_CONF_SET_NUM_BUF_SZ:
  {
   int val = va_arg(ap, int);

   /* need to change all nodes in the base(s), and in spare_buf_beg */
   /*  vstr__cache_add(base, base->len, bytes); -- or _del ... or ... */
   if (!conf->spare_buf_num && !conf->ref)
   {
    conf->buf_sz = val;
    
    ret = TRUE;
   }
  }
  break;
  
  case VSTR_CNTL_CONF_GET_FLAG_MALLOC_BAD:
  {
   int *val = va_arg(ap, int *);
   
   *val = conf->malloc_bad;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_CONF_SET_FLAG_MALLOC_BAD:
  {
   int val = va_arg(ap, int);

   conf->malloc_bad = !!val;
   
   ret = FALSE;
  }
  break;
  
  default:
    break;
 }

 va_end(ap);
 
 return (ret);
}
