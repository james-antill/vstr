#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);

  vstr_add_vstr(s2, 0, s1, 1, s1->len, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 1, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, 0, s1, 1, s1->len, VSTR_TYPE_ADD_BUF_PTR);
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, 0, s1, 1, s1->len, VSTR_TYPE_ADD_BUF_REF);
  TST_B_TST(ret, 3, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, 0, s1, 1, s1->len, VSTR_TYPE_ADD_ALL_REF);
  TST_B_TST(ret, 4, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, 0, s1, 1, s1->len, VSTR_TYPE_ADD_ALL_BUF);
  TST_B_TST(ret, 5, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  return (TST_B_RET(ret));
}
