#include "../tst-main.c"

static const char *rf = __FILE__;

static int ret = 0;

static void tst_vstr_i(unsigned int num, Vstr_base *t1,
                       Vstr_base *t_from, size_t pos, size_t len,
                       unsigned int flags)
{
  vstr_del(t_from, 1, t_from->len); /* setup each time for _BUF converters */
  VSTR_ADD_CSTR_BUF(t_from, t_from->len, buf);
  VSTR_ADD_CSTR_PTR(t_from, t_from->len, buf);
  VSTR_ADD_CSTR_PTR(t_from, t_from->len, " xy");
  vstr_del(t_from, 1, strlen(" xy"));
  
  vstr_del(t1, 1, t1->len);

  vstr_add_vstr(t1, t1->len, t_from, 1, 0, flags);
  vstr_add_vstr(t1, t1->len, t_from, pos, len, flags);
  vstr_add_vstr(t1, t1->len, t_from, 2, 0, flags);

  TST_B_TST(ret, num, !VSTR_CMP_EQ(t1, 1, t1->len, t_from, pos, len));
}

static void tst_vstr_a(unsigned int num,
                       size_t pos, size_t len, unsigned int flags)
{
  tst_vstr_i(num, s2, s1, pos, len, flags);
  tst_vstr_i(num, s3, s1, pos, len, flags);
}

int tst(void)
{
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  tst_vstr_a( 1, 1, s1->len, VSTR_TYPE_ADD_DEF);
  tst_vstr_a( 2, 1, s1->len, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_a( 3, 1, s1->len, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_a( 4, 1, s1->len, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_a( 5, 1, s1->len, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_a( 6, 4, 16, VSTR_TYPE_ADD_DEF);
  tst_vstr_a( 7, 4, 16, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_a( 8, 4, 16, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_a( 9, 4, 16, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_a(10, 4, 16, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_a(11, 16, 32, VSTR_TYPE_ADD_DEF);
  tst_vstr_a(12, 16, 32, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_a(13, 16, 32, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_a(14, 16, 32, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_a(15, 16, 32, VSTR_TYPE_ADD_BUF_REF);
  
  return (TST_B_RET(ret));
}
