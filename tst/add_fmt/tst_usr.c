#include "../tst-main.c"

static const char *rf = __FILE__;

#ifndef HAVE_POSIX_HOST
static int getpid(void) { return (0xdeadbeef); }
#endif

static int tst_usr_vstr_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  Vstr_base *sf    = spec->data_ptr[0];
  size_t sf_flags  = *(unsigned int *)(spec->data_ptr[1]);

  assert(!strcmp(spec->name, "{Vstr:%p%u}"));
  
  if (!vstr_add_vstr(st, pos, sf, 1, sf->len, sf_flags))
    return (FALSE);

  return (TRUE);
}

static int tst_usr_pid_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  assert(!strcmp(spec->name, "{PID}"));
  
  if (!vstr_add_fmt(st, pos, "%ld", (long)getpid()))
    return (FALSE);

  return (TRUE);
}

int tst(void)
{
  int ret = 0;

  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_BUF(s2, s2->len, "$ ");
  VSTR_ADD_CSTR_BUF(s2, s2->len, buf);
  vstr_add_fmt(s2, s2->len, " -- %ld $", (long)getpid());

  s1->conf->fmt_usr_escape = '$'; /* FIXME: cntl */
  s3->conf->fmt_usr_escape = '$'; /* FIXME: cntl */
  
  vstr_fmt_add(s1->conf, "{Vstr:%p%u}", tst_usr_vstr_cb,
               VSTR_TYPE_FMT_PTR_VOID,
               VSTR_TYPE_FMT_UINT,
               VSTR_TYPE_FMT_END);
  
  vstr_add_fmt(s1, 0, "${Vstr:%p%u}", (void *)s2, 0);
  
  TST_B_TST(ret, 1, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_sc_fmt_add_vstr(s3->conf, "{Vstr:%p%zu%zu%u}");
  vstr_fmt_add(s3->conf, "{PID}", tst_usr_pid_cb, VSTR_TYPE_FMT_END);
  
  vstr_add_fmt(s3, 0, "$$ ${Vstr:%p%zu%zu%u} -- ${PID} $$",
               (void *)s2, 3, strlen(buf), 0);
  
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));

  return (TST_B_RET(ret));
}
