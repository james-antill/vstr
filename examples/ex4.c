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

typedef struct ex4_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex4_mmap_ref;

#if 0
static void ex4_ref_munmap(Vstr_ref *passed_ref)
{
 ex4_mmap_ref *ref = (ex4_mmap_ref *)passed_ref;
 munmap(ref->ref.ptr, ref->len);
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
  vstr_add_buf(str, str->len, strlen(msg), msg);

  if (vstr_export_chr(str, strlen(msg)) == ':')
    vstr_add_fmt(str, str->len, " %d %s", saved_errno, strerror(saved_errno));

  vstr_add_buf(str, str->len, 1, "\n");

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

static void ex4_cpy_write(Vstr_base *base, int fd)
{
 static Vstr_base *cpy = NULL;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;

 if (base->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("before ex4_cpy_write:");
 if ((!cpy && !(cpy = vstr_make_base(NULL))) || cpy->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 vstr_add_vstr(cpy, 0, base->len, base, 1, VSTR_TYPE_ADD_BUF_PTR);
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

static void test1(Vstr_base *str1, Vstr_base *str2, const char *tester)
{
 size_t netstr_beg = 0;
 size_t netstr_pos = 0;
 size_t netstr_len = 0;
 size_t tmp = 0;

 netstr_beg = vstr_add_netstr_beg(str1);
 if (netstr_beg != 1)
   PROBLEM("netstr_beg");
 
 tmp = vstr_add_fmt(str1, str1->len, "%s", tester);
 vstr_add_netstr_end(str1, netstr_beg);

 vstr_add_fmt(str2, str2->len, "String is: %s\n", tester);
 vstr_add_fmt(str2, str2->len, "String len: %d\n", tmp);
 
 ex4_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 netstr_beg = vstr_srch_netstr_fwd(str1, 1, 1, &netstr_pos, &netstr_len);
 if (netstr_beg != 1)
   PROBLEM("netstr_beg");
 if (netstr_len != tmp)
   PROBLEM("netstr_len");

 vstr_add_fmt(str2, str2->len, "Parse netstr: %d %d %d\n",
              netstr_beg, netstr_pos, netstr_len);
 
 ex4_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len); 
 vstr_del(str1, 1, str1->len);
}

static void test2(Vstr_base *str1, Vstr_base *str2)
{
 size_t netstr_beg = 0;
 size_t netstr_pos = 0;
 size_t netstr_len = 0;
 
 /* test empty */
 netstr_beg = vstr_add_netstr_beg(str1);
 if (netstr_beg != 1)
   PROBLEM("netstr_beg");
 
 vstr_add_netstr_end(str1, netstr_beg);

 vstr_add_fmt(str2, str2->len, "String empty.\n");
 
 ex4_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 netstr_beg = vstr_srch_netstr_fwd(str1, 1, 1, &netstr_pos, &netstr_len);
 if (netstr_beg != 1)
   PROBLEM("netstr_beg");
 if (netstr_len != 0)
   PROBLEM("netstr_len");

 vstr_add_fmt(str2, str2->len, "Parse netstr: %d %d %d\n",
              netstr_beg, netstr_pos, netstr_len);
 
 ex4_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len); 
 vstr_del(str1, 1, str1->len);
}

int main(void /* int argc, char *argv[] */)
{
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 Vstr_base *str2 = NULL;
 
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

 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 /* This is all implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex4_cpy_write() via. base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init(), and vstr_make_*() explicitly */
 
 vstr_add_fmt(str2, str2->len, "Testing vstr_srch_netstr\n");

 test1(str1, str2, "Test da netstr.");
 test2(str1, str2);

 exit (EXIT_SUCCESS);
}
