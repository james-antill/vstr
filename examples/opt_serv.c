#include "opt_serv.h"

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

static int opt_serv__conf_d1(struct Opt_serv_opts *opts,
                             const Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  if (!conf_token_list_num(token, token->depth_num))
    return (FALSE);
  conf_parse_token(conf, token);

  if (0){ }
  else if (OPT_SERV_SYM_EQ("chroot"))
    OPT_SERV_X_VSTR(opts->chroot_dir);
  else if (OPT_SERV_SYM_EQ("cntl-file") ||
           OPT_SERV_SYM_EQ("control-file"))
    OPT_SERV_X_VSTR(opts->cntl_file);
  else if (OPT_SERV_SYM_EQ("daemonize"))
    OPT_SERV_X_TOGGLE(opts->become_daemon);
  else if (OPT_SERV_SYM_EQ("drop-privs"))
  {
    int val = opts->drop_privs;
    int ern = conf_sc_token_parse_toggle(conf, token, &val);
    unsigned int num = conf_token_list_num(token, token->depth_num);
    
    if (ern == CONF_SC_TYPE_RET_ERR_NO_MATCH)
      return (FALSE);
    if (!val && num)
      return (FALSE);
    if ((num != 3) && (num != 6))
      return (FALSE);

    opts->drop_privs = val;
    while (num)
    {
      conf_parse_token(conf, token);
      if (token->type != CONF_TOKEN_TYPE_CLIST)
        return (FALSE);
      
      conf_parse_token(conf, token);
      if (0) { }
      else if (OPT_SERV_SYM_EQ("uid")) OPT_SERV_X_UINT(opts->priv_uid);
      else if (OPT_SERV_SYM_EQ("gid")) OPT_SERV_X_UINT(opts->priv_gid);
      else
        return (FALSE);
      
      num -= 3;
    }
    if (num)
      return (FALSE);
  }
  else if (OPT_SERV_SYM_EQ("idle-timeout"))
    OPT_SERV_X_UINT(opts->idle_timeout);
  else if (OPT_SERV_SYM_EQ("listen"))
  {
    unsigned int num = conf_token_list_num(token, token->depth_num);

    if ((num != 3) && (num != 6) && (num != 9) && (num != 12) && (num != 15))
      return (FALSE);
    
    while (num)
    {
      conf_parse_token(conf, token);
      if (token->type != CONF_TOKEN_TYPE_CLIST)
        return (FALSE);

      conf_parse_token(conf, token);
      if (0) { }
      else if (OPT_SERV_SYM_EQ("defer-accept"))
        OPT_SERV_X_UINT(opts->defer_accept);
      else if (OPT_SERV_SYM_EQ("port"))
        OPT_SERV_X_UINT(opts->tcp_port);
      else if (OPT_SERV_SYM_EQ("address") ||
               OPT_SERV_SYM_EQ("addr"))
        OPT_SERV_X_VSTR(opts->ipv4_address);
      else if (OPT_SERV_SYM_EQ("queue-length"))
        OPT_SERV_X_UINT(opts->q_listen_len);
      else if (OPT_SERV_SYM_EQ("filter"))
        OPT_SERV_X_VSTR(opts->acpt_filter_file);
      else
        return (FALSE);
      
      num -= 3;
    }

    if (num)
      return (FALSE);
  }
  else if (OPT_SERV_SYM_EQ("pid-file"))
    OPT_SERV_X_VSTR(opts->pid_file);
  else if (OPT_SERV_SYM_EQ("processes") ||
           OPT_SERV_SYM_EQ("procs"))
    OPT_SERV_X_UINT(opts->num_procs);
  else
    return (FALSE);
  
  return (TRUE);
}

int opt_serv_conf(struct Opt_serv_opts *opts,
                  const Conf_parse *conf, Conf_token *token)
{
  unsigned int cur_depth = token->depth_num;
  
  ASSERT(opts && conf && token);

  if (!OPT_SERV_SYM_EQ("org.and.daemon-conf-1.0"))
    return (FALSE);
  
  while (conf_token_list_num(token, cur_depth))
  {
    conf_parse_token(conf, token);
    if (!opt_serv__conf_d1(opts, conf, token))
      return (FALSE);
  }
  
  return (TRUE);
}

int opt_serv_conf_parse_cstr(Vstr_base *out,
                             Opt_serv_opts *opts, const char *data)
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
  
    if (!opt_serv__conf_d1(opts, conf, token))
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

int opt_serv_conf_parse_file(Vstr_base *out,
                             Opt_serv_opts *opts, const char *fname)
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
    
    if (!OPT_SERV_SYM_EQ("org.and.daemon-conf-1.0"))
      goto conf_fail;
  
    if (!opt_serv_conf(opts, conf, token))
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

int opt_serv_conf_init(struct Opt_serv_opts *opts)
{
  if (!(opts->pid_file         = vstr_make_base(NULL)) ||
      !(opts->cntl_file        = vstr_make_base(NULL)) ||
      !(opts->chroot_dir       = vstr_make_base(NULL)) ||
      !(opts->acpt_filter_file = vstr_make_base(NULL)) ||
      !(opts->vpriv_uid        = vstr_make_base(NULL)) ||
      !(opts->vpriv_gid        = vstr_make_base(NULL)) ||
      !(opts->ipv4_address     = vstr_make_base(NULL)) ||
      FALSE)
    return (FALSE);

  return (TRUE);
}

void opt_serv_conf_free(struct Opt_serv_opts *opts)
{
  if (!opts)
    return;
  
  vstr_free_base(opts->pid_file);         opts->pid_file         = NULL;
  vstr_free_base(opts->cntl_file);        opts->cntl_file        = NULL;
  vstr_free_base(opts->chroot_dir);       opts->chroot_dir       = NULL;
  vstr_free_base(opts->acpt_filter_file); opts->acpt_filter_file = NULL;
  vstr_free_base(opts->vpriv_uid);        opts->vpriv_uid        = NULL;
  vstr_free_base(opts->vpriv_gid);        opts->vpriv_gid        = NULL;
  vstr_free_base(opts->ipv4_address);     opts->ipv4_address     = NULL;
}

