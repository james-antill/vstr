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

typedef struct ex7_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex7_mmap_ref;

static int have_mmaped_file = 0;

#if 0
static void ex7_ref_munmap(Vstr_ref *passed_ref)
{
 ex7_mmap_ref *ref = (ex7_mmap_ref *)passed_ref;
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

static void ex7_cpy_write(Vstr_base *base, int fd)
{
 static Vstr_base *cpy = NULL;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;
 
 if (base->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("before ex7_cpy_write:");
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

static void do_test(Vstr_base *str1, Vstr_base *str2)
{
  struct iovec *iovs = NULL;
  unsigned int num = 0;
  
  vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);
  assert(!str1->conf->malloc_bad);
  vstr_mov(str2, 0, str1, 1, str1->len);
  assert(!str2->conf->malloc_bad);
  
  vstr_add_ptr(str1, str1->len,
               (char *)"Hello this is a constant message.\n\n",
               strlen("Hello this is a constant message.\n\n"));
  assert(!str1->conf->malloc_bad);
  
  vstr_del(str2, strlen("Hello vstr, World "), strlen(" is"));
  vstr_del(str2, strlen("Hello "), strlen(" vstr,"));
  assert(!str2->conf->malloc_bad);
  
  ex7_cpy_write(str2, 1);

  vstr_add_iovec_buf_beg(str2, strlen("Hello "), 2, 4, &iovs, &num);
  assert(!str2->conf->malloc_bad);

  ((char *)(iovs[0].iov_base))[0] = 'a';

  vstr_add_iovec_buf_end(str2, strlen("Hello "), 1);

  vstr_mov(str2, 0, str1, 1, str1->len);
  assert(!str2->conf->malloc_bad);

  ex7_cpy_write(str2, 1);
  vstr_del(str2, 1, str2->len);
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

 /* have only 1 character per _buf node */
 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 1);
 
 str1 = vstr_make_base(conf);
 if (!str1)
   errno = ENOMEM, PROBLEM("vstr_make_base:");
 
 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, PROBLEM("vstr_make_base:");
 
 setlocale(LC_ALL, "");

 do_test(str1, str2);
 do_test(str2, str1);

 if (have_mmaped_file)
   PROBLEM("bad munmap");
 
 exit (EXIT_SUCCESS);
}
