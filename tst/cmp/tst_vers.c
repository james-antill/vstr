#include "../tst-main.c"

static const char *rf = __FILE__;

#include "tst-cmp.h"

#define TEST_CMP_VSTR_FUNC(x1, x2, x3, y1, y2, y3) \
        vstr_cmp_vers(x1, x2, x3, y1, y2, y3)
#define TEST_CMP_CSTR_FUNC(x1, x2, x3, y1) \
        VSTR_CMP_VERS_CSTR(x1, x2, x3, y1)

int tst(void)
{
  int ret = 0;
  unsigned int count = 0;
  
  TEST_CMP_EQ_0("",           "");
  TEST_CMP_EQ_0("abcd",       "abcd");
  TEST_CMP_EQ_0("abcd1",      "abcd1");
  TEST_CMP_EQ_0("abcd12345",  "abcd12345");
  TEST_CMP_EQ_0("abcd012345", "abcd012345");
  
  TEST_CMP_GT_0("a",          "");
  TEST_CMP_GT_0("abcd1234",   "abcd123");
  TEST_CMP_GT_0("abcd1234",   "abcd999");
  TEST_CMP_GT_0("abcd9234",   "abcd1234");
  TEST_CMP_GT_0("abcd01234",  "abcd0123");/* 8 */
  TEST_CMP_GT_0("abcd0999",   "abcd01234");
  TEST_CMP_GT_0("abcd0",      "abcd00");
  TEST_CMP_GT_0("abcd00",     "abcd009");
  TEST_CMP_GT_0("abcd09",     "abcd00"); /* 12 */
  TEST_CMP_GT_0("abcd00z",    "abcd00a");
  TEST_CMP_GT_0("abcd09z",    "abcd09a");
  TEST_CMP_GT_0("abcd0",      "abcd");
  TEST_CMP_GT_0("abcd1",      "abcd"); /* 16 */

  VSTR_ADD_CSTR_BUF(s1, 0, "abcd");
  VSTR_ADD_CSTR_BUF(s2, 0, "abcd");
  
  TST_B_TST(ret, 20, !VSTR_CMP_VERS_EQ(s1, 1, 0, s2, 1, 0));
  TST_B_TST(ret, 21, !VSTR_CMP_VERS_EQ(s1, 1, s1->len, s2, 1, s2->len));

  VSTR_ADD_CSTR_BUF(s2, 0, "wxyz");
  
  TST_B_TST(ret, 22, !VSTR_CMP_VERS_EQ(s1, 1, s1->len, s2, 5, 4));
  TST_B_TST(ret, 23, !VSTR_CMP_VERS_EQ(s2, 5, 4, s1, 1, s1->len));

  VSTR_ADD_CSTR_BUF(s2, s2->len, "wxyz");
  
  TST_B_TST(ret, 24, !VSTR_CMP_VERS_EQ(s1, 1, s1->len, s2, 5, 4));
  TST_B_TST(ret, 25, !VSTR_CMP_VERS_EQ(s2, 5, 4, s1, 1, s1->len));

  VSTR_ADD_CSTR_BUF(s1, s1->len, "wxyz");
  VSTR_ADD_CSTR_BUF(s1, 0, "wxyz");
  
  TST_B_TST(ret, 26, !VSTR_CMP_VERS_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_del(s1, 1, 8);
  
  TST_B_TST(ret, 27, !VSTR_CMP_VERS_EQ(s1, 1, s1->len, s2, 1, 4));
  TST_B_TST(ret, 28, !VSTR_CMP_VERS_EQ(s2, 1, 4, s1, 1, s1->len));
  TST_B_TST(ret, 29, !VSTR_CMP_VERS_EQ(s1, 1, s1->len, s2, 9, 4));
  TST_B_TST(ret, 30, !VSTR_CMP_VERS_EQ(s2, 9, 4, s1, 1, s1->len));

  return (TST_B_RET(ret));
}
