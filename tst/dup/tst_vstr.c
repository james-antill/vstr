#include "../tst-main.c"

static const char *rf = __FILE__;

static int ret = 0;

static void tst_vstr_ci(unsigned int num, Vstr_conf *conf,
                        Vstr_base *t_from, size_t pos, size_t len,
                        unsigned int flags)
{
  Vstr_base *t1 = vstr_dup_vstr(conf, t_from, pos, len, flags);
  
  TST_B_TST(ret, num, !VSTR_CMP_EQ(t1, 1, t1->len, t_from, pos, len));
  
  vstr_free_base(t1);
}

static void tst_vstr_ca(unsigned int num, Vstr_conf *conf,
                        size_t pos, size_t len, unsigned int flags)
{
  tst_vstr_ci(num, conf, s1, pos, len, flags);
  tst_vstr_ci(num, conf, s2, pos, len, flags);
  tst_vstr_ci(num, conf, s3, pos, len, flags);
}

static void tst_vstr_a(unsigned int num,
                       size_t pos, size_t len, unsigned int flags)
{
  tst_vstr_ca(num, s1->conf, pos, len, flags);
  tst_vstr_ca(num, s2->conf, pos, len, flags);
  tst_vstr_ca(num, s3->conf, pos, len, flags);
}

int tst(void)
{   
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);

  VSTR_ADD_CSTR_BUF(s2, 0, buf);
  VSTR_ADD_CSTR_PTR(s2, 0, buf);

  VSTR_ADD_CSTR_BUF(s3, 0, buf);
  VSTR_ADD_CSTR_PTR(s3, 0, buf);

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
