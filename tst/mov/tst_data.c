#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;

  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  /* 1 */
  VSTR_ADD_CSTR_BUF(s2, 0, buf);
  VSTR_ADD_CSTR_BUF(s3, 0, buf);
  
  vstr_mov(s1, 0, s2, 1, s2->len);
  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
  
  vstr_mov(s1, 0, s3, 1, s3->len);
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
  
  /* 2 */
  VSTR_ADD_CSTR_PTR(s2, 0, buf);
  VSTR_ADD_CSTR_PTR(s3, 0, buf);
  
  vstr_mov(s1, 0, s2, 1, s2->len);
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
  
  vstr_mov(s1, 0, s3, 1, s3->len);
  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
  
  /* 3 */
  VSTR_ADD_CSTR_BUF(s2, 0, buf);
  VSTR_ADD_CSTR_BUF(s3, 0, buf);
  
  vstr_mov(s1, 0, s2, 9, s2->len - 8);
  vstr_mov(s1, 0, s2, 1, 8);
  TST_B_TST(ret, 5, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
  
  vstr_mov(s1, 0, s3, 9, s3->len - 8);
  vstr_mov(s1, 0, s3, 1, 8);
  TST_B_TST(ret, 6, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);

  /* 4 */
  VSTR_ADD_CSTR_PTR(s2, 0, buf);
  VSTR_ADD_CSTR_PTR(s3, 0, buf);
  
  vstr_mov(s1, 0, s2, 9, s2->len - 8);
  vstr_mov(s1, 0, s2, 1, 8);
  TST_B_TST(ret, 5, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  /* no delete */

  TST_B_TST(ret, 7, !vstr_mov(s1, 0, s1, 1, s1->len));
  TST_B_TST(ret, 8, !vstr_mov(s1, 8, s1, 1, s1->len));
  TST_B_TST(ret, 9, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);

  VSTR_ADD_CSTR_PTR(s1, 0, buf);
  
  TST_B_TST(ret, 10, !vstr_mov(s1, 4, s1, 8, s1->len - 7));
  TST_B_TST(ret, 11, !vstr_mov(s3, s3->len, s3, 5, 3));
  TST_B_TST(ret, 12, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));

  TST_B_TST(ret, 13, !vstr_mov(s3, 4, s3, s3->len - 2, 3));
  TST_B_TST(ret, 13, !vstr_mov(s1, 4, s1, s1->len - 2, 3));
  TST_B_TST(ret, 15, !VSTR_CMP_EQ(s1, 1, s1->len, s3, 1, s3->len));
  TST_B_TST(ret, 16, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));

  return (TST_B_RET(ret));
}
