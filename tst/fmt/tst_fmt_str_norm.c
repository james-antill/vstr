#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  const char *t_fmt = "";
  const char *t_str = NULL;
  const wchar_t *t_wstr = NULL;
  
  vstr_add_fmt(s1, 0, t_fmt);
  vstr_add_fmt(s1, 0, "%.*s %.*ls", -1, t_str, -3, t_wstr);
  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "(null) (null)"));

  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s1, 0, "%-3s%3s%3.1s%-3.1s%.s", "a", "b", "cdef", "def", "abcd");

  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "a    bcd"));
  
  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s1, 0, "%-3ls%3ls%3.1ls%-3.1ls%.ls",
               L"a", L"b", L"cdef", L"def", L"abcd");
  
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "a    bcd"));

  return (TST_B_RET(ret));
}
