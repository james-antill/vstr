#include "../tst-main.c"

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

  while (s1->len > 1) vstr_del(s1, 2, 1);
  vstr_del(s1, 1, 1);
  while (s2->len > 1) vstr_del(s2, 2, 1);
  vstr_del(s2, 1, 1);
  while (s3->len > 1) vstr_del(s3, 2, 1);
  vstr_del(s3, 1, 1);
  
  return (s1->len || s2->len || s3->len);
}
