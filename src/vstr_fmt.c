#define VSTR_ADD_FMT_C
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
/* Registration/deregistration of custom format specifiers */
#include "main.h"


static
Vstr__fmt_usr_name_node **
vstr__fmt_usr_srch(Vstr_conf *conf, const char *name)
{
  Vstr__fmt_usr_name_node **scan = &conf->fmt_usr_names;
  size_t len = strlen(name);
  
  while (*scan)
  {
    assert(!(*scan)->next || ((*scan)->name_len <= (*scan)->next->name_len));
    
    if (((*scan)->name_len == len) && !memcmp((*scan)->name_str, name, len))
      return (scan);
    
    scan = &(*scan)->next;
  }
  
  return (NULL);
}

/* like srch, but matches in a format (Ie. not zero terminated) */
Vstr__fmt_usr_name_node *vstr__fmt_usr_match(Vstr_conf *conf, const char *fmt)
{
  Vstr__fmt_usr_name_node *scan = conf->fmt_usr_names;
  size_t fmt_max_len = 0;

  if (!conf->fmt_name_max)
  {
    while (scan)
    {
      if (fmt_max_len < conf->fmt_name_max)
        fmt_max_len = conf->fmt_name_max;
      
      scan = scan->next;
    }

    scan = conf->fmt_usr_names;
  }

  fmt_max_len = strnlen(fmt, conf->fmt_name_max);
  while (scan && (fmt_max_len >= scan->name_len))
  {
    assert(!scan->next || (scan->name_len <= scan->next->name_len));
    
    if (!memcmp(fmt, scan->name_str, scan->name_len))
      return (scan);
    
    scan = scan->next;
  }
  
  return (NULL);
}

int vstr_nx_fmt_add(Vstr_conf *conf, const char *name,
                    int (*func)(Vstr_base *, size_t, Vstr_fmt_spec *), ...)
{
  Vstr__fmt_usr_name_node **scan = &conf->fmt_usr_names;
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
           (scan_type == VSTR_TYPE_FMT_PTR_CHAR) ||
           (scan_type == VSTR_TYPE_FMT_PTR_WCHAR_T) ||
           (scan_type == VSTR_TYPE_FMT_ERRNO) ||
           FALSE);
    
    node->types[count - 2] = scan_type;
  }
  assert(count >= 1);
  node->types[count - 1] = scan_type;
  node->sz = count;

  va_end(ap);

  if (!*scan || (conf->fmt_name_max && (conf->fmt_name_max < node->name_len)))
    conf->fmt_name_max = node->name_len;

  while (*scan)
  {
    if ((*scan)->name_len >= node->name_len)
      break;
    
    scan = &(*scan)->next;
  }

  node->next = *scan;
  *scan = node;

  assert(vstr__fmt_usr_srch(conf, name));

  return (TRUE);
}

void vstr_nx_fmt_del(Vstr_conf *conf, const char *name)
{
  Vstr__fmt_usr_name_node **scan = vstr__fmt_usr_srch(conf, name);

  if (scan)
  {
    Vstr__fmt_usr_name_node *tmp = *scan;

    assert(tmp);

    *scan = tmp->next;
    
    if (tmp->name_len == conf->fmt_name_max)
      conf->fmt_name_max = 0;

    free(tmp);
  }
}

int vstr_nx_fmt_srch(Vstr_conf *conf, const char *name)
{
  return (!!vstr__fmt_usr_srch(conf, name));
}
