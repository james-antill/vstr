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

static void old_function_integrate(const char *str_fwd, const char *str_bck)
{/* the idea here is that this is an old function that you have no control over
  * and needs 2 "C strings" and will do stuff with them... */

 fprintf(stdout, "%s%s%s%s",
         " This example will print out all of the arguments\n"
         "passed to it on the command line.\n"
         " Here we go...\n\n",
         str_fwd,
         "\n And now backwards...\n\n",
         str_bck);
}

int main(int argc, char *argv[])
{
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 int count = 0;
 Vstr_ref *args_fwd = NULL;
 size_t off = 0;
 
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

 /* This is all implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex4_cpy_write() via. base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init(), and vstr_make_*() explicitly
  * Also the export_cstr has to be done explicitly */

 count = 0;
 while (count < argc)
 {
  vstr_add_fmt(str1, str1->len, "Arg(%d): %s\n", count, argv[count]);
  
  ++count;
 }
 
 if (!(args_fwd = vstr_export_cstr_ref(str1, 1, str1->len, &off)))
   errno = ENOMEM, DIE("vstr_export_cstr_ref:");
 
 vstr_del(str1, 1, str1->len);
 
 count = argc;
 while (count > 0)
 {
  --count;
  
  vstr_add_fmt(str1, str1->len, "Arg(%d): %s\n", count, argv[count]);
 }

 if (!vstr_export_cstr_ptr(str1, 1, str1->len))
   errno = ENOMEM, DIE("vstr_export_cstr_ptr:");

 old_function_integrate(((char *)args_fwd->ptr) + off,
                        vstr_export_cstr_ptr(str1, 1, str1->len));

 vstr_ref_del(args_fwd);

 vstr_free_base(str1);
 
 exit (EXIT_SUCCESS);
}
