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

#define CONF_PARSE_LIST_DEPTH_SZ 256
#define CONF_PARSE_INIT {NULL, NULL, 0, CONF_PARSE_STATE_BEG, 0}

#define CONF_PARSE_TYPE_ERR              0
#define CONF_PARSE_TYPE_CLIST            1
#define CONF_PARSE_TYPE_SLIST            2
#define CONF_PARSE_TYPE_QUOTE_DDD        3
#define CONF_PARSE_TYPE_QUOTE_D          4
#define CONF_PARSE_TYPE_QUOTE_SSS        5
#define CONF_PARSE_TYPE_QUOTE_S          6
#define CONF_PARSE_TYPE_SYMBOL           7


typedef struct Conf_parse
{
 Vstr_sects *sects;
 Vstr_base *data;
 size_t parsed;
 unsigned int state;
 unsigned int depth;
} Conf_parse;

extern int conf_parse_tokenize(Conf_parse *);
extern Vstr_sect_node *conf_parse_token(Conf_parse *, unsigned int *);
extern unsigned int conf_parse_type(Vstr_base *, size_t, size_t);
extern const char *conf_parse_token_type2name(unsigned int);


#endif
