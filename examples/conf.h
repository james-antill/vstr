#ifndef CONF_H
#define CONF_H

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

#define CONF_PARSE_LIST_DEPTH_SZ 64

#define CONF_TOKEN_TYPE_ERR              0
#define CONF_TOKEN_TYPE_CLIST            1
#define CONF_TOKEN_TYPE_SLIST            2
#define CONF_TOKEN_TYPE_QUOTE_DDD        3
#define CONF_TOKEN_TYPE_QUOTE_D          4
#define CONF_TOKEN_TYPE_QUOTE_SSS        5
#define CONF_TOKEN_TYPE_QUOTE_S          6
#define CONF_TOKEN_TYPE_SYMBOL           7


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

extern int conf_parse_token(Conf_parse *, Conf_token *);

extern void conf_token_init(Conf_token *);
extern const char *conf_token_name(const Conf_token *);
extern const Vstr_sect_node *conf_token_value(const Conf_token *);

extern int conf_token_cmp_val_cstr_eq(const Conf_parse *, const Conf_token *,
                                      const char *);
extern int conf_token_cmp_sym_cstr_eq(const Conf_parse *, const Conf_token *,
                                      const char *);
extern int conf_token_cmp_str_cstr_eq(const Conf_parse *, const Conf_token *,
                                      const char *);


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

#endif


#endif
