#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  int mfail_count = 0;

  TST_B_TST(ret, 1, FALSE);

  return (TST_B_RET(ret));
}
