#include "../tst-main.c"

static const char *rf = __FILE__;

#include "tst-cmp.h"

#define TEST_CMP_VSTR_FUNC(x1, x2, x3, y1, y2, y3) \
        vstr_cmp_case(x1, x2, x3, y1, y2, y3)
#define TEST_CMP_CSTR_FUNC(x1, x2, x3, y1) \
        VSTR_CMP_CASE_CSTR(x1, x2, x3, y1)

int tst(void)
{
  int ret = 0;
  unsigned int count = 0;

  TEST_CMP_EQ_0("",           "");
  TEST_CMP_EQ_0("AbCd",       "abcd");
  TEST_CMP_EQ_0("AbCd1",      "abcd1");
  TEST_CMP_EQ_0("AbCd12345",  "abcd12345");
  TEST_CMP_EQ_0("AbCd012345", "abcd012345");
  
  TEST_CMP_GT_0("Ab",           "");
  TEST_CMP_GT_0("AbCd1234",   "abcd123");
  TEST_CMP_GT_0("AbCd01234",  "abcd0123");
  TEST_CMP_GT_0("AbCd00",     "abcd0");
  TEST_CMP_GT_0("AbCd009",    "abcd00");
  TEST_CMP_GT_0("AbCd09",     "abcd00");
  TEST_CMP_GT_0("AbCd00z",    "abcd00a");
  TEST_CMP_GT_0("AbCd09z",    "abcd09a");
  TEST_CMP_GT_0("AbCd0",      "abcd");
  TEST_CMP_GT_0("AbCd1",      "abcd");
  
  return (TST_B_RET(ret));
}
