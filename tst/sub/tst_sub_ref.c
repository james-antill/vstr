#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  static Vstr_ref ref;
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);

  ref.ptr = buf;
  ref.func = vstr_ref_cb_free_nothing;
  ref.ref = 0;

  VSTR_SUB_CSTR_REF(s1, 1, s1->len, &ref, 0);
  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  
  VSTR_SUB_CSTR_REF(s1, 1, s1->len, &ref, 0);
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  
  VSTR_SUB_CSTR_REF(s1, 1, s1->len, &ref, 1);
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf + 1));
  
  vstr_del(s1, 1, s1->len);
  VSTR_ADD_CSTR_BUF(s1, s1->len, "abcd");
  
  vstr_sub_cstr_ref(s1, 1, s1->len, &ref, 0);
  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  
  return (TST_B_RET(ret));
}
