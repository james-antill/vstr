#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  char *ptr = NULL;
  struct iovec iov[1];
  unsigned int num = 0;
  size_t len = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  ptr = strdup(buf);

  VSTR_ADD_CSTR_BUF(s1, 0, buf);

  iov[0].iov_base = buf;
  iov[0].iov_len = sizeof(buf);
  
  memset(buf, 'X', sizeof(buf));
  len = vstr_export_iovec_cpy_buf(s1, 1, s1->len, iov, 1, &num);

  ret |= (1 << 0) * (len != s1->len);
  ret |= (1 << 1) * (num != 1);

  ret |= (1 << 2) * !!memcmp(buf, iov[0].iov_base, len);

  free(ptr);
  
  return (ret);
}
