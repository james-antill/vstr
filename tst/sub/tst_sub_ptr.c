#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);

  VSTR_SUB_CSTR_PTR(s1, 1, s1->len, buf);
  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));

  VSTR_SUB_CSTR_PTR(s1, 1, s1->len, buf);
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));  

  VSTR_SUB_CSTR_PTR(s1, 1, s1->len, buf + 1);
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf + 1));  

  vstr_del(s1, 1, s1->len);
  vstr_add_cstr_buf(s1, s1->len, "abcd");

  vstr_sub_cstr_ptr(s1, 1, s1->len, buf);
  TST_B_TST(ret, 4, !vstr_cmp_cstr_eq(s1, 1, s1->len, buf));

  return (TST_B_RET(ret));
}
