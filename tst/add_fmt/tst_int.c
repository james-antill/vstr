#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  vstr_add_fmt(s1, 0, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  return (!VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
}
