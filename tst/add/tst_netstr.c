#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t pos = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  vstr_add_fmt(s2, s2->len, "%d:", strlen(buf));
  VSTR_ADD_CSTR_PTR(s2, s2->len, buf);
  VSTR_ADD_CSTR_PTR(s2, s2->len, ",");

  pos = vstr_add_netstr_beg(s1, s1->len);
  VSTR_ADD_CSTR_PTR(s1, s1->len, buf);  
  vstr_add_netstr_end(s1, pos, s1->len);

  TST_B_TST(ret, 1,
            !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));

  vstr_del(s1, 1, s1->len);
  
  pos = vstr_add_netstr_beg(s1, s1->len);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);  
  vstr_add_netstr_end(s1, pos, s1->len);

  TST_B_TST(ret, 2,
            !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));

  return (TST_B_RET(ret));
}
