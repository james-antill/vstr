#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  vstr_add_non(s1, 0, 128);

  return (!VSTR_CMP_BUF_EQ(s1, 1, s1->len, NULL, 128));
}
