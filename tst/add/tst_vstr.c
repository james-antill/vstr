#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);

  /* FIXME: more options */
  vstr_add_vstr(s2, 0, s1, 1, s1->len, 0);
  
  return (!VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
}
