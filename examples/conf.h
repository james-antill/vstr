#ifndef CONF_H
#define CONF_H

#include <vstr.h>

#define CONF_PARSE_ERR 0
#define CONF_PARSE_FIN 1

#define CONF_PARSE_STATE_BEG             0
#define CONF_PARSE_STATE_WS              1
#define CONF_PARSE_STATE_LIST_END_OR_WS  2
#define CONF_PARSE_STATE_CHOOSE          3
#define CONF_PARSE_STATE_QUOTE_D_BEG     4
#define CONF_PARSE_STATE_QUOTE_S_BEG     5
#define CONF_PARSE_STATE_QUOTE_D_END     6
#define CONF_PARSE_STATE_QUOTE_DDD_END   7
#define CONF_PARSE_STATE_QUOTE_S_END     8
#define CONF_PARSE_STATE_QUOTE_SSS_END   9
#define CONF_PARSE_STATE_SYMBOL_END     10
#define CONF_PARSE_STATE_END            11

#define CONF_PARSE_LIST_DEPTH_SZ 64

#define CONF_TOKEN_TYPE_ERR              0
#define CONF_TOKEN_TYPE_CLIST            1
#define CONF_TOKEN_TYPE_SLIST            2
#define CONF_TOKEN_TYPE_QUOTE_DDD        3
#define CONF_TOKEN_TYPE_QUOTE_D          4
#define CONF_TOKEN_TYPE_QUOTE_SSS        5
#define CONF_TOKEN_TYPE_QUOTE_S          6
#define CONF_TOKEN_TYPE_SYMBOL           7

#define CONF_SC_TYPE_RET_OK 0
#define CONF_SC_TYPE_RET_ERR_TOO_MANY  1
#define CONF_SC_TYPE_RET_ERR_NO_MATCH  2
#define CONF_SC_TYPE_RET_ERR_NOT_EXIST 3
#define CONF_SC_TYPE_RET_ERR_PARSE     4

typedef struct Conf_parse
{
 Vstr_sects *sects;
 Vstr_base *data;
 size_t parsed;
 unsigned int state;
 unsigned int depth;
} Conf_parse;
#define CONF_PARSE_INIT {NULL, NULL, 0, CONF_PARSE_STATE_BEG, 0}

typedef struct Conf_token
{
 Vstr_sect_node node[1];
 unsigned int type;
 unsigned int num;
 unsigned int depth_num;
 unsigned int depth_nums[CONF_PARSE_LIST_DEPTH_SZ];
} Conf_token;
#define CONF_TOKEN_INIT {{{0,0}}, CONF_TOKEN_TYPE_ERR, 0, 0, {0}}

extern int conf_parse_lex(Conf_parse *);

extern int conf_parse_token(const Conf_parse *, Conf_token *);

extern void conf_token_init(Conf_token *);
extern const char *conf_token_name(const Conf_token *);
extern const Vstr_sect_node *conf_token_value(const Conf_token *);

extern int conf_token_cmp_val_cstr_eq(const Conf_parse *, const Conf_token *,
                                      const char *);
extern int conf_token_cmp_sym_cstr_eq(const Conf_parse *, const Conf_token *,
                                      const char *);
extern int conf_token_cmp_str_cstr_eq(const Conf_parse *, const Conf_token *,
                                      const char *);
extern int conf_token_cmp_case_val_cstr_eq(const Conf_parse *,
                                           const Conf_token *,
                                           const char *);
extern int conf_token_cmp_case_sym_cstr_eq(const Conf_parse *,
                                           const Conf_token *,
                                           const char *);
extern int conf_token_cmp_case_str_cstr_eq(const Conf_parse *,
                                           const Conf_token *,
                                           const char *);

extern unsigned int conf_token_list_num(const Conf_token *, unsigned int);

extern int conf_sc_token_parse_toggle(const Conf_parse *, Conf_token *, int *);
extern int conf_sc_token_parse_uint(const Conf_parse *, Conf_token *,
                                    unsigned int *);
extern int conf_sc_token_app_vstr(const Conf_parse *, Conf_token *,
                                  Vstr_base *,
                                  const Vstr_base **, size_t *, size_t *);
extern int conf_sc_token_sub_vstr(const Conf_parse *, Conf_token *,
                                  Vstr_base *, size_t, size_t);
extern int conf_sc_conv_unesc(Vstr_base *, size_t, size_t, size_t *);

extern void conf_parse_backtrace(Vstr_base *, const char *,
                                 const Conf_parse *, const Conf_token *);

#if !defined(CONF_COMPILE_INLINE)
# ifdef VSTR_AUTOCONF_NDEBUG
#  define CONF_COMPILE_INLINE 1
# else
#  define CONF_COMPILE_INLINE 0
# endif
#endif

#if defined(VSTR_AUTOCONF_HAVE_INLINE) && CONF_COMPILE_INLINE
extern inline const Vstr_sect_node *conf_token_value(const Conf_token *token)
{
  if ((token->type >= CONF_TOKEN_TYPE_QUOTE_DDD) &&
      (token->type <= CONF_TOKEN_TYPE_SYMBOL))
    return (token->node);
  
  return (NULL);
}

extern inline int conf_token_cmp_val_cstr_eq(const Conf_parse *conf,
                                             const Conf_token *token,
                                             const char *cstr)
{
  const Vstr_sect_node *val = conf_token_value(token);
  
  if (!val) return (0);
  
  return (vstr_cmp_cstr_eq(conf->data, val->pos, val->len, cstr));
}

extern inline int conf_token_cmp_sym_cstr_eq(const Conf_parse *conf,
                                             const Conf_token *token,
                                             const char *cstr)
{
  if (token->type == CONF_TOKEN_TYPE_SYMBOL)
    return (conf_token_cmp_val_cstr_eq(conf, token, cstr));
  return (0);
}

extern inline int conf_token_cmp_str_cstr_eq(const Conf_parse *conf,
                                             const Conf_token *token,
                                             const char *cstr)
{
  if ((token->type >= CONF_TOKEN_TYPE_QUOTE_DDD) &&
      (token->type <= CONF_TOKEN_TYPE_QUOTE_S))
    return (conf_token_cmp_val_cstr_eq(conf, token, cstr));
  
  return (0);
}

extern inline int conf_token_cmp_case_val_cstr_eq(const Conf_parse *conf,
                                                  const Conf_token *token,
                                                  const char *cstr)
{
  const Vstr_sect_node *val = conf_token_value(token);
  
  if (!val) return (0);
  
  return (vstr_cmp_case_cstr_eq(conf->data, val->pos, val->len, cstr));
}

extern inline int conf_token_cmp_case_sym_cstr_eq(const Conf_parse *conf,
                                                  const Conf_token *token,
                                                  const char *cstr)
{
  if (token->type == CONF_TOKEN_TYPE_SYMBOL)
    return (conf_token_cmp_case_val_cstr_eq(conf, token, cstr));
  return (0);
}

extern inline int conf_token_cmp_case_str_cstr_eq(const Conf_parse *conf,
                                                  const Conf_token *token,
                                                  const char *cstr)
{
  if ((token->type >= CONF_TOKEN_TYPE_QUOTE_DDD) &&
      (token->type <= CONF_TOKEN_TYPE_QUOTE_S))
    return (conf_token_cmp_case_val_cstr_eq(conf, token, cstr));
  
  return (0);
}

extern inline unsigned int conf_token_list_num(const Conf_token *token,
                                               unsigned int depth)
{
  if (!depth || (depth > token->depth_num))
    return (0);

  return (token->depth_nums[depth - 1] - token->num);
}

extern inline int conf_sc_token_parse_toggle(const Conf_parse *conf,
                                             const Conf_token *token, int *val)
{
  unsigned int num = conf_token_list_num(token, token->depth_num);
  int ern = CONF_SC_TYPE_RET_OK;
  
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

#endif


#endif
