#include "tst-main.c"

static const char *rf = __FILE__;

/* this is a hack to make deleteing look like adding... */
static int tst_ad(Vstr_conf *conf, const char *name)
{
  vstr_fmt_del(conf, name);

  return (TRUE);
}

static void tst_fmt_del(Vstr_conf *conf)
{
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "vstr", "p%zu%zu%u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "buf", "s%zu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ptr", "s%zu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "non", "zu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ref", "p%zu%zu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ref", "p%u%zu", "}"); /* 1.0.0 API */
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "rep_chr", "c%zu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "BKMG.u",   "u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "BKMG/s.u", "u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "bKMG.u",   "u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "bKMG/s.u", "u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "BKMG.ju",   "ju", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "BKMG/s.ju", "ju", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "bKMG.ju",   "ju", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "bKMG/s.ju", "ju", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ipv4.p", "p", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ipv6.p", "p", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ipv4.v", "p", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ipv6.v", "p%u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ipv4.v+C", "p%u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "ipv6.v+C", "p%u%u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "B.u",   "u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "B.lu", "lu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "B.zu", "zu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "B.ju", "ju", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "b.u",   "u", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "b.lu", "lu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "b.zu", "zu", "}");
  VSTR_SC_FMT_ADD(tst_ad, conf, "{", "b.ju", "ju", "}");
}


int tst(void)
{
  int ret = 0;
  int mfail_count = 0;

  mfail_count = 0;
  do
  {
    tst_fmt_del(NULL);
    tst_mfail_num(++mfail_count);
  } while (!vstr_sc_fmt_add_all(NULL));
  tst_mfail_num(0);

  TST_B_TST(ret, 1, !vstr_fmt_srch(NULL, "{vstr}"));
  TST_B_TST(ret, 2, !vstr_fmt_srch(NULL, "{buf}"));
  TST_B_TST(ret, 3, !vstr_fmt_srch(NULL, "{ref}"));
  TST_B_TST(ret, 4, !vstr_fmt_srch(NULL, "{ref:%p%u%zu}"));
  TST_B_TST(ret, 5, !vstr_fmt_srch(NULL, "{ref:%p%zu%zu}"));
  TST_B_TST(ret, 6, !vstr_fmt_srch(NULL, "{rep_chr:%d%c%zu}"));
  TST_B_TST(ret, 7, !vstr_fmt_srch(NULL, "{rep_chr:%d%d%c%zu}"));
  TST_B_TST(ret, 8, !vstr_fmt_srch(NULL, "{B.u}"));
  TST_B_TST(ret, 9, !vstr_fmt_srch(NULL, "{b.ju}"));

  return (TST_B_RET(ret));
}
