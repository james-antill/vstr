#define VSTR_ADD_FMT_C
/*
 *  Copyright (C) 2002, 2003  James Antill
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
    
    if (((*scan)->name_len == len) &&
        !vstr_wrap_memcmp((*scan)->name_str, name, len))
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
  
  if (conf->fmt_usr_curly_braces)
  { /* we know they follow a format of one of...
       "{" [^}]* "}"
       "[" [^]]* "]"
       "<" [^>]* ">"
       "(" [^)]* ")"
     * so we can find the length */
    char *ptr = NULL;
    size_t len = 0;

    if (*fmt == '{') ptr = strchr(fmt, '}');
    if (*fmt == '[') ptr = strchr(fmt, ']');
    if (*fmt == '<') ptr = strchr(fmt, '>');
    if (*fmt == '(') ptr = strchr(fmt, ')');
    
    if (!ptr)
      return (NULL);

    len = (ptr - fmt) + 1;
    while (scan)
    {
      assert(!scan->next || (scan->name_len <= scan->next->name_len));
      
      if ((scan->name_len == len) &&
          !vstr_wrap_memcmp(scan->name_str, fmt, len))
        return (scan);

      if (scan->name_len > len)
        break;
      
      scan = scan->next;
    }
    
    return (NULL);
  }
  
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
    
    if (!vstr_wrap_memcmp(fmt, scan->name_str, scan->name_len))
      return (scan);
    
    scan = scan->next;
  }
  
  return (NULL);
}

#define VSTR__FMT_ADD_Q(name, len, b1, b2) ( \
  ((name)[0] == (b1)) && \
  ((name)[(len) - 1] == (b2)) && \
  (((len) == 2) || ((len) > 2)) && \
  !memchr((name) + 1, (b1), (len) - 2) && \
  !memchr((name) + 1, (b2), (len) - 2) \
 )

int vstr_fmt_add(Vstr_conf *passed_conf, const char *name,
                 int (*func)(Vstr_base *, size_t, Vstr_fmt_spec *), ...)
{
  Vstr_conf *conf = passed_conf ? passed_conf : vstr__options.def;
  Vstr__fmt_usr_name_node **scan = &conf->fmt_usr_names;
  va_list ap;
  unsigned int count = 1;
  unsigned int scan_type = 0;
  Vstr__fmt_usr_name_node *node = NULL;

  node = VSTR__MK(sizeof(Vstr__fmt_usr_name_node) +
                  (sizeof(unsigned int) * count));

  if (!node)
  {
    conf->malloc_bad = TRUE;
    return (FALSE);
  }
  
  if (vstr__fmt_usr_srch(conf, name))
    return (FALSE);
  
  node->name_str = name;
  node->name_len = strlen(name);
  node->func = func;

  if (conf->fmt_usr_curly_braces &&
      !VSTR__FMT_ADD_Q(name, node->name_len, '{', '}') &&
      !VSTR__FMT_ADD_Q(name, node->name_len, '[', ']') &&
      !VSTR__FMT_ADD_Q(name, node->name_len, '<', '>') &&
      !VSTR__FMT_ADD_Q(name, node->name_len, '(', ')'))
    conf->fmt_usr_curly_braces = FALSE;
  
  va_start(ap, func);
  while ((scan_type = va_arg(ap, unsigned int)))
  {
    ++count;
    if (!VSTR__MV(node, sizeof(Vstr__fmt_usr_name_node) +
                  (sizeof(unsigned int) * count)))
    {
      conf->malloc_bad = TRUE;
      VSTR__F(node);
      va_end(ap);
      return (FALSE);
    }

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

void vstr_fmt_del(Vstr_conf *passed_conf, const char *name)
{
  Vstr_conf *conf = passed_conf ? passed_conf : vstr__options.def;
  Vstr__fmt_usr_name_node **scan = vstr__fmt_usr_srch(conf, name);

  if (scan)
  {
    Vstr__fmt_usr_name_node *tmp = *scan;

    assert(tmp);

    *scan = tmp->next;
    
    if (tmp->name_len == conf->fmt_name_max)
      conf->fmt_name_max = 0;

    VSTR__F(tmp);
  }
}

int vstr_fmt_srch(Vstr_conf *passed_conf, const char *name)
{
  Vstr_conf *conf = passed_conf ? passed_conf : vstr__options.def;
  return (!!vstr__fmt_usr_srch(conf, name));
}
