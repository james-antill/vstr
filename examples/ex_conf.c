
/* this parses and dumps a conf.c configuration ... */

#define EX_UTILS_NO_USE_INPUT 1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_OPEN  1
#include "ex_utils.h"

#include "conf.h"

int main(int argc, char *argv[])
{
  Vstr_base *out = NULL;
  Vstr_base *s1 = ex_init(&out);
  Conf_parse conf[1]  = {CONF_PARSE_INIT};
  Conf_token token[1] = {CONF_TOKEN_INIT};
  
  if (argc != 2)
    errx(EXIT_FAILURE, "args");

  if (!(conf->sects = vstr_sects_make(2)))
    errx(EXIT_FAILURE, "sects_make");
  
  if (!vstr_sc_read_len_file(s1, 0, argv[1], 0, 0, NULL))
    errx(EXIT_FAILURE, "read(%s)", argv[1]);

  conf->data = s1;
  if (!conf_parse_lex(conf))
    errx(EXIT_FAILURE, "conf_parse(%s)", argv[1]);
  
  while (conf_parse_token(conf, token))
  {
    const Vstr_sect_node *val = NULL;
    
    if (!(val = conf_token_value(token)))
    {
      vstr_add_rep_chr(out, out->len, ' ', (token->depth_num - 1) << 1);
      vstr_add_fmt(out, out->len, "[%s]\n", conf_token_name(token));
    }
    else
    {
      vstr_add_rep_chr(out, out->len, ' ', token->depth_num << 1);
      vstr_add_fmt(out, out->len, "<%s>\n", conf_token_name(token));
    
      vstr_add_rep_chr(out, out->len, ' ', (token->depth_num << 1) + 1);
      vstr_add_vstr(out, out->len, s1, val->pos, val->len, 0);
      vstr_add_cstr_buf(out, out->len, "\n");
    }
  }

  if (out->conf->malloc_bad)
    errno = ENOMEM, err(EXIT_FAILURE, "print");
  
  io_put_all(out, STDOUT_FILENO);
  
  vstr_sects_free(conf->sects);
  
  exit (ex_exit(s1, out));
}
