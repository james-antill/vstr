#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  VSTR_ADD_CSTR_BUF(s1, s1->len, "");
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);
  VSTR_ADD_CSTR_BUF(s1, s1->len, "");

  TST_B_TST(ret,  1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));

  VSTR_ADD_CSTR_BUF(s2, s2->len, "");
  VSTR_ADD_CSTR_BUF(s2, s2->len, buf);
  VSTR_ADD_CSTR_BUF(s2, s2->len, "");

  TST_B_TST(ret,  2, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf));

  VSTR_ADD_CSTR_BUF(s3, s3->len, "");
  VSTR_ADD_CSTR_BUF(s3, s3->len, buf);
  VSTR_ADD_CSTR_BUF(s3, s3->len, "");

  TST_B_TST(ret,  3, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, buf));


  VSTR_ADD_CSTR_BUF(s1, 1, buf);

  TST_B_TST(ret,  5, !VSTR_CMP_CSTR_EQ(s1, 2, s1->len / 2, buf));
  TST_B_TST(ret,  6, !VSTR_CMP_CSTR_EQ(s1, 2 + (s1->len / 2), (s1->len / 2) - 1,
                                       buf + 1));

  VSTR_ADD_CSTR_BUF(s2, 1, buf);

  TST_B_TST(ret,  7, !VSTR_CMP_CSTR_EQ(s2, 2, s2->len / 2, buf));
  TST_B_TST(ret,  8, !VSTR_CMP_CSTR_EQ(s2, 2 + (s2->len / 2), (s2->len / 2) - 1,
                                       buf + 1));

  VSTR_ADD_CSTR_BUF(s3, 1, buf);

  TST_B_TST(ret,  9, !VSTR_CMP_CSTR_EQ(s3, 2, s3->len / 2, buf));  
  TST_B_TST(ret, 10, !VSTR_CMP_CSTR_EQ(s3, 2 + (s3->len / 2), (s3->len / 2) - 1,
                                       buf + 1));
  
  return (TST_B_RET(ret));
}
