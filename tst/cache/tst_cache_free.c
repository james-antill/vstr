#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{ /* other stuff done in export_cstr_ptr */
  vstr_cache_cb_free(s4, 0); /* free all cached stuff, with no cache */
  
  return (EXIT_SUCCESS);
}
