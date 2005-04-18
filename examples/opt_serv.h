#ifndef OPT_SERV_H
#define OPT_SERV_H

#include "opt.h"
#include "conf.h"

#define OPT_SERV_CONF_USE_DAEMON FALSE
#define OPT_SERV_CONF_USE_DROP_PRIVS FALSE
#define OPT_SERV_CONF_USE_PDEATHSIG FALSE
#define OPT_SERV_CONF_DEF_TCP_DEFER_ACCEPT 8 /* HC usage txt */
#define OPT_SERV_CONF_DEF_PRIV_UID 60001
#define OPT_SERV_CONF_DEF_PRIV_GID 60001
#define OPT_SERV_CONF_DEF_NUM_PROCS 1
#define OPT_SERV_CONF_DEF_IDLE_TIMEOUT (2 * 60)
#define OPT_SERV_CONF_DEF_Q_LISTEN_LEN 128
#define OPT_SERV_CONF_DEF_MAX_CONNECTIONS 0
#define OPT_SERV_CONF_DEF_RLIM_FILE_NUM 0

typedef struct Opt_serv_policy_opts
{
 unsigned int idle_timeout;
} Opt_serv_policy_opts;

typedef struct Opt_serv_opts
{
 unsigned int become_daemon : 1;
 unsigned int drop_privs : 1;
 unsigned int use_pdeathsig : 1;
 unsigned int defer_accept : 12; /* 0 => 4095 (1hr8m) (10th-22nd bitfields) */
 Vstr_base *pid_file;
 Vstr_base *cntl_file;
 Vstr_base *chroot_dir;
 Vstr_base *acpt_filter_file;

 Vstr_base *vpriv_uid;
 uid_t priv_uid;
 Vstr_base *vpriv_gid;
 gid_t priv_gid;
 unsigned int num_procs;
 unsigned int idle_timeout;
 Vstr_base *ipv4_address;
 short tcp_port;
 unsigned int q_listen_len;
 unsigned int max_connections;
 unsigned int rlim_file_num;
} Opt_serv_opts;
/* uid/gid default to NFS nobody */
#define OPT_SERV_CONF_INIT_OPTS(x)                                      \
    OPT_SERV_CONF_USE_DAEMON,                                           \
    OPT_SERV_CONF_USE_DROP_PRIVS,                                       \
    OPT_SERV_CONF_USE_PDEATHSIG,                                        \
    OPT_SERV_CONF_DEF_TCP_DEFER_ACCEPT,                                 \
    NULL, NULL, NULL, NULL,                                             \
    NULL, OPT_SERV_CONF_DEF_PRIV_UID,                                   \
    NULL, OPT_SERV_CONF_DEF_PRIV_GID,                                   \
    OPT_SERV_CONF_DEF_NUM_PROCS,                                        \
    OPT_SERV_CONF_DEF_IDLE_TIMEOUT,                                     \
    NULL, x,                                                            \
    OPT_SERV_CONF_DEF_Q_LISTEN_LEN,                                     \
    OPT_SERV_CONF_DEF_MAX_CONNECTIONS, \
    OPT_SERV_CONF_DEF_RLIM_FILE_NUM

#define OPT_SERV_CONF_DECL_OPTS(N, x)                           \
    Opt_serv_opts N[1] = {{OPT_SERV_CONF_INIT_OPTS(x)}}

extern int  opt_serv_conf_init(Opt_serv_opts *);
extern void opt_serv_conf_free(Opt_serv_opts *);

extern int opt_serv_conf(Opt_serv_opts *, const Conf_parse *, Conf_token *);
extern int opt_serv_conf_parse_cstr(Vstr_base *, Opt_serv_opts *, const char *);
extern int opt_serv_conf_parse_file(Vstr_base *, Opt_serv_opts *, const char *);

#define OPT_SERV_DECL_GETOPTS()                         \
   {"help", no_argument, NULL, 'h'},                    \
   {"daemon", optional_argument, NULL, 1},              \
   {"chroot", required_argument, NULL, 2},              \
   {"drop-privs", optional_argument, NULL, 3},          \
   {"priv-uid", required_argument, NULL, 4},            \
   {"priv-gid", required_argument, NULL, 5},            \
   {"pid-file", required_argument, NULL, 6},            \
   {"cntl-file", required_argument, NULL, 7},           \
   {"acpt-filter-file", required_argument, NULL, 8},    \
   {"accept-filter-file", required_argument, NULL, 8},  \
   {"processes", required_argument, NULL, 9},           \
   {"procs", required_argument, NULL, 9},               \
   {"debug", no_argument, NULL, 'd'},                   \
   {"host", required_argument, NULL, 'H'},              \
   {"port", required_argument, NULL, 'P'},              \
   {"nagle", optional_argument, NULL, 'n'},             \
   {"max-connections", required_argument, NULL, 'M'},   \
   {"idle-timeout", required_argument, NULL, 't'},      \
   {"defer-accept", required_argument, NULL, 10},       \
   {"version", no_argument, NULL, 'V'}

#define OPT_SERV_GETOPTS(opts)                                          \
    case 't': opts->idle_timeout    = atoi(optarg);                 break; \
    case 'H': OPT_VSTR_ARG(opts->ipv4_address);                     break; \
    case 'M': OPT_NUM_NR_ARG(opts->max_connections, "max connections"); break; \
    case 'P': OPT_NUM_ARG(opts->tcp_port, "tcp port", 0, 65535, ""); break; \
    case 'd': vlg_debug(vlg);                                       break; \
                                                                        \
    case 'n': OPT_TOGGLE_ARG(evnt_opt_nagle);                       break; \
                                                                        \
    case 1: OPT_TOGGLE_ARG(opts->become_daemon);                    break; \
    case 2: OPT_VSTR_ARG(opts->chroot_dir);                         break; \
    case 3: OPT_TOGGLE_ARG(opts->drop_privs);                       break; \
    case 4: OPT_VSTR_ARG(opts->vpriv_uid);                          break; \
    case 5: OPT_VSTR_ARG(opts->vpriv_gid);                          break; \
    case 6: OPT_VSTR_ARG(opts->pid_file);                           break; \
    case 7: OPT_VSTR_ARG(opts->cntl_file);                          break; \
    case 8: OPT_VSTR_ARG(opts->acpt_filter_file);                   break; \
    case 9: OPT_NUM_ARG(opts->num_procs, "number of processes",         \
                        1, 255, "");                                break; \
    case 10: OPT_NUM_ARG(opts->defer_accept, "seconds to defer connections", \
                         0, 4906, " (1 hour 8 minutes)");           break



/* simple typer for EQ */
#define OPT_SERV_SYM_EQ(x)                                      \
    conf_token_cmp_sym_buf_eq(conf, token, x, strlen(x))

/* eXport data from configuration file to structs... */
#define OPT_SERV_X_TOGGLE(x) do {                               \
      int opt__val = (x);                                       \
                                                                \
      if (conf_sc_token_parse_toggle(conf, token, &opt__val))   \
        return (FALSE);                                         \
      (x) = opt__val;                                           \
    } while (FALSE)

#define OPT_SERV_X_UINT(x) do {                                 \
      unsigned int opt__val = 0;                                \
                                                                \
      if (conf_sc_token_parse_uint(conf, token, &opt__val))     \
        return (FALSE);                                         \
      (x) = opt__val;                                           \
    } while (FALSE)

#define OPT_SERV_X__ESC_VSTR(x, p, l) do {                              \
      if ((token->type == CONF_TOKEN_TYPE_QUOTE_ESC_D) ||               \
          (token->type == CONF_TOKEN_TYPE_QUOTE_ESC_DDD) ||             \
          (token->type == CONF_TOKEN_TYPE_QUOTE_ESC_S) ||               \
          (token->type == CONF_TOKEN_TYPE_QUOTE_ESC_SSS) ||             \
          FALSE)                                                        \
        if (!conf_sc_conv_unesc((x), p, l, NULL))                       \
          return (FALSE);                                               \
    } while (FALSE)

#define OPT_SERV_X__VSTR(x, p, l) do {                                  \
      if (conf_sc_token_sub_vstr(conf, token, x, p, l))                 \
        return (FALSE);                                                 \
                                                                        \
      if ((x)->conf->malloc_bad)                                        \
        return (FALSE);                                                 \
      OPT_SERV_X__ESC_VSTR(x, p, token->u.node->len);                   \
    } while (FALSE)

#define OPT_SERV_X_VSTR(x) OPT_SERV_X__VSTR(x, 1, (x)->len)

#endif
