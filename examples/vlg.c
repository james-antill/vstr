
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <err.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

#include <limits.h>

#include "date.h"

#define VLG_COMPILE_INLINE 0
#include "vlg.h"

#ifndef VSTR_AUTOCONF_NDEBUG
# define assert(x) do { if (x) {} else errx(EXIT_FAILURE, "assert(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
# define ASSERT(x) do { if (x) {} else errx(EXIT_FAILURE, "ASSERT(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
#else
# define ASSERT(x)
# define assert(x)
#endif
#define ASSERT_NOT_REACHED() assert(FALSE)

/* how much memory should we preallocate so it's "unlikely" we'll get mem errors
 * when writting a log entry */
#define VLG_MEM_PREALLOC (4 * 1024)

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

static Vstr_conf *vlg__conf = NULL;
static int vlg__done_syslog_init = FALSE;


static void vlg__flush(Vlg *vlg, int type, int out_err)
{
  Vstr_base *dlg = vlg->out_vstr;

  if (!vlg->daemon_mode)
  {
    int fd = out_err ? STDERR_FILENO : STDOUT_FILENO;
    const char *tm = date_syslog(time(NULL));
    
    if (!vstr_add_cstr_ptr(dlg, 0, tm) ||
        !vstr_add_cstr_ptr(dlg, 0, "[") ||
        !vstr_add_cstr_ptr(dlg, strlen(tm) + 1, "]: "))
      errno = ENOMEM, err(EXIT_FAILURE, "time");

    if ((type == LOG_WARNING) && !vstr_add_cstr_ptr(dlg, 0, "WARN: "))
      errno = ENOMEM, err(EXIT_FAILURE, "warn");
    if ((type == LOG_ALERT) && !vstr_add_cstr_ptr(dlg, 0, "ERR: "))
      errno = ENOMEM, err(EXIT_FAILURE, "warn");
    if ((type == LOG_DEBUG) && !vstr_add_cstr_ptr(dlg, 0, "DEBUG: "))
      errno = ENOMEM, err(EXIT_FAILURE, "vlog_vdbg");

    while (dlg->len)
      if (!vstr_sc_write_fd(dlg, 1, dlg->len, fd, NULL) && (errno != EAGAIN))
        err(EXIT_FAILURE, "vlg__flush");
  }
  else
  {
    const char *tmp = vstr_export_cstr_ptr(dlg, 1, dlg->len);

    if (!tmp)
      errno = ENOMEM, err(EXIT_FAILURE, "vlog__flush");
    
    /* ignoring borken syslog()'s that overflow ... eventually implement
     * syslog protocol, until then use a real OS */
    syslog(type, "%s", tmp);
    vstr_del(dlg, 1, dlg->len);
  }
}

/* because vlg goes away quickly it's likely we'll want to just use _BUF_PTR
   for Vstr data to save the copying. So here is a helper. */
static int vlg__fmt__add_vstr_add_vstr(Vstr_base *base, size_t pos,
                                       Vstr_fmt_spec *spec)
{
  Vstr_base *sf          = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_pos          = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);
  size_t sf_len          = VSTR_FMT_CB_ARG_VAL(spec, size_t, 2);
  unsigned int sf_flags  = VSTR_TYPE_ADD_BUF_PTR;
                                                                                
  if (!vstr_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                          VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);
                                                                                
  if (!vstr_add_vstr(base, pos, sf, sf_pos, sf_len, sf_flags))
    return (FALSE);
                                                                                
  if (!vstr_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
                                                                                
  return (TRUE);
}

static int vlg__fmt_add_vstr_add_vstr(Vstr_conf *conf, const char *name)
{
  return (vstr_fmt_add(conf, name, vlg__fmt__add_vstr_add_vstr,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_SIZE_T,
                       VSTR_TYPE_FMT_SIZE_T,
                       VSTR_TYPE_FMT_END));
}

/* also a helper for printing sects --
 * should probably have some in Vstr itself */
static int vlg__fmt__add_vstr_add_sect_vstr(Vstr_base *base, size_t pos,
                                            Vstr_fmt_spec *spec)
{
  Vstr_base *sf          = VSTR_FMT_CB_ARG_PTR(spec, 0);
  Vstr_sects *sects      = VSTR_FMT_CB_ARG_PTR(spec, 1);
  unsigned int num       = VSTR_FMT_CB_ARG_VAL(spec, unsigned int, 2);
  size_t sf_pos          = VSTR_SECTS_NUM(sects, num)->pos;
  size_t sf_len          = VSTR_SECTS_NUM(sects, num)->len;
  unsigned int sf_flags  = VSTR_TYPE_ADD_BUF_PTR;
                                                                                
  if (!vstr_sc_fmt_cb_beg(base, &pos, spec, &sf_len,
                          VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);
                                                                                
  if (!vstr_add_vstr(base, pos, sf, sf_pos, sf_len, sf_flags))
    return (FALSE);
                                                                                
  if (!vstr_sc_fmt_cb_end(base, pos, spec, sf_len))
    return (FALSE);
                                                                                
  return (TRUE);
}

static int vlg__fmt_add_vstr_add_sect_vstr(Vstr_conf *conf, const char *name)
{
  return (vstr_fmt_add(conf, name, vlg__fmt__add_vstr_add_sect_vstr,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_UINT,
                       VSTR_TYPE_FMT_END));
}

/* also a helper for printing any network address */
static int vlg__fmt__add_vstr_add_sa(Vstr_base *base, size_t pos,
                                     Vstr_fmt_spec *spec)
{
  struct sockaddr *sa = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t obj_len = 0;
  char buf1[128 + 1];
  char buf2[sizeof(short) * CHAR_BIT + 1];
  const char *ptr1 = NULL;
  size_t len1 = 0;
  const char *ptr2 = NULL;
  size_t len2 = 0;

  assert(sa);
  assert(sizeof(buf1) >= INET_ADDRSTRLEN);
  assert(sizeof(buf1) >= INET6_ADDRSTRLEN);

  switch (sa->sa_family)
  {
    case AF_INET:
    {
      struct sockaddr_in *sin4 = (void *)sa;
      ptr1 = inet_ntop(AF_INET, &sin4->sin_addr, buf1, sizeof(buf1));
      if (!ptr1) ptr1 = "<unknown>";
      len1 = strlen(ptr1);
      ptr2 = buf2;
      len2 = vstr_sc_conv_num10_uint(buf2, sizeof(buf2), ntohs(sin4->sin_port));
    }
    break;
      
    case AF_INET6:
    {
      struct sockaddr_in6 *sin6 = (void *)sa;
      ptr1 = inet_ntop(AF_INET6, &sin6->sin6_addr, buf1, sizeof(buf1));
      if (!ptr1) ptr1 = "<unknown>";
      len1 = strlen(ptr1);
      ptr2 = buf2;
      len2 = vstr_sc_conv_num10_uint(buf2,sizeof(buf2), ntohs(sin6->sin6_port));
    }
    break;
    
    case AF_LOCAL:
    {
      struct sockaddr_un *sun = (void *)sa;
      ptr1 = sun->sun_path;
      len1 = strlen(ptr1);
    }
    break;
    
    default: ASSERT_NOT_REACHED();
  }

  obj_len = len1 + !!len2 + len2;
  
  if (!vstr_sc_fmt_cb_beg(base, &pos, spec, &obj_len,
                          VSTR_FLAG_SC_FMT_CB_BEG_OBJ_ATOM))
    return (FALSE);
  ASSERT(obj_len == (len1 + !!len2 + len2));
  
  if (!vstr_add_buf(base, pos, ptr1, len1))
    return (FALSE);
  if (ptr2 && (!vstr_add_rep_chr(base, pos + len1, '@', 1) ||
               !vstr_add_buf(    base, pos + len1 + 1, ptr2, len2)))
    return (FALSE);
                                                                                
  if (!vstr_sc_fmt_cb_end(base, pos, spec, obj_len))
    return (FALSE);
                                                                                
  return (TRUE);
}

static int vlg__fmt_add_vstr_add_sa(Vstr_conf *conf, const char *name)
{
  return (vstr_fmt_add(conf, name, vlg__fmt__add_vstr_add_sa,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_END));
}

int vlg_sc_fmt_add_all(Vstr_conf *conf)
{
  return (VSTR_SC_FMT_ADD(conf, vlg__fmt_add_vstr_add_vstr,
                          "<vstr", "p%zu%zu", ">") &&
          VSTR_SC_FMT_ADD(conf, vlg__fmt_add_vstr_add_sect_vstr,
                          "<vstr.sect", "p%p%u", ">") &&
          VSTR_SC_FMT_ADD(conf, vlg__fmt_add_vstr_add_sa,
                          "<sa", "p", ">"));
}

void vlg_init(void)
{
  unsigned int buf_sz = 0;
  
  if (!(vlg__conf = vstr_make_conf()))
    goto malloc_err_vstr_conf;

  if (!vstr_cntl_conf(vlg__conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$') ||
      !vstr_sc_fmt_add_all(vlg__conf) ||
      !vlg_sc_fmt_add_all(vlg__conf) ||
      FALSE)
    goto malloc_err_vstr_fmt_all;

  vstr_cntl_conf(vlg__conf, VSTR_CNTL_CONF_GET_NUM_BUF_SZ, &buf_sz);

  /* don't bother with _NON nodes */
  if (!vstr_make_spare_nodes(vlg__conf, VSTR_TYPE_NODE_BUF,
                             (VLG_MEM_PREALLOC / buf_sz) + 1) ||
      !vstr_make_spare_nodes(vlg__conf, VSTR_TYPE_NODE_PTR,
                             (VLG_MEM_PREALLOC / buf_sz) + 1) ||
      !vstr_make_spare_nodes(vlg__conf, VSTR_TYPE_NODE_REF,
                             (VLG_MEM_PREALLOC / buf_sz) + 1))
    goto malloc_err_vstr_spare;

  return;
  
 malloc_err_vstr_spare:
 malloc_err_vstr_fmt_all:
  vstr_free_conf(vlg__conf);
 malloc_err_vstr_conf:
  errno = ENOMEM; err(EXIT_FAILURE, "vlg_init");
}

void vlg_exit(void)
{
  if (vlg__done_syslog_init)
    closelog();
  
  vstr_free_conf(vlg__conf); vlg__conf = NULL;
}

Vlg *vlg_make(void)
{
  Vlg *vlg = malloc(sizeof(Vlg));

  if (!vlg)
    goto malloc_err_vlg;

  if (!(vlg->out_vstr = vstr_make_base(vlg__conf)))
    goto malloc_err_vstr_base;
  
  vlg->out_dbg     = 0;
  vlg->daemon_mode = FALSE;
  
  return (vlg);

 malloc_err_vstr_base:
  free(vlg);
 malloc_err_vlg:
  
  return (NULL);
}

/* don't actually free ... this shouldn't happen until exit time anyway */
void vlg_free(Vlg *vlg)
{
  vstr_free_base(vlg->out_vstr);
  vlg->out_vstr = NULL; /* should NULL deref if used post "free" */
}

void vlg_daemon(Vlg *vlg, const char *name)
{
  if (!vlg__done_syslog_init)
    openlog(name, LOG_PID | LOG_NDELAY, LOG_DAEMON);
  vlg__done_syslog_init = TRUE;

  vlg->daemon_mode = TRUE;
}

void vlg_debug(Vlg *vlg)
{
  if (vlg->out_dbg >= 3)
    return;

  ++vlg->out_dbg;
}

/* ================== actual logging functions ================== */

/* ---------- va_list ---------- */
void vlg_vabort(Vlg *vlg, const char *fmt, va_list ap)
{
  Vstr_base *dlg = vlg->out_vstr;

  if (!vstr_add_vfmt(dlg, dlg->len, fmt, ap))
    errno = ENOMEM, err(EXIT_FAILURE, "vlog_vabort");
  
  if (vstr_srch_chr_fwd(dlg, 1, dlg->len, '\n'))
    vlg__flush(vlg, LOG_ALERT, TRUE);
  
  abort();
}

void vlg_verr(Vlg *vlg, int exit_code, const char *fmt, va_list ap)
{
  Vstr_base *dlg = vlg->out_vstr;

  if (!vstr_add_vfmt(dlg, dlg->len, fmt, ap))
    errno = ENOMEM, err(exit_code, "vlog_verr");
  
  if (vstr_srch_chr_fwd(dlg, 1, dlg->len, '\n'))
    vlg__flush(vlg, LOG_ALERT, TRUE);
  
  _exit(exit_code);
}

void vlg_vwarn(Vlg *vlg, const char *fmt, va_list ap)
{
  Vstr_base *dlg = vlg->out_vstr;

  if (!vstr_add_vfmt(dlg, dlg->len, fmt, ap))
    errno = ENOMEM, err(EXIT_FAILURE, "vlog_vwarn");
  
  if (vstr_srch_chr_fwd(dlg, 1, dlg->len, '\n'))
    vlg__flush(vlg, LOG_WARNING, TRUE);
}

void vlg_vinfo(Vlg *vlg, const char *fmt, va_list ap)
{
  Vstr_base *dlg = vlg->out_vstr;
  
  if (!vstr_add_vfmt(dlg, dlg->len, fmt, ap))
    errno = ENOMEM, err(EXIT_FAILURE, "vlog_vinfo");
  
  if (vstr_srch_chr_fwd(dlg, 1, dlg->len, '\n'))
    vlg__flush(vlg, LOG_NOTICE, FALSE);
}

static void vlg__vdbg(Vlg *vlg, const char *fmt, va_list ap)
{
  Vstr_base *dlg = vlg->out_vstr;
  
  if (!vstr_add_vfmt(dlg, dlg->len, fmt, ap))
    errno = ENOMEM, err(EXIT_FAILURE, "vlog_vdbg");
  
  if (vstr_srch_chr_fwd(dlg, 1, dlg->len, '\n'))
    vlg__flush(vlg, LOG_DEBUG, FALSE);
}

void vlg_vdbg1(Vlg *vlg, const char *fmt, va_list ap)
{
  if (vlg->out_dbg < 1)
    return;

  vlg__vdbg(vlg, fmt, ap);
}

void vlg_vdbg2(Vlg *vlg, const char *fmt, va_list ap)
{
  if (vlg->out_dbg < 2)
    return;

  vlg__vdbg(vlg, fmt, ap);
}

void vlg_vdbg3(Vlg *vlg, const char *fmt, va_list ap)
{
  if (vlg->out_dbg < 3)
    return;

  vlg__vdbg(vlg, fmt, ap);
}

/* ---------- ... ---------- */

void vlg_abort(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vabort(vlg, fmt, ap);
  va_end(ap);
  
  ASSERT_NOT_REACHED();
}

void vlg_err(Vlg *vlg, int exit_code, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_verr(vlg, exit_code, fmt, ap);
  va_end(ap);
  
  ASSERT_NOT_REACHED();
}

void vlg_warn(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vwarn(vlg, fmt, ap);
  va_end(ap);
}

void vlg_info(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vinfo(vlg, fmt, ap);
  va_end(ap);
}

void vlg_dbg1(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vdbg1(vlg, fmt, ap);
  va_end(ap);
}

void vlg_dbg2(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vdbg2(vlg, fmt, ap);
  va_end(ap);
}

void vlg_dbg3(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vdbg3(vlg, fmt, ap);
  va_end(ap);
}

void vlg_pid_file(Vlg *vlg, const char *pid_file)
{
  int fd = open(pid_file, O_WRONLY | O_CREAT | O_TRUNC);
  Vstr_base *out = vlg->out_vstr;
  
  if (fd == -1)
    vlg_err(vlg, EXIT_FAILURE, "open(%s): %m\n", pid_file);

  if (out->len)
    vlg_err(vlg, EXIT_FAILURE, "Data in vlg for pid_file\n");
  
  if (!vstr_add_fmt(out, out->len, "%lu", (unsigned long)getpid()))
    vlg_err(vlg, EXIT_FAILURE, "vlg_pid_file: %m\n");

  while (out->len)
    if (!vstr_sc_write_fd(out, 1, out->len, fd, NULL) && (errno != EAGAIN))
      vlg_err(vlg, EXIT_FAILURE, "write(%s): %m\n", pid_file);

  close(fd);
}
