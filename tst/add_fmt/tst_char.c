#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  sprintf(buf, "%d %d %u %u", SCHAR_MAX, SCHAR_MIN, 0, UCHAR_MAX);
  vstr_add_fmt(s1, 0, "%d %d %u %u", SCHAR_MAX, SCHAR_MIN, 0, UCHAR_MAX);

  return (!VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
}
