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

/* this shows how you can remove the typedef namespace, if you wish to use it */
#define VSTR_COMPILE_TYPEDEF 0
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

typedef struct ex3_mmap_ref
{
 struct Vstr_ref ref;
 size_t len;
} ex3_mmap_ref;

#if 0
static void ex3_ref_munmap(struct Vstr_ref *passed_ref)
{
 ex3_mmap_ref *ref = (ex3_mmap_ref *)passed_ref;
 munmap(ref->ref.ptr, ref->len);
}
#endif

static void problem(const char *msg)
{
 int saved_errno = errno;
 struct Vstr_base *str = NULL;
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

static void ex3_cpy_write(struct Vstr_base *base, int fd)
{
 static struct Vstr_base *cpy = NULL;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;

 if (base->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("before ex3_cpy_write:");
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

int main(void /* int argc, char *argv[] */)
{
  struct Vstr_conf *conf = NULL;
  /* Vstr_base *str1 = NULL; */
  struct Vstr_base *str2 = NULL;
  /* size_t netstr_beg1 = 0;
     size_t netstr_beg2 = 0; */
  struct Vstr_base *x_str = NULL;
  struct Vstr_base *y_str = NULL;
 
 if (!vstr_init())
   errno = ENOMEM, PROBLEM("vstr_init:");
 
 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, PROBLEM("vstr_make_conf:");
 
 /* have only 1 character per _buf node */
 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 1);
 vstr_cntl_opt(VSTR_CNTL_OPT_SET_CONF, conf);

 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 /* This is all implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex3_cpy_write() via. base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init(), and vstr_make_*() explicitly */

#define TEST_CMP_STR(x) #x
#define TEST_CMP_XSTR(x) TEST_CMP_STR(x)

 /* this is a bit sick so ... don't try this at home. but it works */
#define TEST_CMP_BEG(x, y) do { \
 x_str = vstr_dup_buf(NULL, (x), (sizeof(x) - 1)); \
 if (!x_str) errno = ENOMEM, PROBLEM("vstr_dup_buf x_str:"); \
 y_str = vstr_dup_ptr(NULL, (y), (sizeof(y) - 1)); \
 if (!y_str) errno = ENOMEM, PROBLEM("vstr_dup_ptr y_str:"); \
 vstr_add_fmt(str2, str2->len, "  Strings: 1=(%s) 2=(%s)\n", x, y); \
 ex3_cpy_write(str2, 1); vstr_del(str2, 1, str2->len); \
 if (TEST_CMP_FUNC (x_str, 1, x_str->len, y_str, 1, y_str->len)

#define TEST_CMP_END(x, y) ) PROBLEM(TEST_CMP_XSTR(TEST_CMP_FUNC) \
                                     ": " x ":" y "\n"); \
 vstr_free_base(x_str); \
 vstr_free_base(y_str); \
 vstr_add_fmt(str2, str2->len, "  ... OK\n"); \
 ex3_cpy_write(str2, 1); vstr_del(str2, 1, str2->len); \
 } while (FALSE)

#define TEST_CMP_EQ_0(x, y) \
 TEST_CMP_BEG(x, y) != 0 TEST_CMP_END(x, y)
                                
#define TEST_CMP_GT_0(x, y) \
 TEST_CMP_BEG(x, y) <= 0 TEST_CMP_END(x, y); \
 TEST_CMP_BEG(y, x) >= 0 TEST_CMP_END(y, x)


#define TEST_CMP_FUNC vstr_cmp
 
 vstr_add_fmt(str2, str2->len, "Testing %s\n", TEST_CMP_XSTR(TEST_CMP_FUNC));

 ex3_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 TEST_CMP_EQ_0("abcd", "abcd");
 TEST_CMP_EQ_0("abcd1", "abcd1");
 TEST_CMP_EQ_0("abcd12345", "abcd12345");
 TEST_CMP_EQ_0("abcd012345", "abcd012345");
 
#undef TEST_CMP_FUNC
#define TEST_CMP_FUNC vstr_cmp_case
 
 vstr_add_fmt(str2, str2->len, "Testing %s\n", TEST_CMP_XSTR(TEST_CMP_FUNC));

 ex3_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 TEST_CMP_EQ_0("abcd", "abcd");
 TEST_CMP_EQ_0("abcd1", "abcd1");
 TEST_CMP_EQ_0("abcd12345", "abcd12345");
 TEST_CMP_EQ_0("abcd012345", "abcd012345");
 
#undef TEST_CMP_FUNC
#define TEST_CMP_FUNC vstr_cmp_vers

 vstr_add_fmt(str2, str2->len, "Testing %s\n", TEST_CMP_XSTR(TEST_CMP_FUNC));

 ex3_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 /* these all work in GNU strverscmp() --
  *   although you might not/won't get the same return value number (but it'll
  *   be the correct sign) */
 TEST_CMP_EQ_0("abcd", "abcd");
 TEST_CMP_EQ_0("abcd1", "abcd1");
 TEST_CMP_EQ_0("abcd12345", "abcd12345");
 TEST_CMP_EQ_0("abcd012345", "abcd012345");

 TEST_CMP_GT_0("abcd1234", "abcd123");
 TEST_CMP_GT_0("abcd1234", "abcd999");
 TEST_CMP_GT_0("abcd9234", "abcd1234");
 TEST_CMP_GT_0("abcd01234", "abcd0123");
 TEST_CMP_GT_0("abcd0999", "abcd01234");
 TEST_CMP_GT_0("abcd0", "abcd00");
 TEST_CMP_GT_0("abcd00", "abcd009");
 TEST_CMP_GT_0("abcd09", "abcd00");
 TEST_CMP_GT_0("abcd00z", "abcd00a");
 TEST_CMP_GT_0("abcd09z", "abcd09a");
 TEST_CMP_GT_0("abcd0", "abcd");
 TEST_CMP_GT_0("abcd1", "abcd");

 /* ************************************************************************ */
 /* custom stuff... */
#undef TEST_CMP_FUNC
 vstr_add_fmt(str2, str2->len, "  Strings: abcd (NON,4) xyz\n");
 
 x_str = vstr_make_base(NULL);
 y_str = vstr_make_base(NULL);
 vstr_add_fmt(x_str, x_str->len, "abcd");
 vstr_add_fmt(y_str, y_str->len, "abcd");

 vstr_add_non(x_str, x_str->len, 4);
 vstr_add_non(y_str, y_str->len, 4);

 vstr_add_fmt(x_str, x_str->len, "xyz");
 vstr_add_fmt(y_str, y_str->len, "xyz");

 if (vstr_cmp_vers(x_str, 1, x_str->len, y_str, 1, y_str->len))
   PROBLEM("vstr_cmp_vers");
 if (vstr_cmp_case(x_str, 1, x_str->len, y_str, 1, y_str->len))
   PROBLEM("vstr_cmp_case");
 if (vstr_cmp(x_str, 1, x_str->len, y_str, 1, y_str->len))
   PROBLEM("vstr_cmp");
 
 vstr_free_base(x_str);
 vstr_free_base(y_str);

 vstr_add_fmt(str2, str2->len, "  ... OK\n");
 ex3_cpy_write(str2, 1); vstr_del(str2, 1, str2->len);

 exit (EXIT_SUCCESS);
}
