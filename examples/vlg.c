
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <err.h>

#define VLG_COMPILE_INLINE 0
#include "vlg.h"

#define assert(x) do { if (x) {} else errx(EXIT_FAILURE, "assert(" #x "), FAILED at line %u", __LINE__); } while (FALSE)
#define ASSERT(x) do { if (x) {} else errx(EXIT_FAILURE, "ASSERT(" #x "), FAILED at line %u", __LINE__); } while (FALSE)
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


static void vlg__flush(Vlg *vlg, int syslg_type, int out_err)
{
  Vstr_base *dlg = vlg->out_vstr;

  if (!vlg->daemon_mode)
  {
    int fd = out_err ? STDERR_FILENO : STDOUT_FILENO;
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
    syslog(syslg_type, "%s", tmp);
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

void vlg_init(void)
{
  unsigned int buf_sz = 0;
  
  if (!(vlg__conf = vstr_make_conf()))
    goto malloc_err_vstr_conf;

  if (!vstr_cntl_conf(vlg__conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$') ||
      !vstr_sc_fmt_add_all(vlg__conf) ||
      !VSTR_SC_FMT_ADD(vlg__conf, vlg__fmt_add_vstr_add_vstr,
                       "<vstr", "p%zu%zu", ">") ||
      !VSTR_SC_FMT_ADD(vlg__conf, vlg__fmt_add_vstr_add_sect_vstr,
                       "<vstr.sect", "p%p%u", ">") ||
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
    openlog(name, LOG_PID, LOG_DAEMON);
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
void vlg_verr(Vlg *vlg, int exit_code, const char *fmt, va_list ap)
{
  Vstr_base *dlg = vlg->out_vstr;

  if (!vstr_add_vfmt(dlg, dlg->len, fmt, ap))
    errno = ENOMEM, err(exit_code, "vlog_verr");
  
  if (vstr_srch_chr_fwd(dlg, 1, dlg->len, '\n'))
  {
    if (!vstr_add_cstr_ptr(dlg, 0, "ERR: "))
      errno = ENOMEM, err(EXIT_FAILURE, "vlog_verr");
      
    vlg__flush(vlg, LOG_ALERT, TRUE);
  }
  
  _exit(exit_code);
}

void vlg_vwarn(Vlg *vlg, const char *fmt, va_list ap)
{
  Vstr_base *dlg = vlg->out_vstr;

  if (!vstr_add_vfmt(dlg, dlg->len, fmt, ap))
    errno = ENOMEM, err(EXIT_FAILURE, "vlog_vwarn");
  
  if (vstr_srch_chr_fwd(dlg, 1, dlg->len, '\n'))
  {
    if (!vstr_add_cstr_ptr(dlg, 0, "WARN: "))
      errno = ENOMEM, err(EXIT_FAILURE, "vlog_vwarn");
      
    vlg__flush(vlg, LOG_WARNING, TRUE);
  }
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
  {
    if (!vstr_add_cstr_ptr(dlg, 0, "DEBUG: "))
      errno = ENOMEM, err(EXIT_FAILURE, "vlog_vdbg");
      
    vlg__flush(vlg, LOG_DEBUG, FALSE);
  }
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

extern void vlg_err(Vlg *vlg, int exit_code, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_verr(vlg, exit_code, fmt, ap);
  va_end(ap);
  
  ASSERT_NOT_REACHED();
}

extern void vlg_warn(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vwarn(vlg, fmt, ap);
  va_end(ap);
}

extern void vlg_info(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vinfo(vlg, fmt, ap);
  va_end(ap);
}

extern void vlg_dbg1(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vdbg1(vlg, fmt, ap);
  va_end(ap);
}

extern void vlg_dbg2(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vdbg2(vlg, fmt, ap);
  va_end(ap);
}

extern void vlg_dbg3(Vlg *vlg, const char *fmt, ... )
{
  va_list ap;

  va_start(ap, fmt);
  vlg_vdbg3(vlg, fmt, ap);
  va_end(ap);
}
