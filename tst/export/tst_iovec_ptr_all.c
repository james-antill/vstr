#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  struct iovec *iov = NULL;
  unsigned int num = 0;
  unsigned int count = 0;
  size_t len = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);

  len = vstr_export_iovec_ptr_all(s1, &iov, &num);

  TST_B_TST(ret, 1, (len != s1->len));
  TST_B_TST(ret, 2, (num != s1->num));

  len = 0;
  while (len < s1->len)
  {
    TST_B_TST(ret, 3, !!memcmp(buf + len,
                               iov[count].iov_base, iov[count].iov_len));
    len += iov[count].iov_len;
    ++count;
  }
  TST_B_TST(ret, 4, (len != s1->len));
  
  return (TST_B_RET(ret));
}
