#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  char *optr = NULL;
  char *ptr = NULL;
  int ret = 0;
  struct iovec *dum1 = NULL;
  unsigned int dum2 = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);

  ptr = vstr_export_cstr_ptr(s1, 1, s1->len);
  
  TST_B_TST(ret, 1, !!strcmp(buf, ptr));

  optr = ptr;
  ptr = vstr_export_cstr_ptr(s1, 4, s1->len - 3);
  
  TST_B_TST(ret, 2, !!strcmp(buf + 3, ptr));
  TST_B_TST(ret, 3, (ptr != (optr + 3)));

  vstr_add_iovec_buf_beg(s1, s1->len, 1, 2, &dum1, &dum2);
  vstr_add_iovec_buf_end(s1, s1->len, 0);
  
  ptr = vstr_export_cstr_ptr(s1, 4, s1->len - 3);
  
  TST_B_TST(ret, 4, !!strcmp(buf + 3, ptr));
  TST_B_TST(ret, 5, (ptr != (optr + 3)));

  /* s2 */
  vstr_add_buf(s2, 0, "a", 1);
  ptr = vstr_export_cstr_ptr(s2, 1, s2->len);
  vstr_del(s2, 1, s2->len);
  
  vstr_add_vstr(s2, 0, s1, 1, s1->len, 0);
  ptr = vstr_export_cstr_ptr(s2, 6, s2->len - 5);
  
  TST_B_TST(ret, 6, !!strcmp(buf + 5, ptr));
  TST_B_TST(ret, 7, (ptr != (optr + 5)));

  vstr_cache_cb_free(s2, 0); /* free all cached stuff */
  
  ptr = vstr_export_cstr_ptr(s2, 6, s2->len - 5);
  
  TST_B_TST(ret, 8, !!strcmp(buf + 5, ptr));
  TST_B_TST(ret, 9, (ptr == (optr + 5)));

  /* s3 */
  vstr_add_buf(s3, 0, "a", 1);
  ptr = vstr_export_cstr_ptr(s3, 1, s3->len);
  vstr_del(s3, 1, s3->len);
  
  buf[s1->len - 1] = 0; /* chop last char from buffer */
  vstr_add_vstr(s3, 0, s1, 1, s1->len - 1, 0);
  
  ptr = vstr_export_cstr_ptr(s3, 6, s3->len - 5);
  
  TST_B_TST(ret, 10, !!strcmp(buf + 5, ptr));
  TST_B_TST(ret, 11, (ptr == (optr + 5)));

  return (TST_B_RET(ret));
}
