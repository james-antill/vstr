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

#include <vstr.h>

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#ifndef __GNUC__
# define __PRETTY_FUNCTION__ "(unknown)"
#endif

#define STR(x) #x
#define XSTR(x) STR(x)

#define SHOW(x, y, z) \
 "File: " x "\n" \
 "Function: " y "\n" \
 "Line: " XSTR(z) "\n" \
 "Problem: "



#define PROBLEM(x) problem(SHOW(__FILE__, __PRETTY_FUNCTION__, __LINE__) x)

#define assert(x) do { if (!(x)) PROBLEM("Assert=\"" #x "\""); } while (FALSE)

static void problem(const char *msg)
{
 int saved_errno = errno;
 Vstr_base *str = NULL;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;
 
 str = vstr_make_base(NULL);
 if (str)
 {
  vstr_add_buf(str, str->len, msg, strlen(msg));

  if (vstr_export_chr(str, strlen(msg)) == ':')
    vstr_add_fmt(str, str->len, " %d %s", saved_errno, strerror(saved_errno));

  vstr_add_buf(str, str->len, "\n", 1);

  len = vstr_export_iovec_ptr_all(str, &vec, &num);
  if (!len)
    _exit (EXIT_FAILURE);

  while ((size_t)(bytes = writev(2, vec, num)) != len)
  {
   if ((bytes == -1) && (errno != EAGAIN))
     break;
   if (bytes == -1)
     continue;
   
   vstr_del(str, 1, (size_t)bytes);
   
   len = vstr_export_iovec_ptr_all(str, &vec, &num);
  }
 }
 
 _exit (EXIT_FAILURE);
}

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
 
 if (!vstr_init())
   errno = ENOMEM, PROBLEM("vstr_init:");
 
 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, PROBLEM("vstr_make_conf:");
 
 /* have only 1 character per _buf node */
 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 1);
 vstr_cntl_opt(VSTR_CNTL_OPT_SET_CONF, conf);

 str1 = vstr_make_base(NULL);
 if (!str1)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

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
 
 if (!(args_fwd = vstr_export_cstr_ref(str1, 1, str1->len)))
   errno = ENOMEM, PROBLEM("vstr_export_cstr_ref:");
 vstr_del(str1, 1, str1->len);
 
 count = argc;
 while (count > 0)
 {
  --count;
  
  vstr_add_fmt(str1, str1->len, "Arg(%d): %s\n", count, argv[count]);
 }

 if (!vstr_export_cstr_ptr(str1, 1, str1->len))
   errno = ENOMEM, PROBLEM("vstr_export_cstr_ptr:");

 old_function_integrate(args_fwd->ptr,
                        vstr_export_cstr_ptr(str1, 1, str1->len));

 vstr_ref_del_ref(args_fwd);

 vstr_free_base(str1);
 
 exit (EXIT_SUCCESS);
}
