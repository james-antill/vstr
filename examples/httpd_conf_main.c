
#include "httpd_conf_main.h"

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

static int httpd__conf_main_d1(Httpd_opts *opts,
                               const Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  if (!conf_token_list_num(token, token->depth_num))
    return (FALSE);
  conf_parse_token(conf, token);

  if (OPT_SERV_SYM_EQ("org.and.daemon-conf-1.0"))
  {
    if (!opt_serv_conf(opts->s, conf, token))
      return (FALSE);
  }
  else if (OPT_SERV_SYM_EQ("directory-filename"))
    OPT_SERV_X_VSTR(opts->dir_filename);
  else if (OPT_SERV_SYM_EQ("document-root") ||
           OPT_SERV_SYM_EQ("doc-root"))
    OPT_SERV_X_VSTR(opts->document_root);
  else if (OPT_SERV_SYM_EQ("unspecified-hostname"))
    OPT_SERV_X_VSTR(opts->default_hostname);
  else if (OPT_SERV_SYM_EQ("MIME/types-default-type"))
    OPT_SERV_X_VSTR(opts->mime_types_default_type);
  else if (OPT_SERV_SYM_EQ("MIME/types-filename-main"))
    OPT_SERV_X_VSTR(opts->mime_types_main);
  else if (OPT_SERV_SYM_EQ("MIME/types-filename-extra") ||
           OPT_SERV_SYM_EQ("MIME/types-filename-xtra"))
    OPT_SERV_X_VSTR(opts->mime_types_xtra);
  else if (OPT_SERV_SYM_EQ("request-configuration-directory") ||
           OPT_SERV_SYM_EQ("req-conf-dir"))
    OPT_SERV_X_VSTR(opts->req_configuration_dir);
  else if (OPT_SERV_SYM_EQ("server-name"))
    OPT_SERV_X_VSTR(opts->server_name);

  else if (OPT_SERV_SYM_EQ("mmap"))
    OPT_SERV_X_TOGGLE(opts->use_mmap);
  else if (OPT_SERV_SYM_EQ("sendfile"))
    OPT_SERV_X_TOGGLE(opts->use_sendfile);
  else if (OPT_SERV_SYM_EQ("keep-alive"))
    OPT_SERV_X_TOGGLE(opts->use_keep_alive);
  else if (OPT_SERV_SYM_EQ("keep-alive-1.0"))
    OPT_SERV_X_TOGGLE(opts->use_keep_alive_1_0);
  else if (OPT_SERV_SYM_EQ("virtual-hosts") ||
           OPT_SERV_SYM_EQ("vhosts"))
    OPT_SERV_X_TOGGLE(opts->use_vhosts);
  else if (OPT_SERV_SYM_EQ("range"))
    OPT_SERV_X_TOGGLE(opts->use_range);
  else if (OPT_SERV_SYM_EQ("range-1.0"))
    OPT_SERV_X_TOGGLE(opts->use_range_1_0);
  else if (OPT_SERV_SYM_EQ("public-only"))
    OPT_SERV_X_TOGGLE(opts->use_public_only);
  else if (OPT_SERV_SYM_EQ("gzip-content-type-replacement"))
    OPT_SERV_X_TOGGLE(opts->use_gzip_content_replacement);
  else if (OPT_SERV_SYM_EQ("require-content-type"))
    OPT_SERV_X_TOGGLE(opts->use_default_mime_type);
  else if (OPT_SERV_SYM_EQ("error-406"))
    OPT_SERV_X_TOGGLE(opts->use_err_406);
  else if (OPT_SERV_SYM_EQ("canonize-host"))
    OPT_SERV_X_TOGGLE(opts->use_canonize_host);
  else if (OPT_SERV_SYM_EQ("error-host-400"))
    OPT_SERV_X_TOGGLE(opts->use_host_err_400);

  else if (OPT_SERV_SYM_EQ("max-header-size") ||
           OPT_SERV_SYM_EQ("max-header-sz"))
    OPT_SERV_X_UINT(opts->max_header_sz);
  
  else
    return (FALSE);
  
  return (TRUE);
}

static int httpd__conf_main(Httpd_opts *opts,
                            const Conf_parse *conf, Conf_token *token)
{
  unsigned int cur_depth = token->depth_num;
  
  if (!OPT_SERV_SYM_EQ("org.and.jhttpd-conf-main-1.0"))
    return (FALSE);
  
  while (conf_token_list_num(token, cur_depth))
  {
    conf_parse_token(conf, token);
    if (!httpd__conf_main_d1(opts, conf, token))
      return (FALSE);
  }
  
  return (TRUE);
}

int httpd_conf_main_parse_cstr(Vstr_base *out,
                               Httpd_opts *opts, const char *data)
{
  Conf_parse conf[1]  = {CONF_PARSE_INIT};
  Conf_token token[1] = {CONF_TOKEN_INIT};

  ASSERT(opts && data);
  
  if (!(conf->data = vstr_make_base(NULL)))
    goto base_malloc_fail;
  
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len, data))
    goto read_malloc_fail;

  if (!(conf->sects = vstr_sects_make(2)))
    goto sects_malloc_fail;
  
  if (!conf_parse_lex(conf))
    goto conf_fail;

  while (conf_parse_token(conf, token))
  {
    if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
      goto conf_fail;

    if (!httpd__conf_main_d1(opts, conf, token))
      goto conf_fail;
  }

  vstr_sects_free(conf->sects);
  vstr_free_base(conf->data);
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, data, conf, token);
 sects_malloc_fail:
  vstr_sects_free(conf->sects);
 read_malloc_fail:
  vstr_free_base(conf->data);
 base_malloc_fail:
  return (FALSE);
}

int httpd_conf_main_parse_file(Vstr_base *out,
                               Httpd_opts *opts, const char *fname)
{
  Conf_parse conf[1]  = {CONF_PARSE_INIT};
  Conf_token token[1] = {CONF_TOKEN_INIT};

  ASSERT(opts && fname);
  
  if (!(conf->data = vstr_make_base(NULL)))
    goto base_malloc_fail;
  
  if (!vstr_sc_read_len_file(conf->data, 0, fname, 0, 0, NULL))
    goto read_malloc_fail;

  if (!(conf->sects = vstr_sects_make(2)))
    goto sects_malloc_fail;
  
  if (!conf_parse_lex(conf))
    goto conf_fail;

  while (conf_parse_token(conf, token))
  {
    if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
      goto conf_fail;

    if (!conf_parse_token(conf, token))
      goto conf_fail;
    
    if (!OPT_SERV_SYM_EQ("org.and.jhttpd-conf-main-1.0"))
      goto conf_fail;
  
    if (!httpd__conf_main(opts, conf, token))
      goto conf_fail;
  }

  vstr_sects_free(conf->sects);
  vstr_free_base(conf->data);
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, fname, conf, token);
 sects_malloc_fail:
  vstr_sects_free(conf->sects);
 read_malloc_fail:
  vstr_free_base(conf->data);
 base_malloc_fail:
  return (FALSE);
}


int httpd_conf_main_init(Httpd_opts *opts)
{
  if (!(opts->document_root           = vstr_make_base(NULL)) ||
      !(opts->server_name             = vstr_make_base(NULL)) ||
      !(opts->dir_filename            = vstr_make_base(NULL)) ||
      !(opts->mime_types_default_type = vstr_make_base(NULL)) ||
      !(opts->mime_types_main         = vstr_make_base(NULL)) ||
      !(opts->mime_types_xtra         = vstr_make_base(NULL)) ||
      !(opts->default_hostname        = vstr_make_base(NULL)) ||
      !(opts->req_configuration_dir   = vstr_make_base(NULL)) ||
      FALSE)
    return (FALSE);

  vstr_add_cstr_ptr(opts->server_name,     0, HTTPD_CONF_DEF_NAME);
  vstr_add_cstr_ptr(opts->dir_filename,    0, HTTPD_CONF_DEF_DIR_FILENAME);
  vstr_add_cstr_ptr(opts->mime_types_default_type, 0, HTTPD_CONF_DEF_MIME_TYPE);
  vstr_add_cstr_ptr(opts->mime_types_main, 0, HTTPD_CONF_MIME_TYPE_MAIN);

  if (opts->document_root->conf->malloc_bad)
    return (FALSE);

  return (TRUE);
}

void httpd_conf_main_free(Httpd_opts *opts)
{
  if (!opts)
    return;
  
  vstr_free_base(opts->document_root);    opts->document_root           = NULL;
  vstr_free_base(opts->server_name);      opts->server_name             = NULL;
  vstr_free_base(opts->dir_filename);     opts->dir_filename            = NULL;
  vstr_free_base(opts->mime_types_default_type);
                                          opts->mime_types_default_type = NULL;
  vstr_free_base(opts->mime_types_main);  opts->mime_types_main         = NULL;
  vstr_free_base(opts->mime_types_xtra);  opts->mime_types_xtra         = NULL;
  vstr_free_base(opts->default_hostname); opts->default_hostname        = NULL;
  vstr_free_base(opts->req_configuration_dir);
                                          opts->req_configuration_dir   = NULL;
}
