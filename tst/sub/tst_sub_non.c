#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s2, 0, buf);

  vstr_sub_non(s1, 2, s1->len - 2, 2);

  TST_B_TST(ret, 1, !VSTR_CMP_BUF_EQ(s1, 2, s1->len - 2, NULL, 2));
  
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s1, 1, 1, s2, 1, 1));
  TST_B_TST(ret, 3, !VSTR_CMP_EQ(s1, s1->len, 1, s2, s2->len, 1));
    
  vstr_sub_non(s1, 1, 1, 1);
  vstr_sub_non(s1, s1->len, 1, 1);

  TST_B_TST(ret, 4, !VSTR_CMP_BUF_EQ(s1, 1, s1->len, NULL, 4));
  
  return (TST_B_RET(ret));
}
