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
#include <stdint.h>

#include <vstr.h>

#include "ex_utils.h"

int main(void)
{
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 Vstr_base *str2 = NULL;
 struct iovec *iovs = NULL;
 unsigned int num = 0;
 
 if (!vstr_init())
   errno = ENOMEM, DIE("vstr_init:");

 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, DIE("vstr_make_conf:");

 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 4);
 
 str1 = vstr_make_base(conf);
 if (!str1)
   errno = ENOMEM, DIE("vstr_make_base:");

 /* have only 1 character per _buf node */
 

 /* this shows implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex_utils_cpy_write_all() via.
  * base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init() and vstr_make_*() explicitly */
 
 vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);
 VSTR_ADD_CSTR_PTR(str1, str1->len, "Hello this is a constant message.\n\n");

 ex_utils_cpy_write_all(str1, 1);

 vstr_export_iovec_ptr_all(str1, &iovs, &num); /* makes the iovec cache valid */
 
 vstr_del(str1, strlen("Hello vstr, World "), strlen(" is"));
 vstr_del(str1, 1, 1);
 if (0)
   VSTR_SUB_CSTR_BUF(str1, 1, strlen("ello"), "Hello");
 else
   VSTR_ADD_CSTR_BUF(str1, 0, "H");
 VSTR_SUB_CSTR_BUF(str1, strlen("Hello "), strlen(" vstr,"),
                   " ** Vstr subed **");

 ex_utils_cpy_write_all(str1, 1);
 
 vstr_del(str1, str1->len - strlen("ello this is a constant message.\n\n"),
              strlen("Hello this is a constant message.\n\n"));
 
 ex_utils_cpy_write_all(str1, 1);
 
 /* str2 */

 if (str1->conf->malloc_bad)
   DIE("Bad malloc_bad flag\n");
 
 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, DIE("vstr_make_base:");

 vstr_add_vstr(str2, 0, str1, 1, str1->len, 0);

 vstr_sub_buf(str2, strlen("Hello "), strlen(" ** Vstr subed **"), " new", 4);
 vstr_sub_buf(str2, str2->len - strlen(".\n"), 1, "2", 1);

 ex_utils_cpy_write_all(str2, 1);
 
 vstr_del(str2, 1, str2->len);

 ex_utils_check();
 
 exit (EXIT_SUCCESS);
}
