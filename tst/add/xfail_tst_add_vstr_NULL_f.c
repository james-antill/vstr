#include "xfail-tst-main.c"

static const char *rf = __FILE__;

void xfail_tst(void)
{
  vstr_add_vstr(s1, 0, NULL, 1, 1, 0);
}
