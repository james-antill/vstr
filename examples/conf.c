/*
 *  Copyright (C) 2005  James Antill
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
/* configuration file functions */

#include <vstr.h>

#include <err.h>

#include "conf.h"

#ifndef VSTR_AUTOCONF_NDEBUG
# define assert(x) do { if (x) {} else errx(EXIT_FAILURE, "assert(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
# define ASSERT(x) do { if (x) {} else errx(EXIT_FAILURE, "ASSERT(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
#else
# define ASSERT(x)
# define assert(x)
#endif
#define ASSERT_NOT_REACHED() assert(FALSE)

/* how much memory should we preallocate so it's "unlikely" we'll get mem errors
 * when writting a log entry */
#define VLG_MEM_PREALLOC (4 * 1024)

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#define VPREFIX(vstr, p, l, cstr)                                       \
    (((l) >= strlen(cstr)) && vstr_cmp_bod_cstr_eq(vstr, p, l, cstr))

static size_t conf__parse_ws(Conf_parse *conf, size_t pos, size_t len)
{
  conf->state = CONF_PARSE_STATE_CHOOSE;
  return (vstr_spn_cstr_chrs_fwd(conf->data, pos, len, " \t\v\r\n"));
}

static int conf__parse_end_list(Conf_parse *conf, unsigned int *list_nums)
{
  Vstr_sects *sects = conf->sects;
  unsigned int depth_beg_num = 0;
  
  if (!conf->depth)
    return (FALSE);

  depth_beg_num = list_nums[--conf->depth];
  VSTR_SECTS_NUM(sects, depth_beg_num)->len = sects->num - depth_beg_num;
  
  conf->state = CONF_PARSE_STATE_CHOOSE;
  return (TRUE);
}

static int conf__parse_beg_list(Conf_parse *conf, size_t pos,
                                unsigned int *list_nums)
{
  if (conf->depth >= CONF_PARSE_LIST_DEPTH_SZ)
    return (FALSE);
  
  vstr_sects_add(conf->sects, pos, 0);
  list_nums[conf->depth++] = conf->sects->num;
  
  conf->state = CONF_PARSE_STATE_CHOOSE;
  return (TRUE);
}

static void conf__parse_beg_quote_d(Conf_parse *conf, size_t pos,
                                    unsigned int *list_nums)
{
  ASSERT(conf->depth <= CONF_PARSE_LIST_DEPTH_SZ);
            
  vstr_sects_add(conf->sects, pos, 0);
  list_nums[conf->depth++] = conf->sects->num;
  
  conf->state = CONF_PARSE_STATE_QUOTE_D_BEG;
}

static void conf__parse_beg_quote_s(Conf_parse *conf, size_t pos,
                                    unsigned int *list_nums)
{
  ASSERT(conf->depth <= CONF_PARSE_LIST_DEPTH_SZ);
            
  vstr_sects_add(conf->sects, pos, 0);
  list_nums[conf->depth++] = conf->sects->num;
  
  conf->state = CONF_PARSE_STATE_QUOTE_S_BEG;
}

static void conf__parse_end_quote_x(Conf_parse *conf, size_t pos,
                                    unsigned int *list_nums)
{
  Vstr_sects *sects = conf->sects;
  unsigned int depth_beg_num = 0;
  size_t beg_pos = 0;
  
  ASSERT(conf->depth);

  depth_beg_num = list_nums[--conf->depth];
  beg_pos = VSTR_SECTS_NUM(sects, depth_beg_num)->pos;
  VSTR_SECTS_NUM(sects, depth_beg_num)->len = vstr_sc_posdiff(beg_pos, pos);
  
  conf->state = CONF_PARSE_STATE_LIST_END_OR_WS;
}

static void conf__parse_end_quote_xxx(Conf_parse *conf, size_t pos,
                                      unsigned int *list_nums)
{
  Vstr_sects *sects = conf->sects;
  unsigned int depth_beg_num = 0;
  size_t beg_pos = 0;
  
  ASSERT(conf->depth);

  depth_beg_num = list_nums[--conf->depth];
  beg_pos = VSTR_SECTS_NUM(sects, depth_beg_num)->pos;
  VSTR_SECTS_NUM(sects, depth_beg_num)->len = vstr_sc_posdiff(beg_pos, pos + 2);
  
  conf->state = CONF_PARSE_STATE_LIST_END_OR_WS;
}

int conf_parse_tokenize(Conf_parse *conf)
{
  Vstr_base *data = NULL;
  unsigned int list_nums[CONF_PARSE_LIST_DEPTH_SZ + 1];
  size_t pos = 1;
  size_t len = 0;

  ASSERT(conf && conf->data && conf->sects);

  data = conf->data;
  len  = data->len;
  if (FALSE && conf->parsed)
  {
    //    conf__setup_list_nums(conf, list_nums);
    pos = conf->parsed + 1;
    len = conf->data->len - conf->parsed;
  }
  
  while (len)
  {
    size_t plen = 0; /* amount parsed this loop */
    
    switch (conf->state)
    {
      case CONF_PARSE_STATE_BEG: /* unix she-bang */
        if (VPREFIX(data, pos, len, "#!"))
          plen = vstr_cspn_cstr_chrs_fwd(data, pos, len, "\n");
        conf->state = CONF_PARSE_STATE_WS;
        break;
        
      case CONF_PARSE_STATE_WS:
        if (!(plen = conf__parse_ws(conf, pos, len)))
          return (CONF_PARSE_ERR);
        break;
        
      case CONF_PARSE_STATE_LIST_END_OR_WS:
      {
        int byte = vstr_export_chr(data, pos);

        plen = 1;
        switch (byte)
        {
          case ' ':  /* whitespace */
          case '\t': /* whitespace */
          case '\v': /* whitespace */
          case '\r': /* whitespace */
          case '\n': /* whitespace */
            plen = conf__parse_ws(conf, pos, len);
            break;
            
          case ')':
          case ']':
            if (!conf__parse_end_list(conf, list_nums))
              return (CONF_PARSE_ERR);
            break;
          default:
            return (CONF_PARSE_ERR);
        }
        if (!plen)
          return (CONF_PARSE_ERR);
      }
      break;
        
      case CONF_PARSE_STATE_CHOOSE:
      {
        int byte = vstr_export_chr(data, pos);

        plen = 1;
        switch (byte)
        {
          case ' ':  /* whitespace */
          case '\t': /* whitespace */
          case '\v': /* whitespace */
          case '\r': /* whitespace */
          case '\n': /* whitespace */
            plen = conf__parse_ws(conf, pos, len);
            break;
            
          case ')':
          case ']':
            if (!conf__parse_end_list(conf, list_nums))
              return (CONF_PARSE_ERR);
            break;

          case '(':
          case '[':
            if (!conf__parse_beg_list(conf, pos, list_nums))
              return (CONF_PARSE_ERR);
            break;
            
          case '"':
            conf__parse_beg_quote_d(conf, pos, list_nums);
            break;
          case '\'':
            conf__parse_beg_quote_s(conf, pos, list_nums);
            break;
            
          default:
            plen = vstr_cspn_cstr_chrs_fwd(data, pos, len, " \t\v\r\n\"'()[]");
            conf->state = CONF_PARSE_STATE_SYMBOL_END;
            vstr_sects_add(conf->sects, pos, plen);
            break;            
        }
      }
      break;
        
      case CONF_PARSE_STATE_QUOTE_D_BEG:
      {
        static const char dd[] = {'"', '"', 0};
        
        if (!VPREFIX(data, pos, len, dd))
          conf->state = CONF_PARSE_STATE_QUOTE_D_END;
        else
        {
          plen = 2;
          conf->state = CONF_PARSE_STATE_QUOTE_DDD_END;
        }
      }
      break;
      
      case CONF_PARSE_STATE_QUOTE_S_BEG:
        if (!VPREFIX(data, pos, len, "''"))
          conf->state = CONF_PARSE_STATE_QUOTE_S_END;
        else
        {
          plen = 2;
          conf->state = CONF_PARSE_STATE_QUOTE_SSS_END;
        }
        break;
        
      case CONF_PARSE_STATE_QUOTE_D_END:
        if (!(plen = vstr_cspn_cstr_chrs_fwd(data, pos, len, "\"\\")))
        {
          plen = 1;
          if (vstr_export_chr(data, pos) == '"')
            conf__parse_end_quote_x(conf, pos, list_nums);
          else /* \x */
          {
            ASSERT(vstr_export_chr(data, pos) == '\\');
            
            if (len < 2)
              return (CONF_PARSE_ERR);
            plen = 2;
          }
        }
        break;
        
      case CONF_PARSE_STATE_QUOTE_DDD_END:
        if (!(plen = vstr_cspn_cstr_chrs_fwd(data, pos, len, "\"\\")))
        {
          static const char ddd[] = {'"', '"', '"', 0};
          
          plen = 3;
          if (VPREFIX(data, pos, len, ddd))
            conf__parse_end_quote_xxx(conf, pos, list_nums);
          else if (vstr_export_chr(data, pos) != '\\')
            plen = 1;
          else
          {
            if (len < 2)
              return (CONF_PARSE_ERR);
            plen = 2;
          }
        }
        break;
        
      case CONF_PARSE_STATE_QUOTE_S_END:
        if (!(plen = vstr_cspn_cstr_chrs_fwd(data, pos, len, "'\\")))
        {
          plen = 1;
          if (vstr_export_chr(data, pos) == '\'')
            conf__parse_end_quote_x(conf, pos, list_nums);
          else /* \x */
          {
            ASSERT(vstr_export_chr(data, pos) == '\\');
            
            if (len < 2)
              return (CONF_PARSE_ERR);
            plen = 2;
          }
        }
        break;
        
      case CONF_PARSE_STATE_QUOTE_SSS_END:
        if (!(plen = vstr_cspn_cstr_chrs_fwd(data, pos, len, "'\\")))
        {
          plen = 3;
          if (VPREFIX(data, pos, len, "'''"))
            conf__parse_end_quote_xxx(conf, pos, list_nums);
          else if (vstr_export_chr(data, pos) != '\\')
            plen = 1;
          else if (vstr_export_chr(data, pos) == '\\')
          {
            if (len < 2)
              return (CONF_PARSE_ERR);
            plen = 2;
          }
        }
        break;
        
      case CONF_PARSE_STATE_SYMBOL_END:
      {
        int byte = vstr_export_chr(data, pos);

        plen = 1;
        switch (byte)
        {
          case ' ':  /* whitespace */
          case '\t': /* whitespace */
          case '\v': /* whitespace */
          case '\r': /* whitespace */
          case '\n': /* whitespace */
            plen = conf__parse_ws(conf, pos, len);
            break;
            
          case ')':
          case ']':
            if (!conf__parse_end_list(conf, list_nums))
              return (CONF_PARSE_ERR);
            break;

          case '(':
          case '[':
            if (!conf__parse_beg_list(conf, pos, list_nums))
              return (CONF_PARSE_ERR);
            break;
            
          case '"':
          case '\'':
            return (CONF_PARSE_ERR);
          default:
            assert(FALSE);
            return (CONF_PARSE_ERR);
        }
      }
      break;
      
      default:
        assert(FALSE);
        return (CONF_PARSE_ERR);
    }

    len -= plen; pos += plen;
  }

  if (conf->sects->malloc_bad)
    return (CONF_PARSE_ERR);
    
  return (CONF_PARSE_FIN);
}

Vstr_sect_node *conf_parse_token(Conf_parse *conf, unsigned int *num)
{
  ASSERT(conf && conf->sects && num);
  ASSERT(!conf->depth);

  if (*num >= conf->sects->num)
    return (NULL);

  ++*num;
  return (VSTR_SECTS_NUM(conf->sects, *num));
}

unsigned int conf_parse_type(Vstr_base *s1, size_t pos, size_t len)
{
  int byte = 0;

  ASSERT(s1 && pos);
  if (vstr_sc_poslast(pos, len) > s1->len)
    return (CONF_PARSE_TYPE_ERR);
  
  switch ((byte = vstr_export_chr(s1, pos)))
  {
    case ' ':  /* whitespace */
    case '\t': /* whitespace */
    case '\v': /* whitespace */
    case '\r': /* whitespace */
    case '\n': /* whitespace */
    case ')':  /* parse is fucked */
    case ']':  /* parse is fucked */
      assert(FALSE);
      return (CONF_PARSE_TYPE_ERR);
      
    case '(': return (CONF_PARSE_TYPE_CLIST);
    case '[': return (CONF_PARSE_TYPE_SLIST);

    case '"':
    {
      static const char ddd[] = {'"', '"', '"', 0};
      if (VPREFIX(s1, pos, len, ddd))
        return (CONF_PARSE_TYPE_QUOTE_DDD);
      return (CONF_PARSE_TYPE_QUOTE_D);
    }
    
    case '\'':
      if (VPREFIX(s1, pos, len, "'''"))
        return (CONF_PARSE_TYPE_QUOTE_SSS);
      return (CONF_PARSE_TYPE_QUOTE_S);

    default:
      return (CONF_PARSE_TYPE_SYMBOL);
  }
}

static const char *conf__parse_token_name_map[] = {
 "<** Error **>",
 "Circular bracket list",
 "Square bracket list",
 "Quoted string (3x double)",
 "Quoted string (double)",
 "Quoted string (3x single)",
 "Quoted string (single)",
 "Symbol"
};

const char *conf_parse_token_type2name(unsigned int type)
{
  if (type > CONF_PARSE_TYPE_SYMBOL)
    type = 0;

  return (conf__parse_token_name_map[type]);
}
