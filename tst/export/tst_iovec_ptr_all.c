#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  struct iovec *iov = NULL;
  struct iovec *s3iov = NULL;
  unsigned int num = 0;
  unsigned int count = 0;
  size_t len = 0;  
  
  {
    int tmp = 0;
    
    ASSERT(!!vstr_cntl_conf(s3->conf,
                            VSTR_CNTL_CONF_GET_NUM_IOV_MIN_OFFSET, &tmp) &&
           tmp == 0);
    ASSERT(!!vstr_cntl_conf(s3->conf,
                            VSTR_CNTL_CONF_SET_NUM_IOV_MIN_OFFSET, 4));
    ASSERT(!!vstr_cntl_conf(s3->conf,
                            VSTR_CNTL_CONF_GET_NUM_IOV_MIN_OFFSET, &tmp) &&
           tmp == 4);
  }

  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_PTR(s1, 0, buf);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  vstr_add_vstr(s3, 0, s1, 1, s1->len, 0);

  vstr_export_cstr_buf(s1, 1, s1->len, buf, sizeof(buf));
  
  len = vstr_export_iovec_ptr_all(s1, &iov, &num);

  TST_B_TST(ret, 1, (len != s1->len));
  TST_B_TST(ret, 2, (num != s1->num));

  {
    size_t s3len = 0;
    unsigned int s3num = 0;
    
    s3len = vstr_export_iovec_ptr_all(s3, &s3iov, &s3num);
  
    TST_B_TST(ret, 3, (s3len != len));
    TST_B_TST(ret, 4, (s3num != s3->num));
  }
  
  len = 0;
  while (len < s1->len)
  {
    TST_B_TST(ret, 5, !!memcmp(buf + len,
                               iov[count].iov_base, iov[count].iov_len));
    len += iov[count].iov_len;
    ++count;
  }
  TST_B_TST(ret, 6, (len != s1->len));
  TST_B_TST(ret, 6, (count != s1->num));

  count = 0;
  len = 0;
  iov = s3iov;
  while (len < s3->len)
  {
    TST_B_TST(ret, 7, !!memcmp(buf + len,
                               iov[count].iov_base, iov[count].iov_len));
    len += iov[count].iov_len;
    ++count;
  }
  TST_B_TST(ret, 8, (len != s3->len));
  TST_B_TST(ret, 8, (count != s3->num));
  
  TST_B_TST(ret, 10, !s3->iovec_upto_date);
  vstr_add_ptr(s3, 0, "abcd", 4);
  vstr_add_ptr(s3, 0, "abcd", 4);
  vstr_add_ptr(s3, 0, "abcd", 4);
  vstr_add_rep_chr(s3, 0, 'X', s3->conf->buf_sz);
  TST_B_TST(ret, 11, !s3->iovec_upto_date);

  return (TST_B_RET(ret));
}
