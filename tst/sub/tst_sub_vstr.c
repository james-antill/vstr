#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);
  
  vstr_sub_vstr(s2, 1, s2->len, s1, 1, s1->len, VSTR_TYPE_SUB_DEF);
  TST_B_TST(ret,  1, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_sub_vstr(s2, 1, s2->len, s1, 1, s1->len, VSTR_TYPE_SUB_BUF_PTR);
  TST_B_TST(ret,  2, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_sub_vstr(s2, 1, s2->len, s1, 1, s1->len, VSTR_TYPE_SUB_BUF_REF);
  TST_B_TST(ret,  3, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_sub_vstr(s2, 1, s2->len, s1, 1, s1->len, VSTR_TYPE_SUB_ALL_REF);
  TST_B_TST(ret,  4, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_sub_vstr(s2, 1, s2->len, s1, 1, s1->len, VSTR_TYPE_SUB_ALL_BUF);
  TST_B_TST(ret,  5, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_sub_vstr(s2, 1, s2->len, s1, 1, s1->len, VSTR_TYPE_SUB_DEF);
  TST_B_TST(ret,  6, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  TST_B_TST(ret,  7, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, 
                                       vstr_export_cstr_ptr(s1, 1, s1->len)));
  
  TST_B_TST(ret,  8, (vstr_export_cstr_ptr(s1, 1, s1->len) !=
                      vstr_export_cstr_ptr(s1, 1, s1->len)));
    
  memcpy(buf, "abcd", 4); /* alter the data at ptr */
  vstr_cache_cb_sub(s1, strlen(buf) + 1, strlen(buf));
  vstr_cache_cb_sub(s2, strlen(buf) + 1, strlen(buf)); /* should be nop */
  
  TST_B_TST(ret,  9, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, 
                                       vstr_export_cstr_ptr(s1, 1, s1->len)));

  TST_B_TST(ret, 10,
            !vstr_sub_vstr(s2, 1, s2->len, s3, 1, s3->len, VSTR_TYPE_SUB_DEF));
  TST_B_TST(ret, 11, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));
  
  TST_B_TST(ret, 12,
            !vstr_sub_vstr(s2, 1, s2->len, s3, 1, s3->len, VSTR_TYPE_SUB_DEF));
  TST_B_TST(ret, 13, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));
  
  return (TST_B_RET(ret));
}
