#include "../tst-main.c"

static const char *rf = __FILE__;

/* test weird integer corner cases of spec. */
int tst(void)
{
  int ret = 0;
  
  vstr_add_fmt(s1, 0, "%.0d", 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%.d", 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%.*d", 0, 0);
  ret |= s1->len;
  
  vstr_add_fmt(s1, 0, "%.0x", 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%.x", 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%.*x", 0, 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%#.*x", 0, 0);
  ret |= s1->len;
  
  vstr_add_fmt(s1, 0, "%.0o", 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%.o", 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%.*o", 0, 0);
  ret |= s1->len;
  vstr_add_fmt(s1, 0, "%#.o", 0);
  ret |= !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "0");

  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s1, 0, "%#x", 1);
  ret |= !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "0x1");
  
  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s1, 0, "%#x", 0);
  ret |= !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "0");
  
  return (ret);
}
