#include "xfail-tst-main.c"

static const char *rf = __FILE__;

void xfail_tst(void)
{
  vstr_add_ref(s1, 0, NULL, 0, 1);
}
