#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  char *ptr = NULL;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);

  ptr = vstr_export_cstr_ptr(s1, 1, s1->len);
  
  return (!!strcmp(buf, ptr));
}
