#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  Vstr_base *t1 = vstr_dup_non(s1->conf, 128);

  TST_B_TST(ret, 1, !VSTR_CMP_BUF_EQ(t1, 1, t1->len, NULL, 128));

  vstr_free_base(t1);
  
  return (TST_B_RET(ret));
}
