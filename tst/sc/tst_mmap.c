/* TEST: abcd */
#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t tlen = strlen("/* TEST: abcd */\n");
  
#if !defined(VSTR_AUTOCONF_HAVE_MMAP) /* || !defined(HAVE_POSIX_HOST) */
  return (EXIT_FAILED_OK);
#endif

  TST_B_TST(ret, 1, 
            !vstr_sc_mmap_file(s1, s1->len, __FILE__, 0, tlen, NULL));

  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "/* TEST: abcd */\n"));

  return (TST_B_RET(ret));
}
