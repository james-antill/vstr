#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  unsigned int n3 = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_BUF(s2, 0, buf);
  VSTR_ADD_CSTR_BUF(s3, 0, buf);
  n3 = s3->num;
  
  VSTR_SUB_CSTR_BUF(s1, 1, s1->len, buf);
  VSTR_SUB_CSTR_BUF(s2, 1, s2->len, buf);
  VSTR_SUB_CSTR_BUF(s3, 1, s3->len, buf);

  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf));
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, buf));
  
  TST_B_TST(ret, 4, (s1->num != 1));
  TST_B_TST(ret, 5, (s2->num != 1));
  TST_B_TST(ret, 6, (s3->num != n3));
  
  return (TST_B_RET(ret));
}
