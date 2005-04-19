#include "opt_serv.h"

#define EX_UTILS_NO_FUNCS 1
#include "ex_utils.h"

#include "conf.h"

static int opt_serv__conf_d1(struct Opt_serv_opts *opts,
                             const Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

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
    unsigned int depth = token->depth_num;
    int val = opts->drop_privs;
    int ern = conf_sc_token_parse_toggle(conf, token, &val);
    unsigned int num = conf_token_list_num(token, token->depth_num);
    
    if (ern == CONF_SC_TYPE_RET_ERR_NO_MATCH)
      return (FALSE);
    if (!val && num)
      return (FALSE);

    opts->drop_privs = val;
    while (conf_token_list_num(token, depth))
    {
      CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
      CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

      if (0) { }
      else if (OPT_SERV_SYM_EQ("uid"))       OPT_SERV_X_UINT(opts->priv_uid);
      else if (OPT_SERV_SYM_EQ("usrname"))   OPT_SERV_X_VSTR(opts->vpriv_uid);
      else if (OPT_SERV_SYM_EQ("username"))  OPT_SERV_X_VSTR(opts->vpriv_uid);
      else if (OPT_SERV_SYM_EQ("gid"))       OPT_SERV_X_UINT(opts->priv_gid);
      else if (OPT_SERV_SYM_EQ("grpname"))   OPT_SERV_X_VSTR(opts->vpriv_gid);
      else if (OPT_SERV_SYM_EQ("groupname")) OPT_SERV_X_VSTR(opts->vpriv_gid);
      else
        return (FALSE);
    }
  }
  else if (OPT_SERV_SYM_EQ("idle-timeout")) /* FIXME: policy */
    OPT_SERV_X_UINT(opts->idle_timeout);
  else if (OPT_SERV_SYM_EQ("listen"))
  {
    unsigned int depth = token->depth_num;
    while (conf_token_list_num(token, depth))
    {
      CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
      CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

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
    }
  }
  else if (OPT_SERV_SYM_EQ("parent-death-signal"))
    OPT_SERV_X_TOGGLE(opts->use_pdeathsig);
  else if (OPT_SERV_SYM_EQ("pid-file"))
    OPT_SERV_X_VSTR(opts->pid_file);
  else if (OPT_SERV_SYM_EQ("processes") ||
           OPT_SERV_SYM_EQ("procs"))
  {
    unsigned int opt__val = 0;
    unsigned int ret = conf_sc_token_parse_uint(conf, token, &opt__val);
    
    if (ret == CONF_SC_TYPE_RET_ERR_PARSE)
    {
      const Vstr_sect_node *pv = NULL;
      long sc_val = -1;
      
      if (!(pv = conf_token_value(token)))
        return (FALSE);
      if (0) { }
      else if (vstr_cmp_cstr_eq(conf->data, pv->pos, pv->len,
                                "<sysconf-number-processors-configured>") ||
               vstr_cmp_cstr_eq(conf->data, pv->pos, pv->len,
                                "<sysconf-num-procs-configured>") ||
               vstr_cmp_cstr_eq(conf->data, pv->pos, pv->len,
                                "<sysconf-num-procs-conf>"))
        sc_val = sysconf(_SC_NPROCESSORS_CONF);
      else if (vstr_cmp_cstr_eq(conf->data, pv->pos, pv->len,
                                "<sysconf-number-processors-online>") ||
               vstr_cmp_cstr_eq(conf->data, pv->pos, pv->len,
                                "<sysconf-num-procs-online>") ||
               vstr_cmp_cstr_eq(conf->data, pv->pos, pv->len,
                                "<sysconf-num-procs-onln>"))
        sc_val = sysconf(_SC_NPROCESSORS_ONLN);
      else      
        return (FALSE);
      if (sc_val == -1)
        sc_val = 1;
      opt__val = sc_val;  
    }
    else if (ret)
      return (FALSE);
    
    opts->num_procs = opt__val;
  }
  else if (OPT_SERV_SYM_EQ("rlimit"))
  {
    unsigned int depth = token->depth_num;
    while (conf_token_list_num(token, depth))
    {
      CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
      CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

      if (0) { }
      else if (OPT_SERV_SYM_EQ("file-num"))
        OPT_SERV_X_UINT(opts->rlim_file_num);
      else
        return (FALSE);
    }
  }
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
    if (conf_token_at_depth(token) != cur_depth)
      return (FALSE);
    if (!opt_serv__conf_d1(opts, conf, token))
      return (FALSE);
  }
  
  /* And they all live together ... dum dum */
  if (conf->data->conf->malloc_bad)
    return (FALSE);

  return (TRUE);
}

int opt_serv_conf_parse_cstr(Vstr_base *out,
                             Opt_serv_opts *opts, const char *data)
{
  Conf_parse *conf    = conf_parse_make(NULL);
  Conf_token token[1] = {CONF_TOKEN_INIT};

  ASSERT(opts && data);
  
  if (!conf)
    goto conf_malloc_fail;
  
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len, data))
    goto read_malloc_fail;

  if (!conf_parse_lex(conf))
    goto conf_fail;

  while (conf_parse_token(conf, token))
  {
    if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
      goto conf_fail;
  
    if (!opt_serv__conf_d1(opts, conf, token))
      goto conf_fail;
  }

  conf_parse_free(conf);
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, data, conf, token);
 read_malloc_fail:
  conf_parse_free(conf);
 conf_malloc_fail:
  return (FALSE);
}

int opt_serv_conf_parse_file(Vstr_base *out,
                             Opt_serv_opts *opts, const char *fname)
{
  Conf_parse *conf    = conf_parse_make(NULL);
  Conf_token token[1] = {CONF_TOKEN_INIT};

  ASSERT(opts && fname);
  
  if (!conf)
    goto conf_malloc_fail;
  
  if (!vstr_sc_read_len_file(conf->data, 0, fname, 0, 0, NULL))
    goto read_malloc_fail;

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

  conf_parse_free(conf);
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, fname, conf, token);
 read_malloc_fail:
  conf_parse_free(conf);
 conf_malloc_fail:
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

