#include "tst-main.c"

static const char *rf = __FILE__;

#define ADD(x) \
  VSTR_ADD_CSTR_BUF(x, 0, buf); \
  VSTR_ADD_CSTR_PTR(x, 0, buf); \
  VSTR_ADD_CSTR_BUF(x, 0, buf); \
  VSTR_ADD_CSTR_PTR(x, 0, buf)

int tst(void)
{
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  ADD(s1);
  ADD(s2);
  ADD(s3);

  vstr_del(s1, 1, s1->len);
  vstr_del(s2, 1, s2->len);
  vstr_del(s3, 1, s3->len);

  return (s1->len || s2->len || s3->len);
}
