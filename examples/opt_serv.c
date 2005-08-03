#define _GNU_SOURCE 1 /* strsignal() */

#include "opt_serv.h"

#define EX_UTILS_NO_FUNCS 1
#include "ex_utils.h"

#include "opt_policy.h"

#include "mk.h"

#include <sys/resource.h>
#include <grp.h>
#include <signal.h>

/* need better way to test for this */
#ifndef __GLIBC__
# define strsignal(x) ""
#endif

static Vlg *vlg = NULL;

static int opt_serv__conf_main_policy_d1(Opt_serv_policy_opts *opts,
                                         const Conf_parse *conf,
                                         Conf_token *token, int clist)
{
  OPT_SERV_PRIME_SYM_EQ_DECL();
  unsigned int dummy;
  
  if (0) { }
  else if (OPT_SERV_SYM_EQ("timeout"))
  {
    CONF_SC_MAKE_CLIST_BEG(timeout, clist);
    
    else if (OPT_SERV_SYM_EQ("idle"))
      OPT_SERV_X_UINT(opts->idle_timeout);
    else if (OPT_SERV_SYM_EQ("total"))
      OPT_SERV_X_UINT(dummy);
  
    CONF_SC_MAKE_CLIST_END();
  }
  else if (OPT_SERV_SYM_EQ("limit"))
  {
    CONF_SC_MAKE_CLIST_BEG(limit, clist);
    
    else if (OPT_SERV_SYM_EQ("connections"))
      OPT_SERV_X_UINT(opts->max_connections);
    else if (OPT_SERV_SYM_EQ("io-r/s") ||
             OPT_SERV_SYM_EQ("io-read/s") ||
             OPT_SERV_SYM_EQ("io-recv/s"))
      OPT_SERV_X_UINT(dummy);
    else if (OPT_SERV_SYM_EQ("io-s/s") ||
             OPT_SERV_SYM_EQ("io-w/s") ||
             OPT_SERV_SYM_EQ("io-write/s") ||
             OPT_SERV_SYM_EQ("io-send/s"))
      OPT_SERV_X_UINT(dummy);
  
    CONF_SC_MAKE_CLIST_END();
  }
  else
    return (FALSE);
  
  return (TRUE);
}

static int opt_serv__conf_main_policy(Opt_serv_opts *opts,
                                      const Conf_parse *conf, Conf_token *token)
{
  Opt_serv_policy_opts *popts = NULL;
  unsigned int cur_depth = opt_policy_sc_conf_parse(opts, conf, token, &popts);
  int clist = FALSE;
  
  if (!cur_depth)
    return (FALSE);
  
  CONF_SC_MAKE_CLIST_MID(cur_depth, clist);
  
  else if (opt_serv__conf_main_policy_d1(popts, conf, token, clist))
  { }

  CONF_SC_MAKE_CLIST_END();
  
  return (TRUE);
}

#define OPT_SERV__RLIM_VAL(x, y) do {                                   \
      const Vstr_sect_node *pv = NULL;                                  \
      unsigned int val = 0;                                             \
      int ern = 0;                                                      \
                                                                        \
      (y) = TRUE;                                                       \
                                                                        \
      ern = conf_sc_token_parse_uint(conf, token, &val);                \
      if (ern && (ern == CONF_SC_TYPE_RET_ERR_PARSE) &&                 \
          !(pv = conf_token_value(token)))                              \
        return (FALSE);                                                 \
                                                                        \
      if (!pv)                                                          \
        (x) = val;                                                      \
      else                                                              \
      {                                                                 \
        if (0){ }                                                       \
        else if (OPT_SERV_SYM_EQ("<infinity>"))  (x) = RLIM_INFINITY;   \
        else if (OPT_SERV_SYM_EQ("<unlimited>")) (x) = RLIM_INFINITY;   \
        else if (OPT_SERV_SYM_EQ("<none>"))      (x) = 0;               \
        else if (OPT_SERV_SYM_EQ("<zero>"))      (x) = 0;               \
        else if (OPT_SERV_SYM_EQ("<default>"))   (y) = FALSE;           \
        else return (FALSE);                                            \
      }                                                                 \
    }                                                                   \
    while (FALSE)

static int opt_serv__conf_d1(struct Opt_serv_opts *opts,
                             const Conf_parse *conf, Conf_token *token,
                             int clist)
{
  OPT_SERV_PRIME_SYM_EQ_DECL();
  
  if (0){ }

  else if (OPT_SERV_SYM_EQ("policy"))
  {
    if (!opt_serv__conf_main_policy(opts, conf, token))
      return (FALSE);
  }
  
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
    CONF_SC_MAKE_CLIST_MID(depth, clist);

    else if (OPT_SERV_SYM_EQ("uid"))       OPT_SERV_X_UINT(opts->priv_uid);
    else if (OPT_SERV_SYM_EQ("usrname"))   OPT_SERV_X_VSTR(opts->vpriv_uid);
    else if (OPT_SERV_SYM_EQ("username"))  OPT_SERV_X_VSTR(opts->vpriv_uid);
    else if (OPT_SERV_SYM_EQ("gid"))       OPT_SERV_X_UINT(opts->priv_gid);
    else if (OPT_SERV_SYM_EQ("grpname"))   OPT_SERV_X_VSTR(opts->vpriv_gid);
    else if (OPT_SERV_SYM_EQ("groupname")) OPT_SERV_X_VSTR(opts->vpriv_gid);

    CONF_SC_MAKE_CLIST_END();
  }
  else if (OPT_SERV_SYM_EQ("listen"))
  {
    Opt_serv_addr_opts *addr = opt_serv_make_addr(opts);
    CONF_SC_MAKE_CLIST_BEG(listen, clist);

    else if (!addr) return (FALSE);
    
    else if (OPT_SERV_SYM_EQ("defer-accept"))
      OPT_SERV_X_UINT(addr->defer_accept);
    else if (OPT_SERV_SYM_EQ("port"))
      OPT_SERV_X_UINT(addr->tcp_port);
    else if (OPT_SERV_SYM_EQ("address") ||
             OPT_SERV_SYM_EQ("addr"))
      OPT_SERV_X_VSTR(addr->ipv4_address);
    else if (OPT_SERV_SYM_EQ("queue-length"))
      OPT_SERV_X_UINT(addr->q_listen_len);
    else if (OPT_SERV_SYM_EQ("filter"))
      OPT_SERV_X_VSTR(addr->acpt_filter_file);
    else if (OPT_SERV_SYM_EQ("max-connections"))
      OPT_SERV_X_UINT(addr->max_connections);

    CONF_SC_MAKE_CLIST_END();
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
  else if (OPT_SERV_SYM_EQ("resource-limits") ||
           OPT_SERV_SYM_EQ("rlimit"))
  {
    CONF_SC_MAKE_CLIST_BEG(rlimit, clist);

    else if (OPT_SERV_SYM_EQ("CORE") ||
             OPT_SERV_SYM_EQ("core"))
      OPT_SERV__RLIM_VAL(opts->rlim_core_num, opts->rlim_core_call);
    else if (OPT_SERV_SYM_EQ("NOFILE") ||
             OPT_SERV_SYM_EQ("file-descriptor-number") ||
             OPT_SERV_SYM_EQ("fd-num"))
      OPT_SERV__RLIM_VAL(opts->rlim_file_num, opts->rlim_file_call);
      
    CONF_SC_MAKE_CLIST_END();
  }
  /*  else if (OPT_SERV_SYM_EQ("priority"))
      OPT_SERV_X_UINT(opts->sys_priority); */
  else if (OPT_SERV_SYM_EQ("cache-limits"))
  {
    CONF_SC_MAKE_CLIST_BEG(cache_limits, clist);
    
    else if (OPT_SERV_SYM_EQ("spare-vstr-bases"))
      OPT_SERV_X_UINT(opts->max_spare_bases);
    else if (OPT_SERV_SYM_EQ("spare-vstr-nodes-buf"))
      OPT_SERV_X_UINT(opts->max_spare_buf_nodes);
    else if (OPT_SERV_SYM_EQ("spare-vstr-nodes-ptr"))
      OPT_SERV_X_UINT(opts->max_spare_ptr_nodes);
    else if (OPT_SERV_SYM_EQ("spare-vstr-nodes-ref"))
      OPT_SERV_X_UINT(opts->max_spare_ref_nodes);
    
    CONF_SC_MAKE_CLIST_END();
  }
  
  else
    return (FALSE);
  
  return (TRUE);
}
#undef OPT_SERV__RLIM_VAL

int opt_serv_conf(struct Opt_serv_opts *opts,
                  const Conf_parse *conf, Conf_token *token)
{
  unsigned int cur_depth = token->depth_num;
  int clist = FALSE;
  
  ASSERT(opts && conf && token);

  if (!conf_token_cmp_sym_cstr_eq(conf, token, "org.and.daemon-conf-1.0"))
    return (FALSE);
  
  CONF_SC_MAKE_CLIST_MID(cur_depth, clist);
  
  else if (opt_serv__conf_d1(opts, conf, token, clist))
  { }
  
  CONF_SC_MAKE_CLIST_END();
  
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
  
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len,
                         "(org.and.daemon-conf-1.0 "))
    goto read_malloc_fail;
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len, data))
    goto read_malloc_fail;
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len,
                         ")"))
    goto read_malloc_fail;

  if (!conf_parse_lex(conf, 1, conf->data->len))
    goto conf_fail;

  if (!conf_parse_token(conf, token))
    goto conf_fail;
    
  if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
    goto conf_fail;
    
  if (!conf_parse_token(conf, token))
    goto conf_fail;

  ASSERT(conf_token_cmp_sym_cstr_eq(conf, token, "org.and.daemon-conf-1.0"));
  
  if (!opt_serv_conf(opts, conf, token))
    goto conf_fail;
  if (token->num != conf->sects->num)
    goto conf_fail;

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

  if (!conf_parse_lex(conf, 1, conf->data->len))
    goto conf_fail;

  while (conf_parse_token(conf, token))
  {
    if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
      goto conf_fail;

    if (!conf_parse_token(conf, token))
      goto conf_fail;
    
    if (!conf_token_cmp_sym_cstr_eq(conf, token, "org.and.daemon-conf-1.0"))
      goto conf_fail;
  
    if (!opt_serv_conf(opts, conf, token))
      goto conf_fail;
  }

  conf_parse_free(conf);
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, fname, conf, token);
  errno = 0;
 read_malloc_fail:
  if (errno && out) /* can't find config. file */
    vstr_add_fmt(out, out->len, "open(%s): %m", fname);
  conf_parse_free(conf);
 conf_malloc_fail:
  return (FALSE);
}

void opt_serv_conf_free(struct Opt_serv_opts *opts)
{
  Opt_serv_addr_opts *scan = NULL;
  
  if (!opts)
    return;
  
  vstr_free_base(opts->pid_file);         opts->pid_file         = NULL;
  vstr_free_base(opts->cntl_file);        opts->cntl_file        = NULL;
  vstr_free_base(opts->chroot_dir);       opts->chroot_dir       = NULL;
  vstr_free_base(opts->vpriv_uid);        opts->vpriv_uid        = NULL;
  vstr_free_base(opts->vpriv_gid);        opts->vpriv_gid        = NULL;

  scan = opts->addr_beg;
  opts->addr_beg = NULL;
  while (scan)
  {
    Opt_serv_addr_opts *scan_next = scan->next;
    
    vstr_free_base(scan->acpt_filter_file);
    vstr_free_base(scan->ipv4_address);
    F(scan);

    scan = scan_next;
  }
}

int opt_serv_conf_init(Opt_serv_opts *opts)
{
  struct Opt_serv_policy_opts *popts = NULL;
  Opt_serv_addr_opts *addr = MK(sizeof(Opt_serv_addr_opts));
  
  ASSERT(opts && opts->make_policy && opts->copy_policy);

  if (!addr)
    goto mk_addr_fail;
  
  if (!(popts = (*opts->make_policy)(opts)))
    goto mk_policy_fail;

  opts->def_policy = popts;
  vstr_add_cstr_ptr(popts->policy_name, 0, OPT_POLICY_CONF_DEF_POLICY_NAME);

  if (popts->policy_name->conf->malloc_bad)
    goto policy_init_fail;

  opts->pid_file         = vstr_make_base(NULL);
  opts->cntl_file        = vstr_make_base(NULL);
  opts->chroot_dir       = vstr_make_base(NULL);
  opts->vpriv_uid        = vstr_make_base(NULL);
  opts->vpriv_gid        = vstr_make_base(NULL);
  addr->acpt_filter_file = vstr_make_base(NULL);
  addr->ipv4_address     = vstr_make_base(NULL);
    
  if (!opts->pid_file         ||
      !opts->cntl_file        ||
      !opts->chroot_dir       ||
      !opts->vpriv_uid        ||
      !opts->vpriv_gid        ||
      !addr->acpt_filter_file ||
      !addr->ipv4_address     ||
      FALSE)
    goto opts_init_fail;

  addr->next = NULL;
  
  addr->tcp_port        = 0;
  addr->defer_accept    = OPT_SERV_CONF_DEF_TCP_DEFER_ACCEPT;
  addr->q_listen_len    = OPT_SERV_CONF_DEF_Q_LISTEN_LEN;
  addr->max_connections = OPT_SERV_CONF_DEF_MAX_CONNECTIONS;
  
  opts->addr_beg = addr;

  opts->no_conf_listen = TRUE;
  
  return (TRUE);

 opts_init_fail:
  opt_serv_conf_free(opts);
 policy_init_fail:
  vstr_ref_del(popts->ref);
 mk_policy_fail:
  F(addr);
 mk_addr_fail:
  return (FALSE);
}

#define OPT_SERV_ADDR_DUP_VSTR(x)                                       \
    vstr_dup_vstr(opts->addr_beg-> x ->conf ,                           \
                  opts->addr_beg-> x , 1, opts->addr_beg-> x ->len,     \
                  VSTR_TYPE_SUB_BUF_REF)
Opt_serv_addr_opts *opt_serv_make_addr(Opt_serv_opts *opts)
{
  Opt_serv_addr_opts *addr = NULL;

  ASSERT(opts && opts->addr_beg);
  
  if (opts->no_conf_listen)
  { /* use the initial one to start with */
    opts->no_conf_listen = FALSE;
    return (opts->addr_beg);
  }

  /* duplicate the currnet one */
  if (!(addr = MK(sizeof(Opt_serv_addr_opts))))
    goto mk_addr_fail;
    
  addr->acpt_filter_file = OPT_SERV_ADDR_DUP_VSTR(acpt_filter_file);
  addr->ipv4_address     = OPT_SERV_ADDR_DUP_VSTR(ipv4_address);
  
  if (!addr->acpt_filter_file ||
      !addr->ipv4_address     ||
      FALSE)
    goto addr_init_fail;
  
  addr->next = NULL;
  
  addr->tcp_port        = opts->addr_beg->tcp_port;
  addr->defer_accept    = opts->addr_beg->defer_accept;
  addr->q_listen_len    = opts->addr_beg->defer_accept;
  addr->max_connections = opts->addr_beg->defer_accept;

  addr->next = opts->addr_beg;
  opts->addr_beg = addr;

  return (addr);

 addr_init_fail:
  F(addr);
 mk_addr_fail:
  return (FALSE);
}

void opt_serv_logger(Vlg *passed_vlg)
{
  vlg = passed_vlg;
}

void opt_serv_sc_drop_privs(Opt_serv_opts *opts)
{
  if (setgroups(1, &opts->priv_gid) == -1)
    vlg_err(vlg, EXIT_FAILURE, "setgroups(%ld): %m\n", (long)opts->priv_gid);
      
  if (setgid(opts->priv_gid) == -1)
    vlg_err(vlg, EXIT_FAILURE, "setgid(%ld): %m\n", (long)opts->priv_gid);
  
  if (setuid(opts->priv_uid) == -1)
    vlg_err(vlg, EXIT_FAILURE, "setuid(%ld): %m\n", (long)opts->priv_uid);
}

static void opt_serv__sc_rlim_num(const char *name, int resource,
                                  unsigned int num)
{
  struct rlimit rlim[1];
    
  if (getrlimit(resource, rlim) == -1)
    vlg_err(vlg, EXIT_FAILURE, "getrlimit(%s): %m\n", name);


  if (num == RLIM_INFINITY) /* only attempt upwards, if we are privilaged */
  {
    if ((rlim->rlim_max != RLIM_INFINITY) && !getuid())
      rlim->rlim_max = num;
  }
  else
  {
    if ((num > rlim->rlim_max) && !getuid())
      rlim->rlim_max = num;
  
    if (num < rlim->rlim_max) /* can always do this ? */
      rlim->rlim_max = num;
  }
  
  rlim->rlim_cur = rlim->rlim_max; /* upgrade soft to hard */
  
  if (setrlimit(resource, rlim) == -1)
    vlg_err(vlg, EXIT_FAILURE, "setrlimit(%s): %m\n", name);
}

void opt_serv_sc_rlim_file_num(unsigned int rlim_file_num)
{
  opt_serv__sc_rlim_num("NOFILE", RLIMIT_NOFILE, rlim_file_num);
}

void opt_serv_sc_rlim_core_num(unsigned int rlim_core_num)
{
  opt_serv__sc_rlim_num("CORE", RLIMIT_CORE, rlim_core_num);
}

int opt_serv_sc_acpt_end(const Opt_serv_policy_opts *popts,
                         struct Evnt *from_evnt, struct Evnt *evnt)
{
  Acpt_listener *acpt_listener = (Acpt_listener *)from_evnt;
  unsigned int acpt_num = acpt_listener->ref->ref - 1;  /* ref +1 for itself */
  unsigned int acpt_max = acpt_listener->max_connections;

  vlg_dbg1(vlg, "acpt: %u/%u\n", acpt_num, acpt_max);
  
  if (acpt_max && (acpt_num >= acpt_max))
    evnt_wait_cntl_del(from_evnt, POLLIN);

  if (popts->max_connections && (evnt_num_all() > popts->max_connections))
  {
    vlg_info(vlg, "LIMIT-BLOCKED from[$<sa:%p>]: policy $<vstr.all:%p>\n",
             EVNT_SA(evnt), popts->policy_name);
    return (FALSE);
  }
  
  vlg_info(vlg, "CONNECT from[$<sa:%p>]\n", EVNT_SA(evnt));
  
  return (TRUE);
}

void opt_serv_sc_free_beg(struct Evnt *evnt, Vstr_ref *ref)
{
  Acpt_data *acpt_data = ref->ptr;

  if (acpt_data->evnt)
  {
    Acpt_listener *acpt_listener = (Acpt_listener *)acpt_data->evnt;
    unsigned int acpt_num = acpt_listener->ref->ref - 1;
    unsigned int acpt_max = acpt_listener->max_connections;
  
    if (acpt_max && (acpt_num <= acpt_max)) /* note that we are going to -1 */
      evnt_wait_cntl_add(acpt_data->evnt, POLLIN);
    
    evnt_stats_add(acpt_data->evnt, evnt);
  }

  if (evnt->flag_fully_acpt)
    evnt_vlg_stats_info(evnt, "FREE");

  vstr_ref_del(ref);
}

#define OPT_SERV__SIG_OR_ERR(x)                 \
    if (sigaction(x, &sa, NULL) == -1)          \
      err(EXIT_FAILURE, "signal(%d:%s)", x, strsignal(x))

static void opt_serv__sig_crash(int s_ig_num)
{
  vlg_sig_abort(vlg, "SIG[%d]: %s\n", s_ig_num, strsignal(s_ig_num));
}

static void opt_serv__sig_raise_cont(int s_ig_num)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    err(EXIT_FAILURE, "signal init");

  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = SIG_DFL;
  OPT_SERV__SIG_OR_ERR(s_ig_num);

  vlg_sig_info(vlg, "SIG[%d]: %s\n", s_ig_num, strsignal(s_ig_num));
  raise(s_ig_num);
}

static void opt_serv__sig_cont(int s_ig_num)
{
  if (0) /* s_ig_num == SIGCONT) */
  {
    struct sigaction sa;
  
    if (sigemptyset(&sa.sa_mask) == -1)
      err(EXIT_FAILURE, "signal init");

    sa.sa_flags   = SA_RESTART;
    sa.sa_handler = opt_serv__sig_raise_cont;
    OPT_SERV__SIG_OR_ERR(SIGTSTP);
  }
  
  vlg_sig_info(vlg, "SIG[%d]: %s\n", s_ig_num, strsignal(s_ig_num));
}

static void opt_serv__sig_child(int s_ig_num)
{
  ASSERT(s_ig_num == SIGCHLD);
  evnt_child_exited = TRUE;
}

void opt_serv_sc_signals(void)
{
  struct sigaction sa;
  
  if (sigemptyset(&sa.sa_mask) == -1)
    err(EXIT_FAILURE, "signal init %s", "sigemptyset");
  
  /* don't use SA_RESTART ... */
  sa.sa_flags   = 0;
  /* ignore it... we don't have a use for it */
  sa.sa_handler = SIG_IGN;
  
  OPT_SERV__SIG_OR_ERR(SIGPIPE);

  sa.sa_handler = opt_serv__sig_crash;
  
  OPT_SERV__SIG_OR_ERR(SIGSEGV);
  OPT_SERV__SIG_OR_ERR(SIGBUS);
  OPT_SERV__SIG_OR_ERR(SIGILL);
  OPT_SERV__SIG_OR_ERR(SIGFPE);
  OPT_SERV__SIG_OR_ERR(SIGXFSZ);

  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = opt_serv__sig_child;
  OPT_SERV__SIG_OR_ERR(SIGCHLD);
  
  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = opt_serv__sig_cont; /* print, and do nothing */
  
  OPT_SERV__SIG_OR_ERR(SIGUSR1);
  OPT_SERV__SIG_OR_ERR(SIGUSR2);
  OPT_SERV__SIG_OR_ERR(SIGHUP);
  OPT_SERV__SIG_OR_ERR(SIGCONT);
  
  sa.sa_handler = opt_serv__sig_raise_cont; /* queue print, and re-raise */
  
  OPT_SERV__SIG_OR_ERR(SIGTSTP);
  OPT_SERV__SIG_OR_ERR(SIGTERM);
}
#undef SERV__SIG_OR_ERR

void opt_serv_sc_check_children(void)
{
  if (evnt_child_exited)
  {
    vlg_warn(vlg, "Child exited.\n");
    evnt_acpt_close_all();
    evnt_scan_q_close();
    evnt_child_exited = FALSE;
  }
}

void opt_serv_sc_cntl_resources(const Opt_serv_opts *opts)
{ /* cap the amount of "wasted" resources we're using */
  
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BASE,
                 0, opts->max_spare_bases);

  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_BUF,
                 0, opts->max_spare_buf_nodes);
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_PTR,
                 0, opts->max_spare_ptr_nodes);
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_RANGE_SPARE_REF,
                 0, opts->max_spare_ref_nodes);
}

