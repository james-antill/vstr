#ifndef OPT_H
#define OPT_H

#include <getopt.h>
#include <string.h>

#define OPT_TOGGLE_ARG(val) (val = opt_toggle(val, optarg))

extern int opt_toggle(int, const char *);

/* get program name ... but ignore "lt-" libtool prefix */
extern const char *opt_program_name(const char *, const char *);

extern const char *opt_def_toggle(int);

/* uid/gid default to NFS nobody */
#define OPT_SC_SERV_DECL_OPTS() \
  int become_daemon            = FALSE; \
  const char *pid_file         = NULL; \
  const char *cntl_file        = NULL; \
  const char *chroot_dir       = NULL; \
  const char *acpt_filter_file = NULL; \
  int drop_privs               = FALSE; \
  gid_t priv_gid               = 60001; \
  uid_t priv_uid               = 60001

#define OPT_SC_SERV_OPTS() \
      case 1: OPT_TOGGLE_ARG(become_daemon);          break; \
      case 2: chroot_dir       = optarg;              break; \
      case 3: OPT_TOGGLE_ARG(drop_privs);             break; \
      case 4: priv_uid         = atoi(optarg);        break; \
      case 5: priv_gid         = atoi(optarg);        break; \
      case 6: pid_file         = optarg;              break; \
      case 7: cntl_file        = optarg;              break; \
      case 8: acpt_filter_file = optarg;              break


#endif
