#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t off = 0;
  Vstr_ref *ref = NULL;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_BUF(s4, 0, buf);

  ref = vstr_export_cstr_ref(s1, 1, s1->len, &off);

  TST_B_TST(ret, 1, strcmp(buf, ((char *)ref->ptr) + off));

  vstr_ref_del(ref);

  ref = vstr_export_cstr_ref(s1, 2, s1->len - 1, &off);

  TST_B_TST(ret, 2, strcmp(buf + 1, ((char *)ref->ptr) + off));

  vstr_ref_del(ref);

  /* no cache */
  
  ref = vstr_export_cstr_ref(s4, 1, s4->len, &off);
  
  TST_B_TST(ret, 3, strcmp(buf, ((char *)ref->ptr) + off));
  
  vstr_ref_del(ref);

  return (TST_B_RET(ret));
}
