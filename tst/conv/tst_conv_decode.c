#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  VSTR_ADD_CSTR_BUF(s1, s1->len, "abcd xyz\n");
  VSTR_ADD_CSTR_PTR(s1, s1->len, "abcd xyz\n");

  VSTR_ADD_CSTR_BUF(s2, s2->len, "abcd%20xyz%0A");
  VSTR_ADD_CSTR_PTR(s2, s2->len, "abcd%20xyz%0a");
  
  vstr_conv_decode_uri(s2, 1, s2->len);
  
  return (!VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
}
