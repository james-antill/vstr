#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  char *ptr = NULL;
  size_t len = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  ptr = strdup(buf);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);

  memset(buf, 'X', sizeof(buf));
  len = vstr_export_cstr_buf(s1, 1, s1->len, buf, sizeof(buf));

  --len;
  
  ret |= (len != s1->len);
  ret |= 2 * !!strcmp(buf, ptr);

  free(ptr);
  
  return (ret);
}
