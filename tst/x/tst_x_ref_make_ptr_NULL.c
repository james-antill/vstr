#include "tst-main.c"

static const char *rf = __FILE__;

#ifndef __GLIBC__ /* FIXME: need better test */
int tst(void)
{
  return (EXIT_SUCCESS);
}
#else
static int tst_mall_special_fail = FALSE;

extern void *__libc_malloc(size_t);
void *malloc(size_t len)
{
  if (tst_mall_special_fail)
    return (NULL);
  
  return (__libc_malloc(len));
}

int tst(void)
{
  tst_mall_special_fail = TRUE;
  ASSERT(!vstr_ref_make_ptr("", NULL));
  tst_mall_special_fail = FALSE;
  
  return (EXIT_SUCCESS);
}
#endif
