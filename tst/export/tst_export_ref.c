#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t off = 0;
  Vstr_ref *ref = NULL;
  const char *ptr = NULL;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  
  VSTR_ADD_CSTR_BUF(s1, 0, buf);

  TST_B_TST(ret, 1, !(ref = vstr_export_ref(s1, 1, s1->len, &off)));

  TST_B_TST(ret, 2, memcmp(buf, ((char *)ref->ptr) + off, s1->len));

  vstr_ref_del(ref);

  ptr = vstr_export_cstr_ptr(s1, 1, s1->len);
  TST_B_TST(ret, 5, !(ref = vstr_export_ref(s1, 1, s1->len, &off)));
  TST_B_TST(ret, 6,  (ref->ptr != ptr));
  TST_B_TST(ret, 7,   off);

  vstr_add_ref(s2, 0, ref, off, s1->len);

  {
    Vstr_ref *ref2 = NULL;
    
    TST_B_TST(ret, 8, !(ref2 = vstr_export_ref(s2, 1, s2->len, &off)));
    TST_B_TST(ret, 9,  (ref2->ptr != ref->ptr));
    
    vstr_ref_del(ref2);
  }
  vstr_ref_del(ref);
  
  return (TST_B_RET(ret));
}
