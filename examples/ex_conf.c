
#include <vstr.h>
#include "conf.h"

#include "ex_utils.h"

int main(int argc, char *argv[])
{
  Vstr_base *s1 = NULL;
  Vstr_base *out = NULL;
  Conf_parse conf[1] = {CONF_PARSE_INIT};
  Vstr_sect_node *token = NULL;
  unsigned int num = 0;
  
  if (argc != 2)
    errx(EXIT_FAILURE, "args");

  if (!vstr_init())
    errx(EXIT_FAILURE, "vstr_init");

  if (!(s1 = vstr_make_base(NULL)))
    errx(EXIT_FAILURE, "vstr_make");

  if (!(out = vstr_make_base(NULL)))
    errx(EXIT_FAILURE, "vstr_make");

  if (!(conf->sects = vstr_sects_make(2)))
    errx(EXIT_FAILURE, "sects_make");
  
  if (!vstr_sc_read_len_file(s1, 0, argv[1], 0, 0, NULL))
    errx(EXIT_FAILURE, "read(%s)", argv[1]);

  conf->data = s1;
  if (!conf_parse_tokenize(conf))
    errx(EXIT_FAILURE, "conf_parse(%s)", argv[1]);
  
  {
    unsigned int inds[CONF_PARSE_LIST_DEPTH_SZ];
    unsigned int ind_off = 0;
    
    while ((token = conf_parse_token(conf, &num)))
    {
      unsigned int type = conf_parse_type(s1, token->pos, token->len);

      vstr_add_rep_chr(out, out->len, ' ', ind_off << 1);
      vstr_add_fmt(out, out->len, "%s\n", conf_parse_token_type2name(type));
      if ((type == CONF_PARSE_TYPE_CLIST) || (type == CONF_PARSE_TYPE_SLIST))
      {
        if (token->len)
          inds[ind_off++] = token->len;
      }
      else if (type != CONF_PARSE_TYPE_ERR)
      {
        vstr_add_rep_chr(out, out->len, ' ', (ind_off << 1));
        vstr_add_cstr_buf(out, out->len, "= ");
        vstr_add_vstr(out, out->len, s1, token->pos, token->len, 0);
        vstr_add_cstr_buf(out, out->len, "\n");
      }
      
      if (ind_off)
      {
        if (!--inds[ind_off - 1])
          --ind_off;
      }
    }
  }

  if (out->conf->malloc_bad)
    errno = ENOMEM, err(EXIT_FAILURE, "print");
  
  io_put_all(out, STDOUT_FILENO);
  
  vstr_sects_free(conf->sects);
  vstr_free_base(s1);
  vstr_free_base(out);
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
