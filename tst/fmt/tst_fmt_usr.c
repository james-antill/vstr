#include "tst-main.c"

static const char *rf = __FILE__;

#ifndef HAVE_POSIX_HOST
static int getpid(void) { return (0xdeadbeef); }
#endif

static int tst_errno = 0;

static int tst_usr_vstr_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  Vstr_base *sf          = VSTR_FMT_CB_ARG_PTR(spec, 0);
  unsigned int sf_flags  = VSTR_FMT_CB_ARG_VAL(spec, unsigned int, 1);

  assert(!strcmp(spec->name, "{VSTR:%p%u}"));
  
  if (!vstr_add_vstr(st, pos, sf, 1, sf->len, sf_flags))
    return (FALSE);

  return (TRUE);
}

static int tst_usr_pid_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  assert(!strcmp(spec->name, "PID"));
  
  if (!vstr_add_fmt(st, pos, "%lu", (unsigned long)getpid()))
    return (FALSE);

  return (TRUE);
}

static int tst_usr_blank_errno1_cb(Vstr_base *st, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  size_t sf_len = 0;
  
  assert(st && !pos);
  assert(!strcmp(spec->name, "{BLANK_ERRNO1}"));

  assert(tst_errno == ERANGE);
  assert(errno == ERANGE);
  tst_errno = 0;
  errno = 0;

  if (!vstr_sc_fmt_cb_beg(st, &pos, spec, &sf_len,
                          VSTR_FLAG_SC_FMT_CB_BEG_DEF))
    return (FALSE);
  
  if (!vstr_sc_fmt_cb_end(st, pos, spec, sf_len))
    return (FALSE);

  
  return (TRUE);
}

static int tst_usr_blank_errno2_cb(Vstr_base *st, size_t pos,
                                   Vstr_fmt_spec *spec)
{
  assert(st && pos);
  assert(!strcmp(spec->name, "{BLANK_ERRNO2}"));

  assert(!tst_errno);
  assert(errno == ERANGE);
  tst_errno = 0;
  errno = 0;

  return (TRUE);
}

static int tst_usr_strerror_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  assert(!strcmp(spec->name, "m"));
  
  if (!VSTR_ADD_CSTR_PTR(st, pos, strerror(tst_errno)))
    return (FALSE);

  return (TRUE);
}

static int tst_usr_all_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  double             arg1 = VSTR_FMT_CB_ARG_VAL(spec, double, 0);
  long double        arg2 = VSTR_FMT_CB_ARG_VAL(spec, long double, 1);
  int                arg3 = VSTR_FMT_CB_ARG_VAL(spec, int, 2);
  intmax_t           arg4 = VSTR_FMT_CB_ARG_VAL(spec, intmax_t, 3);
  long               arg5 = VSTR_FMT_CB_ARG_VAL(spec, long, 4);
  long long          arg6 = VSTR_FMT_CB_ARG_VAL(spec, long long, 5);
  ptrdiff_t          arg7 = VSTR_FMT_CB_ARG_VAL(spec, ptrdiff_t, 6);
  wchar_t           *arg8 = VSTR_FMT_CB_ARG_PTR(spec, 7);
  size_t             arg9 = VSTR_FMT_CB_ARG_VAL(spec, size_t, 8);
  ssize_t            arg10 = VSTR_FMT_CB_ARG_VAL(spec, ssize_t, 9);
  uintmax_t          arg11 = VSTR_FMT_CB_ARG_VAL(spec, uintmax_t, 10);
  unsigned long      arg12 = VSTR_FMT_CB_ARG_VAL(spec, unsigned long, 11);
  unsigned long long arg13 = VSTR_FMT_CB_ARG_VAL(spec, unsigned long long, 12);
  char              *arg14 = VSTR_FMT_CB_ARG_PTR(spec, 13);
  int                arg15 = VSTR_FMT_CB_ARG_VAL(spec, int, 14);

  if (vstr_add_fmt(st, pos,
                   "%f|%Lf|%d|%jd|%ld|%lld|%td|%ls|%zu|%zd|%ju|%lu|%llu|%s|%c",
                   arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
                   arg11, arg12, arg13, arg14, arg15))
    return (TRUE);

  return (FALSE);
}

int tst(void)
{
  int ret = 0;
  Vstr_ref *ref = NULL;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  vstr_add_fmt(s2, s2->len, "$ %s -- %lu $", buf, (unsigned long)getpid());

  ASSERT(s1->conf->fmt_usr_curly_braces);
  ASSERT(s2->conf->fmt_usr_curly_braces);
  ASSERT(s3->conf->fmt_usr_curly_braces);
  
  vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_cntl_conf(s3->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_cntl_conf(s4->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '%');

  /* setup custom formatters */
  vstr_fmt_add(s1->conf, "{VSTR:%p%u}", tst_usr_vstr_cb,
               VSTR_TYPE_FMT_PTR_VOID,
               VSTR_TYPE_FMT_UINT,
               VSTR_TYPE_FMT_END);
  ASSERT(vstr_fmt_srch(s1->conf, "{VSTR:%p%u}"));
  ASSERT(s1->conf->fmt_usr_curly_braces);

  vstr_fmt_add(s1->conf, "{TST_ALL}", tst_usr_all_cb,
               VSTR_TYPE_FMT_DOUBLE,
               VSTR_TYPE_FMT_DOUBLE_LONG,
               VSTR_TYPE_FMT_INT,
               VSTR_TYPE_FMT_INTMAX_T,
               VSTR_TYPE_FMT_LONG,
               VSTR_TYPE_FMT_LONG_LONG,
               VSTR_TYPE_FMT_PTRDIFF_T,
               VSTR_TYPE_FMT_PTR_WCHAR_T,
               VSTR_TYPE_FMT_SIZE_T,
               VSTR_TYPE_FMT_SSIZE_T,
               VSTR_TYPE_FMT_UINTMAX_T,
               VSTR_TYPE_FMT_ULONG,
               VSTR_TYPE_FMT_ULONG_LONG,
               VSTR_TYPE_FMT_PTR_CHAR,
               VSTR_TYPE_FMT_INT,
               VSTR_TYPE_FMT_END);

  vstr_sc_fmt_add_all(s3->conf);
  ASSERT(s3->conf->fmt_usr_curly_braces);
  vstr_fmt_add(s3->conf, "{BLANK_ERRNO1}", tst_usr_blank_errno1_cb,
               VSTR_TYPE_FMT_ERRNO, VSTR_TYPE_FMT_END);
  ASSERT(s3->conf->fmt_usr_curly_braces);
  vstr_fmt_add(s3->conf, "{BLANK_ERRNO2}", tst_usr_blank_errno2_cb,
               VSTR_TYPE_FMT_ERRNO, VSTR_TYPE_FMT_END);
  ASSERT(s3->conf->fmt_usr_curly_braces);
  vstr_fmt_add(s3->conf, "PID", tst_usr_pid_cb, VSTR_TYPE_FMT_END);
  ASSERT(vstr_fmt_srch(s3->conf, "PID"));
  ASSERT(!s3->conf->fmt_usr_curly_braces);
  
  vstr_fmt_add(s4->conf, "m", tst_usr_strerror_cb, VSTR_TYPE_FMT_END);  

  /* output */
  
  vstr_add_fmt(s1, 0, "${VSTR:%p%u}", (void *)s2, 0);
  
  TST_B_TST(ret, 1, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));

  vstr_add_fmt(s3, 0, "$$ ${vstr:%p%zu%zu%u} -- $PID $$",
               (void *)s2, 3, strlen(buf), 0);
  
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));
  
  vstr_del(s3, 1, s3->len);
  vstr_add_fmt(s3, 0, "$$ ${buf:%s%zu} -- $PID $$",
               buf, strlen(buf));
  
  TST_B_TST(ret, 3, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));

  vstr_del(s3, 1, s3->len);
  vstr_add_fmt(s3, 0, "$$ ${ptr:%s%zu} -- $PID $$",
               buf, strlen(buf));
  
  TST_B_TST(ret, 4, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));

  vstr_del(s3, 1, s3->len);
  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s3, 0, " ${non:%zu} ", strlen(buf));
  vstr_add_rep_chr(s1, s1->len, ' ', 1);
  vstr_add_non(s1, 1, strlen(buf));
  vstr_add_rep_chr(s1, s1->len, ' ', 1);
  
  TST_B_TST(ret, 5, !VSTR_CMP_EQ(s3, 1, s3->len, s1, 1, s1->len));

  vstr_del(s3, 1, s3->len);
  ref = vstr_ref_make_ptr(buf, vstr_ref_cb_free_ref);
  vstr_add_fmt(s3, 0, "$$ $*{ref:%*p%u%zu} -- $PID $$",
               0, ref, 0, strlen(buf));
  vstr_ref_del(ref);
  
  TST_B_TST(ret, 6, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));

  tst_errno = ERANGE;
  
  vstr_del(s4, 1, s4->len);
  vstr_add_fmt(s4, 0, "%d", 1);
  TST_B_TST(ret, 8, !VSTR_CMP_CSTR_EQ(s4, 1, s4->len, "1"));

  vstr_del(s4, 1, s4->len);
  errno = 0;
  vstr_add_fmt(s4, 0, "%m");

  assert(tst_errno == ERANGE);
  
  TST_B_TST(ret, 9, !VSTR_CMP_CSTR_EQ(s4, 1, s4->len, strerror(ERANGE)));

  vstr_del(s3, 1, s3->len);
  errno = tst_errno;
  vstr_add_fmt(s3, 0, "%m");
  
  TST_B_TST(ret, 10, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, strerror(ERANGE)));
  
  vstr_del(s3, 1, s3->len);
  errno = tst_errno;
  vstr_add_fmt(s3, 0,
               "${BLANK_ERRNO1}"
               "%m"
               "${BLANK_ERRNO2}");
  assert(!tst_errno);
  assert(!errno && !tst_errno);
  
  TST_B_TST(ret, 11, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, strerror(ERANGE)));

  vstr_del(s4, 1, s4->len);
  tst_errno = 0;
  errno = ERANGE;
  vstr_add_sysfmt(s4, 0, "%m");
  
  TST_B_TST(ret, 12, !VSTR_CMP_CSTR_EQ(s4, 1, s4->len, strerror(ERANGE)));

  vstr_fmt_del(s1->conf, "{VSTR:%p%u}");
  assert(!vstr_fmt_srch(s1->conf, "{VSTR:%p%u}"));
  vstr_fmt_del(s3->conf, "{vstr:%p%zu%zu%u}");
  assert(!vstr_fmt_srch(s3->conf, "{Vstr:%p%zu%zu%u}"));
  
  {
    const char *t_fmt = "${TST_ALL}";
    const char *correct = ("1.000000|2.000000|"
                           "3|4|5|6|7|"
                           "eight|"
                           "9|10|11|12|13|"
                           "fourteen|");

#ifdef FMT_DBL_none
    correct = ("0.000000|0.000000|"
               "3|4|5|6|7|"
               "eight|"
               "9|10|11|12|13|"
               "fourteen|");
#endif
      
    vstr_del(s1, 1, s1->len);
    vstr_add_fmt(s1, s1->len, t_fmt,
                 1.0, (long double)2.0,
                 3, (intmax_t)4,
                 5L, 6LL,
                 (ptrdiff_t)7,
                 L"eight",
                 (size_t)9, (ssize_t)10, (uintmax_t)11,
                 12UL, 13ULL, "fourteen", 0);
    
    TST_B_TST(ret, 28, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len - 1, correct));
    TST_B_TST(ret, 29, !VSTR_CMP_BUF_EQ(s1, s1->len, 1, "", 1));
  }
  
  return (TST_B_RET(ret));
}
