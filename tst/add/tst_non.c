#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  vstr_add_non(s1, s1->len, 0);
  vstr_add_non(s1, s1->len, 128);
  vstr_add_non(s1, s1->len, 0);

  return (!VSTR_CMP_BUF_EQ(s1, 1, s1->len, NULL, 128));
}
