#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdint.h>

#include <vstr.h>

#include "ex_utils.h"

static void test1(Vstr_base *str1, Vstr_base *str2, const char *tester,
                  int use_netstr2_add, int use_netstr2_parse)
{
 size_t netstr_beg = 0;
 size_t netstr_pos = 0;
 size_t netstr_len = 0;
 size_t netstr_data_len = 0;
 size_t tmp = 0;
 int added = TRUE;
 
 if (use_netstr2_add)
   netstr_beg = vstr_add_netstr2_beg(str1, str1->len);
 else
   netstr_beg = vstr_add_netstr_beg(str1, str1->len);
 if (netstr_beg != 1)
   DIE("netstr_beg");
 
 tmp = vstr_add_fmt(str1, str1->len, "%s", tester);

 if (use_netstr2_add)
   added = vstr_add_netstr2_end(str1, netstr_beg, str1->len);
 else
   added = vstr_add_netstr_end(str1, netstr_beg, str1->len);

 if (!added)
   DIE("netstr_end");
 
 vstr_add_fmt(str2, str2->len, " Use netstr 2 add/parse [%d,%d]\n"
              "String is: %s\n"
              "String len: %d\n",
              use_netstr2_add, use_netstr2_parse, tester, tmp);
 
 ex_utils_cpy_write_all(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 if (use_netstr2_parse)
   netstr_len = vstr_parse_netstr2(str1, 1, str1->len,
                                   &netstr_pos, &netstr_data_len);
 else
   netstr_len = vstr_parse_netstr(str1, 1, str1->len,
                                  &netstr_pos, &netstr_data_len);

 vstr_add_fmt(str2, str2->len, "Parse netstr: pos=%d len=%d "
              "data_pos=%d data_len=%d\n\n",
              netstr_beg, netstr_len, netstr_pos, netstr_data_len);
 
 ex_utils_cpy_write_all(str2, 1);
 vstr_del(str2, 1, str2->len); 
 vstr_del(str1, 1, str1->len);
}

static void test2(Vstr_base *str1, Vstr_base *str2,
                  int use_netstr2_add, int use_netstr2_parse)
{
 size_t netstr_beg = 0;
 size_t netstr_pos = 0;
 size_t netstr_len = 0;
 size_t netstr_data_len = 0;
 int added = TRUE;
 
 if (use_netstr2_add)
   netstr_beg = vstr_add_netstr2_beg(str1, str1->len);
 else
   netstr_beg = vstr_add_netstr_beg(str1, str1->len);
 if (netstr_beg != 1)
   DIE("netstr_beg");
 
 /* test empty */
 
 if (use_netstr2_add)
   added = vstr_add_netstr2_end(str1, netstr_beg, str1->len);
 else
   added = vstr_add_netstr_end(str1, netstr_beg, str1->len);

 if (!added)
   DIE("netstr_end");

 vstr_add_fmt(str2, str2->len, " Use netstr 2 add/parse [%d,%d]\n"
              "String is empty.\n",
              use_netstr2_add, use_netstr2_parse);
 
 ex_utils_cpy_write_all(str2, 1);
 vstr_del(str2, 1, str2->len);

 if (use_netstr2_parse)
   netstr_len = vstr_parse_netstr2(str1, 1, str1->len,
                                   &netstr_pos, &netstr_data_len);
 else
   netstr_len = vstr_parse_netstr(str1, 1, str1->len,
                                  &netstr_pos, &netstr_data_len);

 vstr_add_fmt(str2, str2->len, "Parse netstr: pos=%d len=%d "
              "data_pos=%d data_len=%d\n\n",
              netstr_beg, netstr_len, netstr_pos, netstr_data_len);
 
 ex_utils_cpy_write_all(str2, 1);
 vstr_del(str2, 1, str2->len); 
 vstr_del(str1, 1, str1->len);
}

int main(void /* int argc, char *argv[] */)
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
 vstr_cntl_opt(VSTR_CNTL_OPT_SET_CONF, conf);

 str1 = vstr_make_base(NULL);
 if (!str1)
   errno = ENOMEM, DIE("vstr_make_base:");

 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, DIE("vstr_make_base:");

 /* This is all implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex_utils_cpy_write_all() via.
  * base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init(), and vstr_make_*() explicitly */
 
 vstr_add_fmt(str2, str2->len, "Testing vstr_srch_netstr\n");

 test1(str1, str2, "Test da netstr.", FALSE, FALSE);
 test1(str1, str2, "Test da netstr.", FALSE, TRUE);
 test1(str1, str2, "Test da netstr.", TRUE, FALSE); /* fails */
 test1(str1, str2, "Test da netstr.", TRUE, TRUE);
 test2(str1, str2, FALSE, FALSE);
 test2(str1, str2, FALSE, TRUE);
 test2(str1, str2, TRUE, FALSE); /* fails */
 test2(str1, str2, TRUE, TRUE);

 exit (EXIT_SUCCESS);
}
