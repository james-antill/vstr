#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  int mfail_count = 0;

  vstr_cntl_conf(s3->conf, VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_SEP, "__--==--__");
  vstr_cntl_conf(s3->conf, VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_GRP, "\3");

  do
  {
    ASSERT(!s2->len);
    vstr_free_spare_nodes(s2->conf, VSTR_TYPE_NODE_BUF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!VSTR_SC_ADD_CSTR_GRPNUM_BUF(s2, 0, "AXXXYYYZZZ"));
  
  mfail_count = 0;
  do
  {
    ASSERT(!s3->len);
    vstr_free_spare_nodes(s3->conf, VSTR_TYPE_NODE_BUF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_sc_add_cstr_grpnum_buf(s3, 0, "AXXXYYYZZZ"));

  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, "A,XXX,YYY,ZZZ"));
  TST_B_TST(ret, 2,
            !VSTR_CMP_CSTR_EQ(s3, 1, s3->len,
                              "A__--==--__XXX__--==--__YYY__--==--__ZZZ"));

  return (TST_B_RET(ret));
}
