/* quick but useful static ssi processor.
*/
#include "ex_utils.h"

#include <stdio.h>

#include <sys/time.h>
#include <time.h>

#include <pwd.h>

#include <spawn.h>


#define EX_SSI_FAILED(x) do {                                           \
        vstr_add_fmt(s1, s1->len,                                       \
                     "<!-- \"%s\" SSI command FAILED -->\n", (x));      \
        goto ssi_parse_failed;                                          \
    }                                                                   \
    while (FALSE)

#define EX_SSI_OK(x, sf, p, l, end) do {                                \
      vstr_add_fmt(s1, s1->len, "<!-- \"%s ${vstr:%p%zu%zu%u}"          \
                   "\" SSI command OK -->%s",                           \
                   (x), (sf), (p), (l), VSTR_TYPE_ADD_ALL_REF,          \
                   (end) ? "\n" : "");                                  \
    }                                                                   \
    while (FALSE)




static Vstr_ref *dname_ref = NULL;
static size_t    dname_off = 0;
static size_t    dname_len = 0;

static char *timespec = NULL;
static int use_size_abbrev = TRUE;

static void ex_ssi_cat_read_fd_write_stdout(Vstr_base *s1, int fd)
{
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s1, fd);
 
    if (io_r_state == IO_EOF)
      break;
                                                                                
    io_w_state = io_put(s1, 1);
                                                                                
    io_limit(io_r_state, fd, io_w_state, 1, s1);
  }
}

static size_t ex_ssi_srch_end(Vstr_base *s2, size_t pos, size_t len)
{
  int instr = FALSE;
  
  while (TRUE)
  {
    size_t srch = vstr_cspn_cstr_chrs_fwd(s2, pos, len, "\\\"-");
      
    pos += srch;
    len -= srch;

    if (!len)
      break;

    if (0) { }
    else if (!instr && vstr_export_chr(s2, pos) == '-')
    { /* see if it's the end... */
      if (len < strlen("-->"))
        return (0);
      
      if (vstr_cmp_bod_cstr_eq(s2, pos, len, "-->"))
        return (pos + strlen("->"));
    }
    else if (!instr && vstr_export_chr(s2, pos) == '\\')
    { /* do nothing */ }
    else if ( instr && vstr_export_chr(s2, pos) == '\\')
    {
      if (len > 1)
      {
        ++pos;
        --len;
      }
    }
    else if (vstr_export_chr(s2, pos) == '"')
    { instr = !instr; }

    ++pos;
    --len;
  }
  
  return (0);
}

static inline void ex_ssi_skip_val(Vstr_base *s2, size_t *srch, size_t val)
{
  vstr_del(s2, 1, val);
  *srch        -= val;  
}

static inline void ex_ssi_skip_wsp(Vstr_base *s2, size_t *srch)
{
  size_t val = vstr_spn_cstr_chrs_fwd(s2, 1, *srch, " ");

  ex_ssi_skip_val(s2, srch, val);
}

static inline void ex_ssi_skip_str(Vstr_base *s2, size_t *srch,
                                    const char *passed_val)
{
  size_t val = strlen(passed_val);
  
  ex_ssi_skip_val(s2, srch, val);
}

static size_t ex_ssi_attr_val(Vstr_base *s2, size_t *srch)
{
  size_t pos = 1;
  size_t len = *srch;
  size_t ret = 0;
  
  while (TRUE)
  {
    size_t scan = vstr_cspn_cstr_chrs_fwd(s2, pos, len, "\\\"");

    if (scan == len)
      return (0);

    ret += scan;
    
    if (vstr_export_chr(s2, pos + scan) != '\\')
      break;

    if (scan == (len - 1))
      return (0);

    ++ret;
    
    vstr_del(s2, pos + scan, 1);
    pos += scan + 1;
    len -= scan + 1;
    --*srch;

    ASSERT(len);
  }

  return (ret);
}

static size_t ex_ssi_file_attr(Vstr_base *s2, size_t *srch)
{
  size_t ret = 0;
  
  ex_ssi_skip_wsp(s2, srch);
  
  if (vstr_cmp_case_bod_cstr_eq(s2, 1, *srch, "file=\""))
    ex_ssi_skip_str(s2, srch, "file=\"");
  else
    return (0);

  if (!(ret = ex_ssi_attr_val(s2, srch)))
    return (0);
  
  if (vstr_export_chr(s2, 1) != '/')
  {
    vstr_add_ref(s2, 0, dname_ref, dname_off, dname_len);
    ret                                    += dname_len;
    *srch                                  += dname_len;
  }

  return (ret);
}

static void ex_ssi_exec(Vstr_base *s1,
                        Vstr_base *s2, size_t pos, size_t len,
                        size_t *add_len)
{
#if 0
  Vstr_sects *sects = vstr_sects_make(4);
  int instr = FALSE;
  char **argv;
  char **argp;
  
  /* FIXME: doesn't handle <foo "bar baz" arg2>... */
  vstr_split_cstr_buf(s2, pos, len, " ", sects, 0, VSTR_FLAG_SPLIT_NO_RET);
  
    
  if (vstr_export_chr(s2, 1) != '/')
  {
    vstr_add_ref(s2, 0, dname_ref, dname_off, dname_len);
    ret                                    += dname_len;
    *srch                                  += dname_len;
  }
#else
  FILE *fp = NULL; /* FIXME: hack job */
  size_t srch = 0;
  size_t tpos = pos;
  size_t tlen = len;
  
  while ((srch = vstr_srch_cstr_buf_fwd(s2, tpos, tlen, " ")) && (srch < tlen))
  {
    switch (vstr_export_chr(s2, srch + 1))
    {
      case '-': /* skip options...*/
      case '/': /* and absolute paths... */
        break;
        
      default:
        vstr_add_ref(s2, srch, dname_ref, dname_off, dname_len);
        len                                       += dname_len;
        tlen                                      += dname_len;
        *add_len                                  += dname_len;
    }
    
    tlen -= vstr_sc_posdiff(tpos, srch) + 1;
    tpos  = srch + 1;
  }
#endif
  
  if (!(fp = popen(vstr_export_cstr_ptr(s2, 1, len), "r")))
    err(EXIT_FAILURE, "popen");

  ex_ssi_cat_read_fd_write_stdout(s1, fileno(fp));

  pclose(fp);
}

static const char *ex_ssi_strftime(time_t val, int use_gmt)
{
  static char ret[4096];
  const char *spec = timespec;
  
  if (!spec)
    spec = "%c";

  strftime(ret, sizeof(ret), spec, use_gmt ? gmtime(&val) : localtime(&val));

  return (ret);
}

static const char *ex_ssi_getpwuid_name(uid_t uid)
{
  struct passwd *pw = getpwuid(uid);

  if (!pw)
    return (":unknown:");

  return (pw->pw_name);
}

static int ex_ssi_process(Vstr_base *s1, Vstr_base *s2, time_t last_modified,
                          int last)
{
  size_t srch = 0;
  int ret = FALSE;
  
  /* we don't want to create more data, if we are over our limit */
  if (s1->len > EX_MAX_W_DATA_INCORE)
    return (FALSE);
  
  while (s2->len >= strlen("<!--#"))
  {
    if (!(srch = vstr_srch_cstr_buf_fwd(s2, 1, s2->len, "<!--#")))
    {
      if (last)
        break;
      
      ret = TRUE;
      vstr_mov(s1, s1->len, s2, 1, s2->len - strlen("<!--#"));
      break;
    }
    
    if (srch > 1)
    {
      ret = TRUE;
      vstr_add_vstr(s1, s1->len, s2, 1, srch - 1, VSTR_TYPE_ADD_BUF_REF);
      vstr_del(s2, 1, srch - 1);
    }

    if (!(srch = ex_ssi_srch_end(s2, 1, s2->len)))
      break;
    
    ret = TRUE;

    if (0) { }
    else if (vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "<!--#include"))
    {
      int fd = -1;
      size_t tmp = 0;

      ex_ssi_skip_str(s2, &srch, "<!--#include");

      if (!(tmp = ex_ssi_file_attr(s2, &srch)))
        EX_SSI_FAILED("include");

      EX_SSI_OK("include", s2, 1, tmp, TRUE);

      if (s1->conf->malloc_bad)
        errno = ENOMEM, err(EXIT_FAILURE, "add data");

      fd = io_open(vstr_export_cstr_ptr(s2, 1, tmp));

      ex_ssi_cat_read_fd_write_stdout(s1, fd);

      if (close(fd) == -1)
        warn("close(%s)", vstr_export_cstr_ptr(s2, 1, tmp));
    }
    else if (vstr_cmp_case_bod_cstr_eq(s2, 1, s2->len, "<!--#exec"))
    {
      size_t tmp = 0;
      
      ex_ssi_skip_str(s2, &srch, "<!--#exec");
      ex_ssi_skip_wsp(s2, &srch);

      if (vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "cmd=\""))
        ex_ssi_skip_str(s2, &srch, "cmd=\"");
      else
        EX_SSI_FAILED("exec");

      if (!(tmp = ex_ssi_attr_val(s2, &srch)))
        EX_SSI_FAILED("exec");

      if (s1->conf->malloc_bad)
        errno = ENOMEM, err(EXIT_FAILURE, "add data");

      EX_SSI_OK("exec", s2, 1, tmp, TRUE);

      ex_ssi_exec(s1, s2, 1, tmp, &srch);
    }
    else if (vstr_cmp_case_bod_cstr_eq(s2, 1, s2->len, "<!--#config"))
    {
      size_t tmp = 0;
      enum { tERR = -1,
             tsize, ttime } type = tERR;
      const char *tname[2] = {"config sizefmt", "config timefmt"};
      
      ex_ssi_skip_str(s2, &srch, "<!--#config");
      ex_ssi_skip_wsp(s2, &srch);

      if (0) { }
      else if (vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "timefmt=\""))
        ex_ssi_skip_str(s2, &srch, "timefmt=\""), type = ttime;
      else if (vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "sizefmt=\""))
        ex_ssi_skip_str(s2, &srch, "sizefmt=\""), type = tsize;
      else
        EX_SSI_FAILED("config");

      ASSERT(type >= 0);
      
      if (!(tmp = ex_ssi_attr_val(s2, &srch)))
        EX_SSI_FAILED(tname[type]);

      EX_SSI_OK(tname[type], s2, 1, tmp, FALSE);

      switch (type)
      {
        case ttime:
          free(timespec);
          timespec = vstr_export_cstr_malloc(s2, 1, tmp);
          break;
        case tsize:
          if (0){ }
          else if (vstr_cmp_cstr_eq(s2, 1, tmp, "bytes"))
            use_size_abbrev = FALSE;
          else if (vstr_cmp_cstr_eq(s2, 1, tmp, "abbrev"))
            use_size_abbrev = TRUE;
          break;
        default:
          ASSERT(FALSE);
      }

      /* <!--#config errmsg="foo" --> ? */
    }
    else if (vstr_cmp_case_bod_cstr_eq(s2, 1, s2->len, "<!--#echo"))
    {
      size_t tmp = 0;
      
      ex_ssi_skip_str(s2, &srch, "<!--#echo");
      ex_ssi_skip_wsp(s2, &srch);

      if (vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "encoding=\""))
        ex_ssi_skip_str(s2, &srch, "encoding=\"");
      else
        EX_SSI_FAILED("echo");

      if (!(tmp = ex_ssi_attr_val(s2, &srch)))
        EX_SSI_FAILED("echo");

      if (!vstr_cmp_cstr_eq(s2, 1, tmp, "none"))
        EX_SSI_FAILED("echo");

      srch -= tmp + 1;
      vstr_del(s2, 1, tmp + 1);
      ex_ssi_skip_wsp(s2, &srch);
      
      if (vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "var=\""))
        ex_ssi_skip_str(s2, &srch, "var=\"");
      else
        EX_SSI_FAILED("echo");

      if (!(tmp = ex_ssi_attr_val(s2, &srch)))
        EX_SSI_FAILED("echo");

      if (0) { }
      else if (vstr_cmp_cstr_eq(s2, 1, tmp, "LAST_MODIFIED"))
        vstr_add_cstr_buf(s1, s1->len, ex_ssi_strftime(last_modified, FALSE));
      else if (vstr_cmp_cstr_eq(s2, 1, tmp, "DATE_GMT"))
        vstr_add_cstr_buf(s1, s1->len, ex_ssi_strftime(time(NULL), TRUE));
      else if (vstr_cmp_cstr_eq(s2, 1, tmp, "DATE_LOCAL"))
        vstr_add_cstr_buf(s1, s1->len, ex_ssi_strftime(time(NULL), FALSE));
      else if (vstr_cmp_cstr_eq(s2, 1, tmp, "USER_NAME"))
        vstr_add_cstr_buf(s1, s1->len, ex_ssi_getpwuid_name(getuid()));
      else
        EX_SSI_FAILED("echo");

      EX_SSI_OK("echo", s2, 1, tmp, FALSE);

      
      /* <!--#echo encoding="none" var="LAST_MODIFIED" --> */
      /* <!--#echo encoding="url" var="LAST_MODIFIED" --> */
      /* <!--#echo encoding="entity" var="LAST_MODIFIED" --> */
      
      /* <!--#echo var="DOCUMENT_NAME" --> */
      /* <!--#echo var="DOCUMENT_URI" --> */
      
    }
    else if (vstr_cmp_case_bod_cstr_eq(s2, 1, s2->len, "<!--#fsize"))
    {
      size_t tmp = 0;
      struct stat sbuf[1];
      
      ex_ssi_skip_str(s2, &srch, "<!--#fsize");

      if (!(tmp = ex_ssi_file_attr(s2, &srch)))
        EX_SSI_FAILED("fsize");

      EX_SSI_OK("fsize", s2, 1, tmp, FALSE);

      if (s1->conf->malloc_bad)
        errno = ENOMEM, err(EXIT_FAILURE, "add data");

      if (stat(vstr_export_cstr_ptr(s2, 1, tmp), sbuf))
        err(EXIT_FAILURE, "stat(%s)", vstr_export_cstr_ptr(s2, 1, tmp));

      if (use_size_abbrev)
        vstr_add_fmt(s1, s1->len, "${BKMG.u:%u}", (unsigned)sbuf->st_size);
      else
        vstr_add_fmt(s1, s1->len, "%ju", (uintmax_t)sbuf->st_size);
    }
    else if (vstr_cmp_case_bod_cstr_eq(s2, 1, s2->len, "<!--#flastmod"))
    {
      size_t tmp = 0;
      struct stat sbuf[1];
      
      ex_ssi_skip_str(s2, &srch, "<!--#flastmod");

      if (!(tmp = ex_ssi_file_attr(s2, &srch)))
        EX_SSI_FAILED("flastmod");

      EX_SSI_OK("flastmod", s2, 1, tmp, FALSE);

      if (s1->conf->malloc_bad)
        errno = ENOMEM, err(EXIT_FAILURE, "add data");

      if (stat(vstr_export_cstr_ptr(s2, 1, tmp), sbuf))
        err(EXIT_FAILURE, "stat(%s)", vstr_export_cstr_ptr(s2, 1, tmp));
        
      vstr_add_cstr_buf(s1, s1->len, ex_ssi_strftime(sbuf->st_mtime, TRUE));
    }
    else if (0 && vstr_cmp_case_bod_cstr_eq(s2, 1, s2->len, "<!--#if"))
    {
      ex_ssi_skip_str(s2, &srch, "<!--#if");
      ex_ssi_skip_wsp(s2, &srch);

      /* <!--#if expr="foo" --> ? */
      /* <!--#elif expt="foo" --> ? */
      /* <!--#else --> ? */
      /* <!--#endif --> ? */
      
    }
    else
      vstr_add_cstr_ptr(s1, s1->len, "<!-- UNKNOWN SSI command -->\n");

    ASSERT(vstr_export_chr(s2, srch) == '>');
    
    vstr_del(s2, 1, srch);
  }
  
 ssi_parse_failed:

  if (last && s2->len)
  {
    ret = TRUE;
    vstr_mov(s1, s1->len, s2, 1, s2->len);
  }
  
  return (ret);  
}


static void ex_ssi_process_limit(Vstr_base *s1, Vstr_base *s2,
                                 time_t last_modified, unsigned int lim)
{
  while (s2->len > lim)
  {
    int proc_data = ex_ssi_process(s1, s2, last_modified, !lim);
    if (!proc_data && (io_put(s1, STDOUT_FILENO) == IO_BLOCK))
      io_block(-1, STDOUT_FILENO);
  }
}

static void ex_ssi_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2, int fd)
{
  struct stat sbuf[1];
  time_t last_modified = time(NULL);
  
  if (!fstat(fd, sbuf))
    last_modified = sbuf->st_mtime;
    
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s2, fd);

    if (io_r_state == IO_EOF)
      break;

    ex_ssi_process(s1, s2, last_modified, FALSE);
    
    io_w_state = io_put(s1, 1);

    io_limit(io_r_state, fd, io_w_state, 1, s1);    
  }

  ex_ssi_process_limit(s1, s2, last_modified, 0);
}

static void ex_ssi_fin(Vstr_base *s1, time_t timestamp, const char *fname)
{
  free(timespec);
  timespec = NULL;

  vstr_add_fmt(s1, s1->len,
               "<!-- SSI processing of %s -->\n"
               "<!--   done on %s -->\n"
               "<!--   done by jssi -->\n",
               fname, ex_ssi_strftime(timestamp, FALSE));
}

int main(int argc, char *argv[])
{
  Vstr_base *s2 = NULL;
  Vstr_base *s1 = ex_init(&s2);
  int count = 1;
  time_t now = time(NULL);

  vstr_sc_fmt_add_all(s1->conf);
  vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  
  if (!(dname_ref = vstr_ref_make_strdup("")))
    errno = ENOMEM, err(EXIT_FAILURE, "add data");
    
  if (count >= argc)
  {
    io_fd_set_o_nonblock(STDIN_FILENO);
    ex_ssi_read_fd_write_stdout(s1, s2, STDIN_FILENO);
    ex_ssi_fin(s1, now, "stdin");    
  }
  
  while (count < argc)
  {
    int fd = io_open(argv[count]);
    size_t len = strlen(argv[count]);
    size_t dbeg = 0;
    size_t tdname_len = 0;
    
    if (!vstr_add_buf(s1, s1->len, argv[count], len))
      errno = ENOMEM, err(EXIT_FAILURE, "add data");

    dbeg = s1->len - len + 1;
    vstr_sc_dirname(s1, dbeg, len, &tdname_len);
    if (tdname_len)
    {
      dname_len = tdname_len;
      vstr_ref_del(dname_ref);
      if (!(dname_ref = vstr_export_ref(s1, dbeg, dname_len, &dname_off)))
        errno = ENOMEM, err(EXIT_FAILURE, "add data");
    }
    vstr_del(s1, s1->len - len + 1, len);
    
    ex_ssi_read_fd_write_stdout(s1, s2, fd);
    ex_ssi_fin(s1, now, argv[count]);    

    if (close(fd) == -1)
      warn("close(%s)", argv[count]);

    ++count;
  }

  vstr_ref_del(dname_ref);
  
  io_put_all(s1, STDOUT_FILENO);

  exit (ex_exit(s1, s2));
}
