#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  VSTR_ADD_CSTR_BUF(s1, s1->len, "abcd 2xyz\n\0010");
  VSTR_ADD_CSTR_PTR(s1, s1->len, "abcd 2xyz\n\0010");

  VSTR_ADD_CSTR_BUF(s2, s2->len, "abcd%202xyz%0A%010");
  VSTR_ADD_CSTR_PTR(s2, s2->len, "abcd%202xyz%0A%010");
  
  vstr_conv_encode_uri(s1, 1, s1->len);
 
 return (!VSTR_CMP_CASE_EQ(s1, 1, s1->len, s2, 1, s2->len));
}
