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

  ASSERT(spec->fmt_I);

  if (vstr_add_fmt(st, pos,
                   "%f|%Lf|%8d|%8jd|%8ld|%8lld|%8td|%ls|%8zu|%8zd|%8ju|%8lu|%8llu|%s|%c",
                   arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
                   arg11, arg12, arg13, arg14, arg15))
    return (TRUE);

  return (FALSE);
}

static void tst_fmt(int ret, Vstr_base *t1)
{
  int succeeded = FALSE;
  const char *t_fmt = "$I{TST_ALL}";
  const char *correct = ("1.000000|2.000000|"
                         "       3|       4|"
                         "       5|       6|"
                         "       7|"
                         "eight|"
                         "       9|      10|"
                         "      11|      12|"
                         "      13|"
                         "fourteen|");

#ifdef FMT_DBL_none
  correct = ("0.000000|0.000000|"
             "       3|       4|"
             "       5|       6|"
             "       7|"
             "eight|"
             "       9|      10|"
             "      11|      12|"
             "      13|"
             "fourteen|");
#endif

  vstr_del(t1, 1, t1->len);

  if (!MFAIL_NUM_OK)
  {
    succeeded = vstr_add_fmt(t1, t1->len, t_fmt,
                             1.0, (long double)2.0,
                             3, (intmax_t)4,
                             5L, 6LL,
                             (ptrdiff_t)7,
                             L"eight",
                             (size_t)9, (ssize_t)10, (uintmax_t)11,
                             12UL, 13ULL, "fourteen", 0);
    ASSERT(succeeded);
  }
  else
  {
    unsigned long mfail_count = 1;

    vstr_free_spare_nodes(t1->conf,     VSTR_TYPE_NODE_BUF, 1000);

    TST_B_TST(ret, 26, !tst_mfail_num(1));
    TST_B_TST(ret, 27,  vstr_add_fmt(t1, t1->len, t_fmt,
                                     1.0, (long double)2.0,
                                     3, (intmax_t)4,
                                     5L, 6LL,
                                     (ptrdiff_t)7,
                                     L"eight",
                                     (size_t)9, (ssize_t)10, (uintmax_t)11,
                                     12UL, 13ULL, "fourteen", 0));

    while (!succeeded) /* keep trying until we succeed now */
    {
      vstr_free_spare_nodes(t1->conf, VSTR_TYPE_NODE_BUF, 1000);
      tst_mfail_num(++mfail_count);
      succeeded = vstr_add_fmt(t1, t1->len, t_fmt,
                               1.0, (long double)2.0,
                               3, (intmax_t)4,
                               5L, 6LL,
                               (ptrdiff_t)7,
                               L"eight",
                               (size_t)9, (ssize_t)10, (uintmax_t)11,
                               12UL, 13ULL, "fourteen", 0);
    }
    tst_mfail_num(0);
  }

  TST_B_TST(ret, 28, !VSTR_CMP_BUF_EQ(t1, 1, t1->len,
                                      correct, strlen(correct) + 1));
}

#define IPV4(n1, n2) (((n1) << 8) | (n2))

int tst(void)
{
  int ret = 0;
  Vstr_ref *ref = NULL;
  int mfail_count = 0;
  unsigned int ipv4[4];
  unsigned int ipv6[8];

  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  vstr_add_fmt(s2, s2->len, "$ %s -- %lu $", buf, (unsigned long)getpid());

  ASSERT(s1->conf->fmt_usr_curly_braces);
  ASSERT(s2->conf->fmt_usr_curly_braces);
  ASSERT(s3->conf->fmt_usr_curly_braces);

  vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_cntl_conf(s3->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_cntl_conf(s4->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '%');

  /* setup custom formatters */
  /* test for memory failures... */
  do
  {
    tst_mfail_num(++mfail_count);
  } while (!vstr_fmt_add(s1->conf, "{VSTR:%p%u}", tst_usr_vstr_cb,
                         VSTR_TYPE_FMT_PTR_VOID,
                         VSTR_TYPE_FMT_UINT,
                         VSTR_TYPE_FMT_END));
  tst_mfail_num(0);

  ASSERT(!vstr_fmt_add(s1->conf, "{VSTR:%p%u}", tst_usr_vstr_cb,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_UINT,
                       VSTR_TYPE_FMT_END));

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
  vstr_fmt_add(s3->conf, "{TST_ALL}", tst_usr_all_cb,
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
  vstr_add_fmt(s3, 0, "$$ $*{ref:%*p%zu%zu} -- $PID $$",
               0, ref, 0, strlen(buf));
  vstr_ref_del(ref);

  TST_B_TST(ret, 6, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));

  vstr_del(s1, 1, s1->len);
  vstr_del(s3, 1, s3->len);
  vstr_add_fmt(s3, 0, "X${rep_chr:%c%zu}X", '-', 43);
  vstr_add_rep_chr(s1, s1->len, 'X', 1);
  vstr_add_rep_chr(s1, s1->len, '-', 43);
  vstr_add_rep_chr(s1, s1->len, 'X', 1);

  TST_B_TST(ret, 7, !VSTR_CMP_EQ(s3, 1, s3->len, s1, 1, s1->len));

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

  tst_fmt(ret, s1);
  tst_fmt(ret, s3);

  TST_B_TST(ret, 27, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));

  ipv4[0] = ipv4[1] = ipv4[2] = ipv4[3] = 111;
  ipv6[0] = ipv6[1] = ipv6[2] = 0x22;
  ipv6[3] = ipv6[4] = 0;
  ipv6[5] = 0x222;
  ipv6[6] = ipv6[7] = IPV4(111U, 111);

  ref = vstr_ref_make_ptr((char *)"--ref--", vstr_ref_cb_free_ref);
  ASSERT(ref);
  
  vstr_del(s1, 1, s1->len);
  vstr_del(s3, 1, s3->len);

  mfail_count = 0;

  /* allocate fmt nodes... */
  vstr_add_fmt(s3, 0, "%s%s%s%s%.d%s%s%s%s%.d%s%s%s%s%.d%s%s%s%s%.d%s%s%s%s%.d",
               "", "", "", "", 0, "", "", "", "", 0, "", "", "", "", 0,
               "", "", "", "", 0, "", "", "", "", 0);
  do
  {
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_BUF, 1000);
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_PTR, 1000);
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_NON, 1000);
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_REF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_add_fmt(s3, 0,
                         "$16{buf:%s%zu}|"
                         "$16{ptr:%s%zu}|"
                         "$16{non:%zu}|"
                         "$16{ref:%p%zu%zu}|"
                         "$16{rep_chr:%c%zu}|"
                         "$76{ipv4.v+C:%p%u}|"

                         "$76{ipv6.v+C:%p%u%u}|"
                         "$76{ipv6.v+C:%p%u%u}|"
                         "$76{ipv6.v+C:%p%u%u}|"

                         "$76{ipv6.v+C:%p%u%u}|"
                         "$76{ipv6.v+C:%p%u%u}|"
                         "$76{ipv6.v+C:%p%u%u}"
                         "",
                         "--buf--", 7,
                         "--ptr--", 7,
                         7,
                         ref, 0, 7,
                         'x', 8,
                         ipv4, 24,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_ALIGNED, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_COMPACT, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_STD, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_IPV4_ALIGNED, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_IPV4_COMPACT, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_IPV4_STD, 96));
  tst_mfail_num(0);

  vstr_add_fmt(s1, s1->len, "%s|%s|         ",
               "         --buf--", "         --ptr--");
  vstr_add_non(s1, s1->len, 7);
  vstr_add_fmt(s1, s1->len, "|%s|%s|%s|%s|%s|%s|%s|%s|%s",
               "         --ref--", "        xxxxxxxx",
"                                                          111.111.111.111/24",
"                                  0022:0022:0022:0000:0000:0222:6F6F:6F6F/96",
"                                                  22:22:22::222:6F6F:6F6F/96",
"                                               22:22:22:0:0:222:6F6F:6F6F/96",
"                            0022:0022:0022:0000:0000:0222:111.111.111.111/96",
"                                            22:22:22::222:111.111.111.111/96",
"                                         22:22:22:0:0:222:111.111.111.111/96");
  
  TST_B_TST(ret, 28, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));
  
  vstr_del(s1, 1, s1->len);
  vstr_del(s3, 1, s3->len);

  mfail_count = 0;
  
  do
  {
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_BUF, 1000);
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_PTR, 1000);
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_NON, 1000);
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_REF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_add_fmt(s3, 0,
                         "$-16{buf:%s%zu}|"
                         "$-16{ptr:%s%zu}|"
                         "$-16{non:%zu}|"
                         "$-16{ref:%p%zu%zu}|"
                         "$-16{rep_chr:%c%zu}|"
                         "$-76{ipv4.v+C:%p%u}|"

                         "$-76{ipv6.v+C:%p%u%u}"
                         "$-76{ipv6.v+C:%p%u%u}"
                         "$-76{ipv6.v+C:%p%u%u}"

                         "$-76{ipv6.v+C:%p%u%u}"
                         "$-76{ipv6.v+C:%p%u%u}"
                         "$-76{ipv6.v+C:%p%u%u}"
                         "",
                         "--buf--", 7,
                         "--ptr--", 7,
                         7,
                         ref, 0, 7,
                         'x', 8,
                         ipv4, 24,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_ALIGNED, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_COMPACT, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_STD, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_IPV4_ALIGNED, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_IPV4_COMPACT, 96,
                         ipv6, VSTR_TYPE_SC_FMT_CB_IPV6_IPV4_STD, 96));
  tst_mfail_num(0);

  vstr_add_fmt(s1, s1->len, "%s|%s|", "--buf--         ", "--ptr--         ");
  vstr_add_non(s1, s1->len, 7);
  vstr_add_fmt(s1, s1->len, "         |%s|%s|%s|%s%s%s%s%s%s",
               "--ref--         ", "xxxxxxxx        ",
"111.111.111.111/24                                                          ",
"0022:0022:0022:0000:0000:0222:6F6F:6F6F/96                                  ",
"22:22:22::222:6F6F:6F6F/96                                                  ",
"22:22:22:0:0:222:6F6F:6F6F/96                                               ",
"0022:0022:0022:0000:0000:0222:111.111.111.111/96                            ",
"22:22:22::222:111.111.111.111/96                                            ",
"22:22:22:0:0:222:111.111.111.111/96                                         ");

  TST_B_TST(ret, 29, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));

  vstr_ref_del(ref);
  
  /* flush the max name length cache ... */
  vstr_fmt_add(s3->conf, "{ref-xxxxxxxxxxxxxxxxxxxxxxx}",
               tst_usr_pid_cb, VSTR_TYPE_FMT_END);
  vstr_fmt_del(s3->conf, "{ref-xxxxxxxxxxxxxxxxxxxxxxx}");
  
  {
    const char *ptr = NULL;

    vstr_del(s3, 1, s3->len);
    vstr_add_fmt(s3, 0, "${buf:%s%zu}", ptr, (size_t)1000);
    TST_B_TST(ret, 16, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, "(null)"));
    vstr_del(s3, 1, s3->len);
    vstr_add_fmt(s3, 0, "${ptr:%s%zu}", ptr, (size_t)1000);
    TST_B_TST(ret, 17, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, "(null)"));
  }
  
  return (TST_B_RET(ret));
}
