#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  Vstr_ref *ref = NULL;
  
  ref = vstr_ref_make_malloc(4);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(NULL, vstr_ref_cb_free_ref);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(malloc(1), vstr_ref_cb_free_ptr_ref);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  ref = vstr_ref_make_memdup(ref, sizeof(Vstr_ref *));
  vstr_ref_add(ref);
  vstr_ref_del(ref->ptr);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(malloc(1), vstr_ref_cb_free_ptr);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(ref);
  free(ref);
  
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
