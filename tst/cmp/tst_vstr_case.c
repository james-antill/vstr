#include "../tst-main.c"

static const char *rf = __FILE__;

#include "tst-cmp.h"

#define TEST_CMP_FUNC vstr_cmp_case

int tst(void)
{
  TEST_CMP_EQ_0("abcd", "abcd");
  TEST_CMP_EQ_0("abcd1", "abcd1");
  TEST_CMP_EQ_0("abcd12345", "abcd12345");
  TEST_CMP_EQ_0("abcd012345", "abcd012345");
  
  return (0);
}
