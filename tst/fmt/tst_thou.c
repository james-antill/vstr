#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  sprintf(buf, "%'d", (1000 * 1000));
  vstr_add_fmt(s1, 0, "%'d", (1000 * 1000));

  return (!VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
}
