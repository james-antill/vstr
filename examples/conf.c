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

#define CONF_COMPILE_INLINE 0
#include "conf.h"

#include <err.h>

#ifndef VSTR_AUTOCONF_NDEBUG
# define assert(x) do { if (x) {} else errx(EXIT_FAILURE, "assert(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
# define ASSERT(x) do { if (x) {} else errx(EXIT_FAILURE, "ASSERT(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
#else
# define ASSERT(x)
# define assert(x)
#endif
#define ASSERT_NOT_REACHED() assert(FALSE)

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

/* if the last token is a line comment token ... comment out that line */
static size_t conf__parse_comment(Conf_parse *conf, size_t pos, size_t len)
{
  Vstr_sect_node *node = NULL;

  ASSERT(conf->sects);
  
  if (conf->sects->malloc_bad)
    return (0);

  ASSERT(conf->sects->num);
  
  node = VSTR_SECTS_NUM(conf->sects, conf->sects->num);
              
  if (node->len == 1)
  {
    int byte = vstr_export_chr(conf->data, node->pos);
    
    if ((byte == ';') || (byte == '#'))
    {
      vstr_sects_del(conf->sects, conf->sects->num);
      conf->state = CONF_PARSE_STATE_CHOOSE;
      return (vstr_cspn_cstr_chrs_fwd(conf->data, pos, len, "\n"));
    }
  }
  
  return (0);
}

static int conf__parse_end_list(Conf_parse *conf, unsigned int *list_nums,
                                int byte)
{
  Vstr_sects *sects = conf->sects;
  unsigned int depth_beg_num = 0;
  Vstr_sect_node *node = NULL;
  
  if (!conf->depth)
    return (FALSE);

  depth_beg_num = list_nums[--conf->depth];
  node = VSTR_SECTS_NUM(sects, depth_beg_num);
  if ((byte == ']') && (vstr_export_chr(conf->data, node->pos) != '['))
    return (FALSE);
  if ((byte == ')') && (vstr_export_chr(conf->data, node->pos) != '('))
    return (FALSE);

  ASSERT(!node->len);
  node->len = sects->num - depth_beg_num;
  
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

int conf_parse_lex(Conf_parse *conf)
{
  Vstr_base *data = NULL;
  unsigned int list_nums[CONF_PARSE_LIST_DEPTH_SZ + 1];
  size_t pos = 1;
  size_t len = 0;

  ASSERT(conf && conf->data && conf->sects);

  data = conf->data;
  len  = data->len;
  /*  if (FALSE && conf->parsed)
  {
    conf__setup_list_nums(conf, list_nums);
    pos = conf->parsed + 1;
    len = conf->data->len - conf->parsed;
    } */

  if (conf->state != CONF_PARSE_STATE_BEG)
    return (CONF_PARSE_ERR);

  while (len)
  {
    size_t plen = 0; /* amount parsed this loop */
    
    switch (conf->state)
    {
      case CONF_PARSE_STATE_BEG: /* unix she-bang */
        conf->state = CONF_PARSE_STATE_CHOOSE;
        if (VPREFIX(data, pos, len, "#!"))
        {
          plen = vstr_cspn_cstr_chrs_fwd(data, pos, len, "\n");
          conf->state = CONF_PARSE_STATE_WS;
        }
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
            if (!conf__parse_end_list(conf, list_nums, byte))
              return (CONF_PARSE_ERR);
            break;
          default:
            return (CONF_PARSE_ERR);
        }
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
            if (!conf__parse_end_list(conf, list_nums, byte))
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

        switch (byte)
        {
          case ' ':  /* whitespace */
          case '\t': /* whitespace */
          case '\v': /* whitespace */
          case '\r': /* whitespace */
          case '\n': /* whitespace */
            if (!(plen = conf__parse_comment(conf, pos, len)))
              plen = conf__parse_ws(conf, pos, len);
            break;
            
          case ')':
          case ']':
            if (!(plen = conf__parse_comment(conf, pos, len)))
            {
              plen = 1;
              if (!conf__parse_end_list(conf, list_nums, byte))
                return (CONF_PARSE_ERR);
            }
            break;

          case '(':
          case '[':
            if (!(plen = conf__parse_comment(conf, pos, len)))
            {
              plen = 1;
              if (!conf__parse_beg_list(conf, pos, list_nums))
                return (CONF_PARSE_ERR);
            }
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

  conf->state = CONF_PARSE_STATE_END;
  return (CONF_PARSE_FIN);
}

static unsigned int conf__tok_get_type(Vstr_base *s1, size_t pos, size_t len)
{
  int byte = 0;

  ASSERT(s1 && pos);
  if (vstr_sc_poslast(pos, len) > s1->len)
    return (CONF_TOKEN_TYPE_ERR);
  
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
      return (CONF_TOKEN_TYPE_ERR);
      
    case '(': return (CONF_TOKEN_TYPE_CLIST);
    case '[': return (CONF_TOKEN_TYPE_SLIST);

    case '"':
    {
      static const char ddd[] = {'"', '"', '"', 0};
      if (VPREFIX(s1, pos, len, ddd))
        return (CONF_TOKEN_TYPE_QUOTE_DDD);
      return (CONF_TOKEN_TYPE_QUOTE_D);
    }
    
    case '\'':
      if (VPREFIX(s1, pos, len, "'''"))
        return (CONF_TOKEN_TYPE_QUOTE_SSS);
      return (CONF_TOKEN_TYPE_QUOTE_S);

    default:
      return (CONF_TOKEN_TYPE_SYMBOL);
  }
}

int conf_parse_token(const Conf_parse *conf, Conf_token *token)
{
  Vstr_sect_node *node = NULL;
 
  ASSERT(conf && conf->sects && token);
  ASSERT(!conf->depth); /* finished lex */

  if (token->num >= conf->sects->num)
    return (FALSE);
  ++token->num;
  
  while (token->depth_num &&
         (token->depth_nums[token->depth_num - 1] < token->num))
  {
    ASSERT(token->depth_nums[token->depth_num - 1] == (token->num - 1));
    --token->depth_num;
  }
  
  node = VSTR_SECTS_NUM(conf->sects, token->num);
  
  token->node->pos = node->pos;
  token->node->len = node->len;
  token->type      = conf__tok_get_type(conf->data, node->pos, node->len);

  if (0) { }
  else if ((token->type == CONF_TOKEN_TYPE_QUOTE_DDD) ||
           (token->type == CONF_TOKEN_TYPE_QUOTE_SSS))
  {
    ASSERT(token->node->len >= 6);
    token->node->pos += 3;
    token->node->len -= 6;
  }
  else if ((token->type == CONF_TOKEN_TYPE_QUOTE_D) ||
           (token->type == CONF_TOKEN_TYPE_QUOTE_S))
  {
    ASSERT(token->node->len >= 2);
    token->node->pos += 1;
    token->node->len -= 2;
  }
  else if ((token->type == CONF_TOKEN_TYPE_CLIST) ||
           (token->type == CONF_TOKEN_TYPE_SLIST))
    token->depth_nums[token->depth_num++] = token->num + token->node->len;

  return (TRUE);
}

void conf_token_init(Conf_token *token)
{
  Conf_token dummy = CONF_TOKEN_INIT;
  *token = dummy;
}

static const char *conf__token_name_map[] = {
 "<** Error **>",
 "Circular bracket list",
 "Square bracket list",
 "Quoted string (3x double)",
 "Quoted string (double)",
 "Quoted string (3x single)",
 "Quoted string (single)",
 "Symbol"
};

const char *conf_token_name(const Conf_token *token)
{
  ASSERT(token && (token->type <= CONF_TOKEN_TYPE_SYMBOL));

  return (conf__token_name_map[token->type]);
}

const Vstr_sect_node *conf_token_value(const Conf_token *token)
{
  ASSERT(token && (token->type <= CONF_TOKEN_TYPE_SYMBOL));
  
  if ((token->type >= CONF_TOKEN_TYPE_QUOTE_DDD) &&
      (token->type <= CONF_TOKEN_TYPE_SYMBOL))
    return (token->node);
  
  return (NULL);
}

int conf_token_cmp_val_cstr_eq(const Conf_parse *conf, const Conf_token *token,
                               const char *cstr)
{
  const Vstr_sect_node *val = conf_token_value(token);
  
  if (!val)
    return (FALSE);
  
  return (vstr_cmp_cstr_eq(conf->data, val->pos, val->len, cstr));
}

int conf_token_cmp_sym_cstr_eq(const Conf_parse *conf, const Conf_token *token,
                               const char *cstr)
{
  ASSERT(conf && conf->sects &&
         token && (token->type <= CONF_TOKEN_TYPE_SYMBOL));
  
  if (token->type == CONF_TOKEN_TYPE_SYMBOL)
    return (conf_token_cmp_val_cstr_eq(conf, token, cstr));

  return (FALSE);
}

int conf_token_cmp_str_cstr_eq(const Conf_parse *conf, const Conf_token *token,
                               const char *cstr)
{
  ASSERT(conf && conf->sects &&
         token && (token->type <= CONF_TOKEN_TYPE_SYMBOL));
  
  if ((token->type >= CONF_TOKEN_TYPE_QUOTE_DDD) &&
      (token->type <= CONF_TOKEN_TYPE_QUOTE_S))
    return (conf_token_cmp_val_cstr_eq(conf, token, cstr));
  
  return (FALSE);
}

extern inline int conf_token_cmp_case_val_cstr_eq(const Conf_parse *conf,
                                                  const Conf_token *token,
                                                  const char *cstr)
{
  const Vstr_sect_node *val = conf_token_value(token);
  
  if (!val) return (FALSE);
  
  return (vstr_cmp_case_cstr_eq(conf->data, val->pos, val->len, cstr));
}

extern inline int conf_token_cmp_case_sym_cstr_eq(const Conf_parse *conf,
                                                  const Conf_token *token,
                                                  const char *cstr)
{
  ASSERT(conf && conf->sects &&
         token && (token->type <= CONF_TOKEN_TYPE_SYMBOL));
  
  if (token->type == CONF_TOKEN_TYPE_SYMBOL)
    return (conf_token_cmp_case_val_cstr_eq(conf, token, cstr));
  return (FALSE);
}

extern inline int conf_token_cmp_case_str_cstr_eq(const Conf_parse *conf,
                                                  const Conf_token *token,
                                                  const char *cstr)
{
  ASSERT(conf && conf->sects &&
         token && (token->type <= CONF_TOKEN_TYPE_SYMBOL));
  
  if ((token->type >= CONF_TOKEN_TYPE_QUOTE_DDD) &&
      (token->type <= CONF_TOKEN_TYPE_QUOTE_S))
    return (conf_token_cmp_case_val_cstr_eq(conf, token, cstr));
  
  return (FALSE);
}

unsigned int conf_token_list_num(const Conf_token *token, unsigned int depth)
{
  ASSERT(token && (token->type <= CONF_TOKEN_TYPE_SYMBOL));
  
  if (!depth || (depth > token->depth_num))
    return (0);

  ASSERT(token->depth_nums[depth - 1] >= token->num);
  
  return (token->depth_nums[depth - 1] - token->num);
}

int conf_sc_token_parse_toggle(const Conf_parse *conf,
                               Conf_token *token, int *val)
{
  unsigned int num = conf_token_list_num(token, token->depth_num);
  int ern = CONF_SC_TYPE_RET_OK;
  
  ASSERT(val);

  if (num > 1)
    ern = CONF_SC_TYPE_RET_ERR_TOO_MANY;
  
  if (!num)
  {
    *val = !*val;
    return (ern);
  }

  conf_parse_token(conf, token);
  if (0) { }
  else if (conf_token_cmp_case_val_cstr_eq(conf, token, "on") ||
           conf_token_cmp_case_val_cstr_eq(conf, token, "true") ||
           conf_token_cmp_val_cstr_eq(conf, token, "1"))
    *val = 1;
  else if (conf_token_cmp_case_val_cstr_eq(conf, token, "off") ||
           conf_token_cmp_case_val_cstr_eq(conf, token, "false") ||
           conf_token_cmp_val_cstr_eq(conf, token, "0"))
    *val = 0;
  else
    ern = CONF_SC_TYPE_RET_ERR_NO_MATCH;

  return (ern);
}

int conf_sc_token_parse_uint(const Conf_parse *conf, Conf_token *token,
                             unsigned int *val)
{
  unsigned int num = conf_token_list_num(token, token->depth_num);
  int ern = CONF_SC_TYPE_RET_OK;
  const Vstr_sect_node *pv = NULL;
  unsigned int nflags = VSTR_FLAG02(PARSE_NUM, OVERFLOW, SEP);
  size_t len = 0;
  
  ASSERT(val);

  if (!num)
    return (CONF_SC_TYPE_RET_ERR_NOT_EXIST);
  if (num > 1)
    ern = CONF_SC_TYPE_RET_ERR_TOO_MANY;

  conf_parse_token(conf, token);
  pv   = conf_token_value(token);
  *val = vstr_parse_uint(conf->data, pv->pos, pv->len, nflags, &len, NULL);
  if (len != pv->len)
    ern = CONF_SC_TYPE_RET_ERR_PARSE;
  
  return (ern);
}

int conf_sc_token_app_vstr(const Conf_parse *conf, Conf_token *token,
                           Vstr_base *s1,
                           const Vstr_base **a_s1, size_t *a_pos, size_t *a_len)
{
  unsigned int num = conf_token_list_num(token, token->depth_num);
  int ern = CONF_SC_TYPE_RET_OK;
  const Vstr_sect_node *pv = NULL;
  
  ASSERT(s1);

  if (!num)
    return (CONF_SC_TYPE_RET_ERR_NOT_EXIST);
  conf_parse_token(conf, token);
  pv = conf_token_value(token);
  
  if (vstr_add_vstr(s1, s1->len, conf->data, pv->pos, pv->len,
                    VSTR_TYPE_SUB_BUF_REF))
  {
    *a_s1  = s1;
    *a_pos = (s1->len - pv->len) + 1;
    *a_len = pv->len;
  }
  
  return (ern);
}

int conf_sc_token_sub_vstr(const Conf_parse *conf, Conf_token *token,
                           Vstr_base *s1, size_t pos, size_t len)
{
  unsigned int num = conf_token_list_num(token, token->depth_num);
  int ern = CONF_SC_TYPE_RET_OK;
  const Vstr_sect_node *pv = NULL;
  
  ASSERT(s1);

  if (!num)
    return (CONF_SC_TYPE_RET_ERR_NOT_EXIST);
  conf_parse_token(conf, token);
  pv = conf_token_value(token);
  vstr_sub_vstr(s1, pos, len, conf->data, pv->pos, pv->len,
                VSTR_TYPE_SUB_BUF_REF);
  
  return (ern);
}

#define SUB2(x, y, z) vstr_sub_cstr_buf(x, y, 2, z)
int conf_sc_conv_unesc(Vstr_base *s1, size_t pos, size_t len,
                       size_t *ret_len)
{
  size_t dummy_len;

  if (!ret_len) ret_len = &dummy_len;
  
  *ret_len = len;
  
  while (!s1->conf->malloc_bad && (len > 1))
  {
    size_t plen = vstr_cspn_cstr_chrs_fwd(s1, pos, len, "\\");

    if (!plen)
    {
      char chr = vstr_export_chr(s1, pos + 1);

      if (chr == '\n')
      {
        vstr_del(s1, pos, 2);
        len -= 2; *ret_len -= 2;
        continue;
      }
      else if (chr == 't') { SUB2(s1, pos, "\t"); --len; --*ret_len; }
      else if (chr == 'v') { SUB2(s1, pos, "\v"); --len; --*ret_len; }
      else if (chr == 'r') { SUB2(s1, pos, "\r"); --len; --*ret_len; }
      else if (chr == 'n') { SUB2(s1, pos, "\n"); --len; --*ret_len; }
      else if (chr == 'b') { SUB2(s1, pos, "\b"); --len; --*ret_len; }
      else if (chr == '0')
      { /* \0 == NIL \0x0 == NIL */
        unsigned char val = 0;
        unsigned int nflags = VSTR_FLAG02(PARSE_NUM, OVERFLOW, SEP);

        if (chr != '0') nflags |= 8;

        /* FIXME: ... parse uchar */
        val = vstr_parse_ushort(s1, pos + 1, len - 1, nflags, &plen, NULL);
        vstr_sub_rep_chr(s1, pos, plen + 1, val, 1);
        len      -= plen; /* byte == \ ... so not plen + 1 */
        *ret_len -= plen;
      }
      else
      { /* rm \ ... Eg. \" == " and \\ == \ */
        vstr_del(s1, pos, 1);
        --len; --*ret_len;
      }
      plen = 1;
    }
    
    len -= plen;
    pos += plen;
  }

  return (!s1->conf->malloc_bad);
}
#undef SUB

void conf_parse_backtrace(Vstr_base *out, const char *filename,
                          const Conf_parse *conf, const Conf_token *token)
{
  const Vstr_sect_node *val = NULL;

  if (!out)
    return;
  
  if (conf->state != CONF_PARSE_STATE_END)
  {
    vstr_add_fmt(out, out->len, "Syntax error in %s.\n", filename);
    return;
  }

  if (!(val = conf_token_value(token)))
    vstr_add_fmt(out, out->len, "Failed parse on node %u [%s]\n",
                 token->num, conf_token_name(token));
  else
  {
    Vstr_base *s1 = conf->data;
    
    vstr_add_fmt(out, out->len, "Failed parse on node %u <%s>: ",
                 token->num, conf_token_name(token));
    vstr_add_vstr(out, out->len, s1, val->pos, val->len, VSTR_TYPE_ADD_BUF_REF);
    vstr_add_cstr_buf(out, out->len, "\n");
  }
}
