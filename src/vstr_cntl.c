#define VSTR_CNTL_C
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
/* functions for the "misc" control stuff. like reading struct values etc. */
#include "main.h"


Vstr__options vstr__options =
{
 NULL,
 0,
 0,
};

int vstr_nx_cntl_opt(int option, ...)
{
 int ret = 0;
 
 va_list ap;

 va_start(ap, option);
 
 switch (option)
 {
  case VSTR_CNTL_OPT_GET_CONF:
  {
   Vstr_conf **val = va_arg(ap, Vstr_conf **);
   
   vstr__add_user_conf(*val = vstr__options.def);
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_OPT_SET_CONF:
  {
   Vstr_conf *val = va_arg(ap, Vstr_conf *);

   if (vstr__options.def != val)
   {
     vstr_nx_free_conf(vstr__options.def);
     vstr__add_user_conf(vstr__options.def = val);
   }
   
   ret = TRUE;
  }
  break;
  
  default:
    break;
 }
 
 va_end(ap);
 
 return (ret);
}

int vstr_nx_cntl_base(Vstr_base *base, int option, ...)
{
 int ret = 0;
 
 va_list ap;

 va_start(ap, option);
 
 switch (option)
 {
  case VSTR_CNTL_BASE_GET_CONF:
  {
    Vstr_conf **val = va_arg(ap, Vstr_conf **);
    
    vstr__add_user_conf(*val = base->conf);
    
    ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_BASE_SET_CONF:
  {
    Vstr_conf *val = va_arg(ap, Vstr_conf *);
    
    if (!val)
      val = vstr__options.def;
    
    if (base->conf == val)
      ret = TRUE;
    else if (((val->buf_sz == base->conf->buf_sz) || !base->len) &&
             vstr__cache_subset_cbs(val, base->conf))
    {
      vstr__del_conf(base->conf);
      vstr__add_base_conf(base, val);
      
      ret = TRUE;
    }
  }
  break;

  default:
    break;
 }

 va_end(ap);
 
 return (ret);
}

int vstr_nx_cntl_conf(Vstr_conf *passed_conf, int option, ...)
{
  Vstr_conf *conf = passed_conf ? passed_conf : vstr__options.def;
  int ret = 0;
  va_list ap;

 assert(conf->user_ref <= conf->ref);
   
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
    unsigned int *val = va_arg(ap, unsigned int *);
   
   *val = conf->buf_sz;
   
   ret = TRUE;
  }
  break;
  
  case VSTR_CNTL_CONF_SET_NUM_BUF_SZ:
  {
    unsigned int val = va_arg(ap, unsigned int);

    if (val > VSTR_MAX_NODE_BUF)
      val = VSTR_MAX_NODE_BUF;
    
    /* this is too restrictive, but getting it "right" would require too much
     * bookkeeping. */
    if (!conf->spare_buf_num && (conf->user_ref == conf->ref))
    {
      conf->buf_sz = val;
      
      ret = TRUE;
    }
  }
  break;

   case VSTR_CNTL_CONF_SET_LOC_CSTR_AUTO_NAME_NUMERIC:
   {
     const char *val = va_arg(ap, const char *);

     ret = vstr__make_conf_loc_numeric(conf, val);
   }
   break;

   case VSTR_CNTL_CONF_GET_LOC_CSTR_NAME_NUMERIC:
   {
     const char **val = va_arg(ap, const char **);

     *val = conf->loc->name_lc_numeric_str;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_SET_LOC_CSTR_NAME_NUMERIC:
   {
     const char *val = va_arg(ap, const char *);
     char *tmp = NULL;
     size_t len = strlen(val);

     if (!(tmp = VSTR__MK(len + 1)))
       break;

     VSTR__F(conf->loc->name_lc_numeric_str);
     vstr_nx_wrap_memcpy(tmp, val, len + 1);
     conf->loc->name_lc_numeric_str = tmp;
     conf->loc->name_lc_numeric_len = len;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_LOC_CSTR_DEC_POINT:
   {
     const char **val = va_arg(ap, const char **);

     *val = conf->loc->decimal_point_str;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_SET_LOC_CSTR_DEC_POINT:
   {
     const char *val = va_arg(ap, const char *);
     char *tmp = NULL;
     size_t len = strlen(val);

     if (!(tmp = VSTR__MK(len + 1)))
       break;

     VSTR__F(conf->loc->decimal_point_str);
     vstr_nx_wrap_memcpy(tmp, val, len + 1);
     conf->loc->decimal_point_str = tmp;
     conf->loc->decimal_point_len = len;     
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_LOC_CSTR_THOU_SEP:
   {
     const char **val = va_arg(ap, const char **);

     *val = conf->loc->thousands_sep_str;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_SEP:
   {
     const char *val = va_arg(ap, const char *);
     char *tmp = NULL;
     size_t len = strlen(val);

     if (!(tmp = VSTR__MK(len + 1)))
       break;

     VSTR__F(conf->loc->thousands_sep_str);
     vstr_nx_wrap_memcpy(tmp, val, len + 1);
     conf->loc->thousands_sep_str = tmp;
     conf->loc->thousands_sep_len = len;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_LOC_CSTR_THOU_GRP:
   {
     const char **val = va_arg(ap, const char **);

     *val = conf->loc->grouping;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_GRP:
   {
     const char *val = va_arg(ap, const char *);
     char *tmp = NULL;
     size_t len = vstr__loc_thou_grp_strlen(val);

     if (!(tmp = VSTR__MK(len + 1)))
       break;

     VSTR__F(conf->loc->grouping);
     vstr_nx_wrap_memcpy(tmp, val, len); tmp[len] = 0;
     conf->loc->grouping = tmp;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_FLAG_IOV_UPDATE:
   {
     int *val = va_arg(ap, int *);
     
     *val = conf->iovec_auto_update;
     
     ret = TRUE;
   }
   break;
  
   case VSTR_CNTL_CONF_SET_FLAG_IOV_UPDATE:
   {
     int val = va_arg(ap, int);

     assert(val == !!val);

     conf->iovec_auto_update = val;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_FLAG_DEL_SPLIT:
   {
     int *val = va_arg(ap, int *);
     
     *val = conf->split_buf_del;
     
     ret = TRUE;
   }
   break;
  
   case VSTR_CNTL_CONF_SET_FLAG_DEL_SPLIT:
   {
     int val = va_arg(ap, int);

     assert(val == !!val);

     conf->split_buf_del = val;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_FLAG_NO_ALLOC_CACHE:
   {
     int *val = va_arg(ap, int *);
     
     *val = conf->no_cache;
     
     ret = TRUE;
   }
   break;
  
   case VSTR_CNTL_CONF_SET_FLAG_NO_ALLOC_CACHE:
   {
     int val = va_arg(ap, int);

     assert(val == !!val);

     conf->no_cache = val;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_FMT_CHAR_ESC:
   {
     char *val = va_arg(ap, char *);
     
     *val = conf->fmt_usr_escape;
     
     ret = TRUE;
   }
   break;
  
   case VSTR_CNTL_CONF_SET_FMT_CHAR_ESC:
   {
     int val = va_arg(ap, int);

     conf->fmt_usr_escape = val;
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_NUM_SPARE_BUF:
   case VSTR_CNTL_CONF_GET_NUM_SPARE_NON:
   case VSTR_CNTL_CONF_GET_NUM_SPARE_PTR:
   case VSTR_CNTL_CONF_GET_NUM_SPARE_REF:
   {
     unsigned int *val = va_arg(ap, unsigned int *);
     
     switch (option)
     {
       case VSTR_CNTL_CONF_GET_NUM_SPARE_BUF: *val = conf->spare_buf_num; break;
       case VSTR_CNTL_CONF_GET_NUM_SPARE_NON: *val = conf->spare_non_num; break;
       case VSTR_CNTL_CONF_GET_NUM_SPARE_PTR: *val = conf->spare_ptr_num; break;
       case VSTR_CNTL_CONF_GET_NUM_SPARE_REF: *val = conf->spare_ref_num; break;
     }
     
     ret = TRUE;
   }
   break;
   
   case VSTR_CNTL_CONF_SET_NUM_SPARE_BUF:
   case VSTR_CNTL_CONF_SET_NUM_SPARE_NON:
   case VSTR_CNTL_CONF_SET_NUM_SPARE_PTR:
   case VSTR_CNTL_CONF_SET_NUM_SPARE_REF:
   {
     unsigned int val = va_arg(ap, unsigned int);
     unsigned int type = 0;
     
     switch (option)
     {
       case VSTR_CNTL_CONF_SET_NUM_SPARE_BUF: type = VSTR_TYPE_NODE_BUF; break;
       case VSTR_CNTL_CONF_SET_NUM_SPARE_NON: type = VSTR_TYPE_NODE_NON; break;
       case VSTR_CNTL_CONF_SET_NUM_SPARE_PTR: type = VSTR_TYPE_NODE_PTR; break;
       case VSTR_CNTL_CONF_SET_NUM_SPARE_REF: type = VSTR_TYPE_NODE_REF; break;
     }
     
     if (val == conf->spare_buf_num)
     { /* do nothing */ }
     else if (val > conf->spare_buf_num)
     {
       unsigned int num = 0;
       
       num = vstr_nx_make_spare_nodes(conf, type, val - conf->spare_buf_num);
       
       if (num != (val - conf->spare_buf_num))
       {
         assert(ret == FALSE);
         break;
       }
     }
     else if (val < conf->spare_buf_num)
     {
       unsigned int num = 0;
       
       num = vstr_nx_free_spare_nodes(conf, type, conf->spare_buf_num - val);
       assert(num == (conf->spare_buf_num - val));
     }
     
     ret = TRUE;
   }
   break;

   case VSTR_CNTL_CONF_GET_ATOMIC_OPS:
   {
     char *val = va_arg(ap, char *);
     
     *val = conf->atomic_ops;
     
     ret = TRUE;
   }
   break;
  
   case VSTR_CNTL_CONF_SET_ATOMIC_OPS:
   {
     int val = va_arg(ap, int);

     conf->atomic_ops = val;
     
     ret = TRUE;
   }
   break;

  default:
    break;
 }

 va_end(ap);
 
 return (ret);
}
