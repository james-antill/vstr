#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t pos = 0;
  int mfail_count = 0;
  
  sprintf(buf, "X%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  vstr_add_fmt(s2, s2->len, "%zu:", strlen(buf));
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

  vstr_del(s1, 1, s1->len);
  pos = vstr_add_netstr_beg(s1, s1->len);
  vstr_add_netstr_end(s1, pos, s1->len);

  TST_B_TST(ret, 3,
            !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "0:,"));
  
  do
  {
    TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "0:,"));
    vstr_free_spare_nodes(s1->conf, VSTR_TYPE_NODE_BUF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!(pos = vstr_add_netstr_beg(s1, s1->len)));
  tst_mfail_num(0);

  vstr_add_ptr(s1, s1->len, "a", 1);
  vstr_add_vstr(s4, s4->len, s1, 1, s1->len, 0);

  do
  {
    TST_B_TST(ret, 5, !VSTR_CMP_EQ(s1, 1, s1->len, s4, 1, s4->len));
    vstr_free_spare_nodes(s1->conf, VSTR_TYPE_NODE_BUF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_add_netstr_end(s1, pos, s1->len));
  tst_mfail_num(0);

  TST_B_TST(ret, 6,
            !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "0:,1:a,"));
  
  return (TST_B_RET(ret));
}
