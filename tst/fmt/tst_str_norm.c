#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  sprintf(buf, "%s", "abcd");
  vstr_add_fmt(s1, 0, "%s", "abcd");

  return (!VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
}
