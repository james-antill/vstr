#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  Vstr_base *t1 = NULL;
  const char *const cb1 = "aaaa";
  const char *const cb2 = "XXXXXXXX";

  t1 = vstr_dup_rep_chr(s1->conf, 'a', 8);

  TST_B_TST(ret,  1, !VSTR_CMP_CSTR_EQ(t1,  1, 4, cb1));
  TST_B_TST(ret,  2, !VSTR_CMP_CSTR_EQ(t1,  5, 4, cb1));

  vstr_free_base(t1);
  t1 = vstr_dup_rep_chr(s1->conf, 'X', 8);

  TST_B_TST(ret,  3, !VSTR_CMP_CSTR_EQ(t1,  1, t1->len, cb2));

  vstr_free_base(t1);
  


  t1 = vstr_dup_rep_chr(s2->conf, 'a', 8);

  TST_B_TST(ret,  4, !VSTR_CMP_CSTR_EQ(t1,  1, 4, cb1));
  TST_B_TST(ret,  5, !VSTR_CMP_CSTR_EQ(t1,  5, 4, cb1));

  vstr_free_base(t1);
  t1 = vstr_dup_rep_chr(s2->conf, 'X', 8);

  TST_B_TST(ret,  6, !VSTR_CMP_CSTR_EQ(t1,  1, t1->len, cb2));

  vstr_free_base(t1);
  


  t1 = vstr_dup_rep_chr(s3->conf, 'a', 8);

  TST_B_TST(ret,  7, !VSTR_CMP_CSTR_EQ(t1,  1, 4, cb1));
  TST_B_TST(ret,  8, !VSTR_CMP_CSTR_EQ(t1,  5, 4, cb1));

  vstr_free_base(t1);
  t1 = vstr_dup_rep_chr(s3->conf, 'X', 8);

  TST_B_TST(ret,  9, !VSTR_CMP_CSTR_EQ(t1,  1, t1->len, cb2));

  vstr_free_base(t1);
  


  t1 = vstr_dup_rep_chr(s4->conf, 'a', 8);

  TST_B_TST(ret, 10, !VSTR_CMP_CSTR_EQ(t1,  1, 4, cb1));
  TST_B_TST(ret, 11, !VSTR_CMP_CSTR_EQ(t1,  5, 4, cb1));

  vstr_free_base(t1);
  t1 = vstr_dup_rep_chr(s4->conf, 'X', 8);

  TST_B_TST(ret, 12, !VSTR_CMP_CSTR_EQ(t1,  1, t1->len, cb2));

  vstr_free_base(t1);  
  
  return (TST_B_RET(ret));
}
