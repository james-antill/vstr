#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  struct iovec *iovs = NULL;
  size_t blen = 0;
  size_t len = 0;
  unsigned int num = 0;
  unsigned int scan = 0;
  const char *bscan = buf;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  blen = strlen(buf);
  
  len = vstr_add_iovec_buf_beg(s1, 0, 4, 32, &iovs, &num);
  if (!len)
    return (1);
  if (len < blen)
    return (2);
  len = blen;
  
  while (blen && (scan < num))
  {
    size_t tmp = iovs[scan].iov_len;

    if (tmp > blen)
      tmp = blen;

    memcpy(iovs[scan].iov_base, bscan, tmp);

    bscan += tmp;
    blen -= tmp;

    if (!blen)
      break;
  }
  
  vstr_add_iovec_buf_end(s1, 0, len);
  
  return (!VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
}
