#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t off = 0;
  Vstr_ref *ref = NULL;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);

  ref = vstr_export_cstr_ref(s1, 1, s1->len, &off);

  ret |= !!strcmp(buf, ref->ptr + off);

  vstr_ref_del(ref);
  
  return (ret);
}
