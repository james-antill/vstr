#define _GNU_SOURCE

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdarg.h>
#include <inttypes.h>
#include <locale.h>

#include <vstr.h>

#include "ex_utils.h"

static void do_test(Vstr_base *str1, Vstr_base *str2)
{
  struct iovec *iovs = NULL;
  unsigned int num = 0;

  assert(!str1->len);
  assert(!str2->len);

  vstr_add_ptr(str1, str1->len,
               (char *)"----------------------------------------",
               strlen("----------------------------------------"));
  vstr_add_ptr(str1, str1->len,
               (char *)"----------------------------------------",
               strlen("----------------------------------------"));
    vstr_add_fmt(str1, str1->len, "\n");
  while (str1->len)
    ex_utils_write(str1, 1);
  
  vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);
  vstr_mov(str2, 0, str1, 1, str1->len);
  
  vstr_add_ptr(str1, str1->len,
               (char *)"Hello this is a constant message.\n\n",
               strlen("Hello this is a constant message.\n\n"));
  assert(!str1->conf->malloc_bad);
  
  ex_utils_cpy_write_all(str2, 1);
  
  vstr_del(str2, strlen("Hello vstr, World "), strlen(" is"));
  vstr_del(str2, strlen("Hello "), strlen(" vstr"));
  
  ex_utils_cpy_write_all(str2, 1);

  vstr_add_iovec_buf_beg(str2, strlen("Hello"), 2, 4, &iovs, &num);

  ((char *)(iovs[0].iov_base))[0] = 'a';

  vstr_add_iovec_buf_end(str2, strlen("Hello"), 1);

  vstr_add_buf(str1, 0, " ", 1);
  vstr_mov(str2, strlen("Hello"), str1, 1, 1);
  
  vstr_mov(str2, 0, str1, 1, str1->len);

  while (str2->len)
    ex_utils_write(str2, 1);
}

int main(void)
{
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 Vstr_base *str2 = NULL;
 
 if (!vstr_init())
   errno = ENOMEM, DIE("vstr_init:");

 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, DIE("vstr_make_conf:");

 /* have only 1 character per _buf node */
 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 1);
 
 str1 = vstr_make_base(conf);
 if (!str1)
   errno = ENOMEM, DIE("vstr_make_base:");
 
 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, DIE("vstr_make_base:");
 
 setlocale(LC_ALL, "");

 do_test(str1, str2);
 do_test(str2, str1);

 ex_utils_check();
 
 exit (EXIT_SUCCESS);
}
