/* TEST: abcd */
#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t tlen = strlen("/* TEST: abcd */\n");
  unsigned int err = 0;
  
#if !defined(VSTR_AUTOCONF_HAVE_MMAP) /* || !defined(HAVE_POSIX_HOST) */
  return (EXIT_FAILED_OK);
#endif

  TST_B_TST(ret, 1, 
            !vstr_sc_mmap_file(s1, s1->len, __FILE__, 0, tlen, NULL));

  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "/* TEST: abcd */\n"));

  TST_B_TST(ret, 3, 
            !vstr_sc_mmap_file(s1, s1->len, __FILE__, 0, 0, NULL));

  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s1, 1, strlen("/* TEST: abcd */\n"),
                                      "/* TEST: abcd */\n"));

  TST_B_TST(ret, 5, !!vstr_sc_mmap_fd(s1, s1->len, 1024, 0, 0, &err));

  TST_B_TST(ret, 6, (err != VSTR_TYPE_SC_MMAP_FD_ERR_FSTAT_ERRNO));

  TST_B_TST(ret, 7, !!vstr_sc_mmap_fd(s1, s1->len, 1024, 0, 1, &err));
  
  TST_B_TST(ret, 8, (err != VSTR_TYPE_SC_MMAP_FD_ERR_MMAP_ERRNO));
            
  return (TST_B_RET(ret));
}
