#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  Vstr_ref *ref = NULL;

  vstr_ref_del(NULL);

  ref = vstr_ref_make_malloc(4);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr("", vstr_ref_cb_free_ref);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(malloc(1), vstr_ref_cb_free_ptr_ref);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  ref = vstr_ref_make_memdup(&ref, sizeof(Vstr_ref *));
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(*(Vstr_ref **)(ref->ptr));
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(malloc(1), vstr_ref_cb_free_ptr);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(ref);
  free(ref); /* cb only free's the ref->ptr */

  ref = vstr_ref_make_strdup("abcd");
  vstr_ref_add(ref);
  if (strcmp("abcd", ref->ptr))
    ASSERT(FALSE);
  if ("abcd" == ref->ptr)
    ASSERT(FALSE);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  tst_mfail_num(1); /* test no check error */
  ref = vstr_ref_make_strdup("abcd");
  ASSERT(!ref);
  tst_mfail_num(0);

  ref = VSTR_REF_MAKE_STRDUP("abcd");
  vstr_ref_add(ref);
  if (strcmp("abcd", ref->ptr))
    ASSERT(FALSE);
  if ("abcd" == ref->ptr)
    ASSERT(FALSE);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  {
    Vstr_base *b = vstr_make_base(NULL);
    Vstr_conf *c = vstr_make_conf();
    Vstr_sects *s = vstr_sects_make(4);

    ref = vstr_ref_make_vstr_base(b);
    vstr_ref_add(ref);
    vstr_ref_del(ref);
    vstr_ref_del(ref);
    ref = vstr_ref_make_vstr_conf(c);
    vstr_ref_add(ref);
    vstr_ref_del(ref);
    vstr_ref_del(ref);
    ref = vstr_ref_make_vstr_sects(s);
    vstr_ref_add(ref);
    vstr_ref_del(ref);
    vstr_ref_del(ref);
  }

  return (0);
}
