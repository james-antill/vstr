#include "tst-main.c"

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

  vstr_add_vstr(t1, t1->len, t_from,   1,   0, flags);
  vstr_add_vstr(t1, t1->len, t_from, pos, len, flags);
  vstr_add_vstr(t1, t1->len, t_from,   2,   0, flags);

  TST_B_TST(ret, num, !VSTR_CMP_EQ(t1, 1, t1->len, t_from, pos, len));
}

static void tst_vstr_a(unsigned int num,
                       size_t pos, size_t len, unsigned int flags)
{
  tst_vstr_i(num, s2, s1, pos, len, flags);
  tst_vstr_i(num, s3, s1, pos, len, flags);
}

static void tst_vstr_b(unsigned int num, Vstr_base *t1, unsigned int flags)
{
  size_t len = 0;
  
  vstr_del(t1, 1, t1->len);
  VSTR_ADD_CSTR_BUF(t1, t1->len, buf);
  VSTR_ADD_CSTR_PTR(t1, t1->len, buf);  

  len = t1->len / 2;
  
  vstr_add_vstr(t1, t1->len, t1, 1, t1->len, flags);
  vstr_del(t1, 8, len * 2);
  vstr_add_vstr(t1, len, t1, 1, len, flags);
  vstr_del(t1, 8, len);
  vstr_add_vstr(t1, 7, t1, len + 1, len, flags);
  vstr_del(t1, 8, len);
  vstr_add_vstr(t1, 7, t1, 2, len, flags);
  vstr_del(t1, 2, len * 2);
  
  TST_B_TST(ret, num, !VSTR_CMP_CSTR_EQ(t1, 1, t1->len, buf));
}

int tst(void)
{
  size_t len = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  len = strlen(buf);
  
  tst_vstr_a( 1, 1, s1->len, VSTR_TYPE_ADD_DEF);
  tst_vstr_a( 2, 1, s1->len, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_a( 3, 1, s1->len, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_a( 4, 1, s1->len, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_a( 5, 1, s1->len, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_a( 6, len / 2, 4, VSTR_TYPE_ADD_DEF);
  tst_vstr_a( 7, len / 2, 4, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_a( 8, len / 2, 4, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_a( 9, len / 2, 4, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_a(10, len / 2, 4, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_a(11, len / 2, len, VSTR_TYPE_ADD_DEF);
  tst_vstr_a(12, len / 2, len, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_a(13, len / 2, len, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_a(14, len / 2, len, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_a(15, len / 2, len, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_a(16, len + len / 2, 4, VSTR_TYPE_ADD_DEF);
  tst_vstr_a(17, len + len / 2, 4, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_a(18, len + len / 2, 4, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_a(19, len + len / 2, 4, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_a(20, len + len / 2, 4, VSTR_TYPE_ADD_BUF_REF);

  tst_vstr_b(21, s1, VSTR_TYPE_ADD_DEF);
  tst_vstr_b(21, s1, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_b(21, s1, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_b(21, s1, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_b(21, s1, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_b(22, s2, VSTR_TYPE_ADD_DEF);
  tst_vstr_b(22, s2, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_b(22, s2, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_b(22, s2, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_b(22, s2, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_b(23, s3, VSTR_TYPE_ADD_DEF);
  tst_vstr_b(23, s3, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_b(23, s3, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_b(23, s3, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_b(23, s3, VSTR_TYPE_ADD_BUF_REF);
  
  tst_vstr_b(24, s4, VSTR_TYPE_ADD_DEF);
  tst_vstr_b(24, s4, VSTR_TYPE_ADD_BUF_PTR);
  tst_vstr_b(24, s4, VSTR_TYPE_ADD_ALL_REF);
  tst_vstr_b(24, s4, VSTR_TYPE_ADD_ALL_BUF);
  tst_vstr_b(24, s4, VSTR_TYPE_ADD_BUF_REF);
  
  {
    const char *p1 = vstr_export_cstr_ptr(s1, 1, s1->len);
    const char *p2 = vstr_export_cstr_ptr(s2, 1, s2->len);
    const char *p1_save = p1;

    ASSERT(p1 != p2);
    
    vstr_del(s2, 1, s2->len);
    vstr_add_vstr(s2, 0, s1, 1, s1->len, 0);

    p1 = vstr_export_cstr_ptr(s1, 1, s1->len);
    p2 = vstr_export_cstr_ptr(s2, 1, s2->len);
    
    TST_B_TST(ret, 31, p1 != p1_save);
    TST_B_TST(ret, 31, p1 != p2);
  }
  
  return (TST_B_RET(ret));
}
