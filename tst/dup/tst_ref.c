#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  static Vstr_ref ref2;
  int ret = 0;
  Vstr_base *t1 = NULL;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  ref2.func = vstr_ref_cb_free_nothing;
  ref2.ptr = buf;
  ref2.ref = 0;

  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_REF(s1->conf, &ref2, 0);

  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));

  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_REF(s2->conf, &ref2, 0);

  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));

  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_REF(s3->conf, &ref2, 0);

  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));

  vstr_free_base(t1);
  t1 = VSTR_DUP_CSTR_REF(s4->conf, &ref2, 0);

  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));

  vstr_free_base(t1);

  return (TST_B_RET(ret));
}
