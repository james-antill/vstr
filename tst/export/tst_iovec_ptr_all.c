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

  ret |= (1 << 0) * (len != s1->len);
  ret |= (1 << 1) * (num != s1->num);

  len = 0;
  while (len < s1->len)
  {
    ret |= (1 << 2) * !!memcmp(buf + len,
                               iov[count].iov_base, iov[count].iov_len);
    len += iov[count].iov_len;
    ++count;
  }
  ret |= (1 << 3) * (len != s1->len);
  
  return (ret);
}
