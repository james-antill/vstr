
#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/uio.h>

#include <limits.h>
#include <float.h>
#include <stdarg.h>
#include <inttypes.h>
#include <locale.h>

#include <vstr.h>

#include "ex_utils.h"

#define EX1_CUSTOM_BUF_SZ 2
#define EX1_CUSTOM_DEL_SZ 1

static void do_test(Vstr_base *str2, const char *filename)
{
  size_t netstr_beg1 = 0;
  size_t netstr_beg2 = 0;

  /* Among other things this shows explicit memory checking ...
   *  this is nicer in some cases/styles and you can tell what failed.
   *
   * Note that you _have_ to check vstr_init() and vstr_make_*() explicitly */
  
  if (!(netstr_beg1 = vstr_add_netstr_beg(str2, str2->len)))
    errno = ENOMEM, DIE("vstr_add_netstr_beg:");
  
  if (filename)
    ex_utils_append_file(str2, filename, -1, 0);
  
  if (!(netstr_beg2 = vstr_add_netstr2_beg(str2, str2->len)))
    errno = ENOMEM, DIE("vstr_add_netstr2_beg:");
  
  if (!vstr_add_ptr(str2, str2->len,
                    (char *)"Hello this is a constant message.\n\n",
                    strlen("Hello this is a constant message.\n\n")))
    errno = ENOMEM, DIE("vstr_add_ptr:");
  
  {
    const char *name_lc_numeric = NULL;
    const char *decimal_point = NULL;
    const char *thousands_sep = NULL;

    vstr_cntl_conf(str2->conf, VSTR_CNTL_CONF_GET_LOC_CSTR_NAME_NUMERIC,
                   &name_lc_numeric);
    vstr_cntl_conf(str2->conf, VSTR_CNTL_CONF_GET_LOC_CSTR_DEC_POINT,
                   &decimal_point);
    vstr_cntl_conf(str2->conf, VSTR_CNTL_CONF_GET_LOC_CSTR_THOU_SEP,
                   &thousands_sep);

    vstr_add_fmt(str2, str2->len, "\n\nLocale information:\n"
                 "name: %s\n"
                 "decimal: %s\n"
                 "thousands_sep: %s\n"
                 "grouping:",
                 name_lc_numeric, decimal_point, thousands_sep);
  }

  {
    unsigned int scan = 0;
    
    while (scan < strlen(str2->conf->loc->grouping))
    {
      vstr_add_fmt(str2, str2->len, " %d", str2->conf->loc->grouping[scan]);
      ++scan;
    }
    vstr_add_fmt(str2, str2->len, "\n");
  }

  { /* gcc doesn't understand %'d */
    const char *fool_gcc_fmt = (" Testing numbers...\n"
                                "0=%.d\n"
                                "0=%d\n"
                                "SCHAR_MIN=%'+hhd\n"
                                "SCHAR_MAX=%'+hhd\n"
                                "UCHAR_MAX=%'hhu\n"
                                "SHRT_MIN=%'+hd\n"
                                "SHRT_MAX=%'+hd\n"
                                "USHRT_MAX=%'hu\n"
                                "INT_MIN=%'+d\n"
                                "INT_MAX=%'+d\n"
                                "UINT_MAX=%'u\n"
                                "LONG_MIN=%'+ld\n"
                                "LONG_MAX=%'+ld\n"
                                "ULONG_MAX=%'lu\n"
                                "LONG_LONG_MIN=%'+lld\n"
                                "LONG_LONG_MAX=%'+lld\n"
                                "ULONG_LONG_MAX=%'llu\n"
                                "SIZE_MAX=%'zu\n"
                                "PTRDIFF_MIN=%'+td\n"
                                "PTRDIFF_MAX=%'+td\n"
                                "INTMAX_MIN=%'+jd\n"
                                "INTMAX_MAX=%'+jd\n"
                                "UINTMAX_MAX=%'ju\n"
                                "DOUBLE_MIN=%'+f\n"
                                "DOUBLE_MAX=%'+f\n"
                                );
    
    if (!vstr_add_fmt(str2, str2->len, fool_gcc_fmt,
                      0, 0,
                      SCHAR_MIN,
                      SCHAR_MAX,
                      UCHAR_MAX,
                      SHRT_MIN,
                      SHRT_MAX,
                      USHRT_MAX,
                      INT_MIN,
                      INT_MAX,
                      UINT_MAX,
                      LONG_MIN,
                      LONG_MAX,
                      ULONG_MAX,
                      LONG_LONG_MIN,
                      LONG_LONG_MAX,
                      ULONG_LONG_MAX,
                      SIZE_MAX,
                      PTRDIFF_MIN,
                      PTRDIFF_MAX,
                      INTMAX_MIN,
                      INTMAX_MAX,
                      UINTMAX_MAX,
                      -DBL_MAX, /* DBL_MIN is a very small fraction */
                      DBL_MAX
                      ))
      errno = ENOMEM, DIE("vstr_add_fmt:");
    
    /* gcc doesn't understand i18n param numbers */
    fool_gcc_fmt = (" Testing i18n explicit param numbers...\n"
                    " 1=%1$*1$.*1$d\n"
                    " 2=%2$*2$.*2$d\n"
                    " 2=%2$*4$.*1$d\n"
                    " 3=%3$*1$.*2$jd\n"
                    " 4=%4$*4$.*4$d\n"
                    " 1=%1$*2$.*4$d\n");
    if (!vstr_add_fmt(str2, str2->len, fool_gcc_fmt, 1, 2, (intmax_t)3, 4))
      errno = ENOMEM, DIE("vstr_add_fmt:");
    
    if (!vstr_add_fmt(str2, str2->len, " Testing wchar_t support...\n"
                      "%s: %S\n"
                      "%c: %C\n",
                      "abcd", L"abcd",
                      'z', (wint_t)L'z'))
      errno = ENOMEM, DIE("vstr_add_fmt:");
  }
  
  if (!vstr_add_fmt(str2, str2->len, " Testing %%p=%p.\n", (void *)str2))
    errno = ENOMEM, DIE("vstr_add_fmt:");
  
  if (!vstr_add_netstr2_end(str2, netstr_beg2, str2->len))
    errno = ENOMEM, DIE("vstr_add_netstr2_end:");
  if (!vstr_add_netstr_end(str2, netstr_beg1, str2->len))
    errno = ENOMEM, DIE("vstr_add_netstr_end:");
  
  if (!(netstr_beg1 = vstr_add_netstr_beg(str2, str2->len)))
    errno = ENOMEM, DIE("vstr_add_netstr_beg:");
  if (!vstr_add_netstr_end(str2, netstr_beg1, str2->len))
    errno = ENOMEM, DIE("vstr_add_netstr_end:");
  
  if (!(netstr_beg2 = vstr_add_netstr2_beg(str2, str2->len)))
    errno = ENOMEM, DIE("vstr_add_netstr2_beg:");
  if (!vstr_add_netstr2_end(str2, netstr_beg2, str2->len))
    errno = ENOMEM, DIE("vstr_add_netstr2_end:");
  
  if (!vstr_add_buf(str2, str2->len, "\n", 1))
    errno = ENOMEM, DIE("vstr_add_buf:");
  
  while (str2->len)
  {
    size_t tmp = EX1_CUSTOM_DEL_SZ;
    
    ex_utils_cpy_write_all(str2, 1);
    
    if (tmp > str2->len)
      tmp = str2->len;
    
    vstr_del(str2, 1, tmp);
  }
}

int main(int argc, char *argv[])
{
  Vstr_conf *conf = NULL;
  Vstr_base *str1 = NULL;
  Vstr_base *str2 = NULL;
  
  /* export LC_ALL=en_US to see thousands_sep in action */
  /* before vstr_init() so that the default conf has the locale info. */
  setlocale(LC_ALL, "");
  
  if (!vstr_init())
    errno = ENOMEM, DIE("vstr_init:");
  
  setlocale(LC_ALL, "C"); /* the default conf has the locale... but not the
                           * one we make. Or the rest of the program. */
  
  if (!(conf = vstr_make_conf()))
    errno = ENOMEM, DIE("vstr_make_conf:");
  
  /* have only 2 character per _buf node */
  vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, EX1_CUSTOM_BUF_SZ);
  
  str1 = vstr_make_base(conf);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");
  
  /* this shows implicit memory checking ...
   *  this is nicer in some cases/styles but you can't tell what failed.
   * The checking is done inside ex1_cpy_write() via. base->conf->malloc_bad
   *
   * Note that you _have_ to check vstr_init() and vstr_make_*() explicitly */
  
  vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);
  
  ex_utils_cpy_write_all(str1, 1);
  
  vstr_add_ptr(str1, str1->len,
               (char *)"Hello this is a constant message.\n\n",
               strlen("Hello this is a constant message.\n\n"));
 
  ex_utils_cpy_write_all(str1, 1);
  
  vstr_del(str1, strlen("Hello vstr, World "), strlen(" is"));
  vstr_del(str1, strlen("Hello "), strlen(" vstr,"));
  
  ex_utils_cpy_write_all(str1, 1);
  
  while (str1->len)
  {
    size_t tmp = EX1_CUSTOM_DEL_SZ;
    
    ex_utils_cpy_write_all(str1, 1);

    if (tmp > str1->len)
      tmp = str1->len;
    
    vstr_del(str1, 1, tmp);
  }
  
  /* str2 */
  
  if (str1->conf->malloc_bad)
    DIE("Bad malloc_bad flag\n");
  
  str2 = vstr_make_base(NULL); /* get a default conf */
  if (!str2)
    errno = ENOMEM, DIE("vstr_make_base:");
  
  do_test(str2, (argc > 1) ? argv[1] : NULL);
  do_test(str1, (argc > 1) ? argv[1] : NULL);

  setlocale(LC_ALL, "");

  do_test(str2, (argc > 1) ? argv[1] : NULL);
  do_test(str1, (argc > 1) ? argv[1] : NULL);

  if (!vstr_cntl_conf(str1->conf,
                      VSTR_CNTL_CONF_SET_LOC_CSTR_NAME_NUMERIC, "ex1-custom"))
    errno = ENOMEM, DIE("vstr_cntl_conf(LOC_CSTR_NAME_NUMERIC):");
  if (!vstr_cntl_conf(str1->conf,
                      VSTR_CNTL_CONF_SET_LOC_CSTR_DEC_POINT, "<.>"))
    errno = ENOMEM, DIE("vstr_cntl_conf(LOC_CSTR_DECIMAL_POINT):");
  if (!vstr_cntl_conf(str1->conf,
                      VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_SEP, "->"))
    errno = ENOMEM, DIE("vstr_cntl_conf(LOC_CSTR_THOU_SEP):");
  if (!vstr_cntl_conf(str1->conf,
                      VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_GRP, "\4\2\3"))
    errno = ENOMEM, DIE("vstr_cntl_conf(LOC_CSTR_THOU_GRP):");
  
  do_test(str2, (argc > 1) ? argv[1] : NULL);
  do_test(str1, (argc > 1) ? argv[1] : NULL);

  ex_utils_check();
  
  vstr_free_base(str1);
  vstr_free_base(str2);

  vstr_free_conf(conf);

  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
