#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  Vstr_base *t1 = NULL;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_BUF(s1->conf, buf);
  
  TST_B_TST(ret,  1, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));

  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_BUF(s2->conf, buf);

  TST_B_TST(ret,  2, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));

  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_BUF(s3->conf, buf);

  TST_B_TST(ret,  3, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));
  
  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_BUF(s1->conf, "");

  TST_B_TST(ret,  4, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, ""));
  
  vstr_free_base(t1);
  
  return (TST_B_RET(ret));
}
