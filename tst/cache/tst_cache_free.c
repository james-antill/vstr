#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{ /* other stuff done in export_cstr_ptr */
  vstr_cache_cb_free(s1, 0);
  vstr_cache_cb_free(s2, 0);
  vstr_cache_cb_free(s3, 0);
  vstr_cache_cb_free(s4, 0);

  return (EXIT_SUCCESS);
}
