#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_BUF(s2, 0, buf);
  VSTR_ADD_CSTR_BUF(s3, 0, buf);

  VSTR_SUB_CSTR_BUF(s1, 1, s1->len, buf);
  VSTR_SUB_CSTR_BUF(s2, 1, s2->len, buf);
  VSTR_SUB_CSTR_BUF(s3, 1, s3->len, buf);

  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf));
  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, buf));
  
  return (TST_B_RET(ret));
}
