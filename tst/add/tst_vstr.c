#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);

  /* test s2 */
  vstr_add_vstr(s2, s2->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s2, s2->len, s1, 1, s1->len, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s2, s2->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 1, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, s2->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s2, s2->len, s1, 1, s1->len, VSTR_TYPE_ADD_BUF_PTR);
  vstr_add_vstr(s2, s2->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, s2->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s2, s2->len, s1, 1, s1->len, VSTR_TYPE_ADD_BUF_REF);
  vstr_add_vstr(s2, s2->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 3, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, s2->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s2, s2->len, s1, 1, s1->len, VSTR_TYPE_ADD_ALL_REF);
  vstr_add_vstr(s2, s2->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 4, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, s2->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s2, s2->len, s1, 1, s1->len, VSTR_TYPE_ADD_ALL_BUF);
  vstr_add_vstr(s2, s2->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 5, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  vstr_del(s2, 1, s2->len);
  
  /* test s3 */
  vstr_add_vstr(s3, s3->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s3, s3->len, s1, 1, s1->len, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s3, s3->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 6, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));
  vstr_del(s3, 1, s3->len);
  
  vstr_add_vstr(s3, s3->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s3, s3->len, s1, 1, s1->len, VSTR_TYPE_ADD_BUF_PTR);
  vstr_add_vstr(s3, s3->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 7, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));
  vstr_del(s3, 1, s3->len);
  
  vstr_add_vstr(s3, s3->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s3, s3->len, s1, 1, s1->len, VSTR_TYPE_ADD_BUF_REF);
  vstr_add_vstr(s3, s3->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 8, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));
  vstr_del(s3, 1, s3->len);
  
  vstr_add_vstr(s3, s3->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s3, s3->len, s1, 1, s1->len, VSTR_TYPE_ADD_ALL_REF);
  vstr_add_vstr(s3, s3->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret, 9, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));
  vstr_del(s3, 1, s3->len);
  
  vstr_add_vstr(s3, s3->len, s1, 1, 0, VSTR_TYPE_ADD_DEF);
  vstr_add_vstr(s3, s3->len, s1, 1, s1->len, VSTR_TYPE_ADD_ALL_BUF);
  vstr_add_vstr(s3, s3->len, s1, 2, 0, VSTR_TYPE_ADD_DEF);
  TST_B_TST(ret,10, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));
  vstr_del(s3, 1, s3->len);
  
  return (TST_B_RET(ret));
}
