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

typedef struct ex6_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex6_mmap_ref;

static int have_mmaped_file = 0;

#if 0
static void ex6_ref_munmap(Vstr_ref *passed_ref)
{
 ex6_mmap_ref *ref = (ex6_mmap_ref *)passed_ref;
 munmap(ref->ref.ptr, ref->len);
 free(ref);

 have_mmaped_file = 0;
}
#endif

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

static void ex6_cpy_write(Vstr_base *base, int fd)
{
 static Vstr_base *cpy = NULL;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;
 
 if (base->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("before ex6_cpy_write:");
 if ((!cpy && !(cpy = vstr_make_base(NULL))) || cpy->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 vstr_add_vstr(cpy, 0, base, 1, base->len, VSTR_TYPE_ADD_BUF_PTR);
 if (cpy->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("vstr_add_vstr:");
 
 len = vstr_export_iovec_ptr_all(cpy, &vec, &num);
 if (!len)
   errno = ENOMEM, PROBLEM("vstr_export_iovec_ptr_all:");
 
 while ((size_t)(bytes = writev(fd, vec, num)) != len)
 {
  if ((bytes == -1) && (errno != EAGAIN))
    PROBLEM("writev:");
  if (bytes == -1)
    continue;
  
  vstr_del(cpy, 1, (size_t)bytes);
  
  len = vstr_export_iovec_ptr_all(cpy, &vec, &num);
 }
 
 vstr_del(cpy, 1, len);
 assert(!cpy->len);
}


int main(void)
{
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 Vstr_base *str2 = NULL;
 
 if (!vstr_init())
   errno = ENOMEM, PROBLEM("vstr_init:");

 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, PROBLEM("vstr_make_conf:");

 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 4);
 
 str1 = vstr_make_base(conf);
 if (!str1)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 /* have only 1 character per _buf node */
 

 /* this shows implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex6_cpy_write() via. base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init() and vstr_make_*() explicitly */
 
 vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);

 ex6_cpy_write(str1, 1);

 vstr_add_ptr(str1, str1->len, (char *)"Hello this is a constant message.\n\n",
              strlen("Hello this is a constant message.\n\n"));

 ex6_cpy_write(str1, 1);
 
 vstr_del(str1, strlen("Hello vstr, World "), strlen(" is"));
 vstr_sub_buf(str1, strlen("Hello "), strlen(" vstr,"),
              " ** Vstr subed **", strlen(" ** Vstr subed **"));

 ex6_cpy_write(str1, 1);
 
 vstr_del(str1, str1->len - strlen("ello this is a constant message.\n\n"),
              strlen("Hello this is a constant message.\n\n"));
 
 ex6_cpy_write(str1, 1);
 
 /* str2 */

 if (str1->conf->malloc_bad)
   PROBLEM("Bad malloc_bad flag\n");
 
 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 vstr_add_vstr(str2, 0, str1, 1, str1->len, 0);

 vstr_sub_buf(str2, strlen("Hello "), strlen(" ** Vstr subed **"), " new", 4);
 vstr_sub_buf(str2, str2->len - strlen(".\n"), 1, "2", 1);

 ex6_cpy_write(str2, 1);
 
 vstr_del(str2, 1, str2->len);

 if (have_mmaped_file)
   PROBLEM("bad munmap");
 
 exit (EXIT_SUCCESS);
}
