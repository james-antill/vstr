#include "../tst-main.c"

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
  vstr_add_fmt(s3, 0, "$$ ${ref:%p%u%zu} -- $PID $$",
               ref, 0, strlen(buf));
  vstr_ref_del(ref);
  
  TST_B_TST(ret, 6, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));

  tst_errno = ERANGE;
  
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
  
  return (TST_B_RET(ret));
}
