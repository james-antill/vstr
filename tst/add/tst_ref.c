#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  static Vstr_ref ref;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  ref.ptr = buf;
  ref.func = vstr_ref_cb_free_nothing;
  ref.ref = 0;
  VSTR_ADD_CSTR_REF(s1, 0, &ref, 0);

  return (!VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
}
