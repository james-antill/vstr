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

typedef struct ex1_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex1_mmap_ref;

static int have_mmaped_file = 0;

static void ex1_ref_munmap(Vstr_ref *passed_ref)
{
 ex1_mmap_ref *ref = (ex1_mmap_ref *)passed_ref;
 munmap(ref->ref.ptr, ref->len);
 free(ref);

 have_mmaped_file = 0;
}


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

static void ex1_cpy_write(Vstr_base *base, int fd)
{
 static Vstr_base *cpy = NULL;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;
 
 if (base->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("before ex1_cpy_write:");
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


int main(int argc, char *argv[])
{
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 Vstr_base *str2 = NULL;
 size_t netstr_beg1 = 0;
 size_t netstr_beg2 = 0;
 struct lconv *loc = NULL;
 unsigned int scan = 0;
 
 if (!vstr_init())
   errno = ENOMEM, PROBLEM("vstr_init:");

 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, PROBLEM("vstr_make_conf:");

 /* have only 1 character per _buf node */
 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 1);
 
 str1 = vstr_make_base(conf);
 if (!str1)
   errno = ENOMEM, PROBLEM("vstr_make_base:");
 
 setlocale(LC_ALL, "");
 loc = localeconv();

 /* this shows implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex1_cpy_write() via. base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init() and vstr_make_*() explicitly */
 
 vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);
 
 ex1_cpy_write(str1, 1);

 vstr_add_ptr(str1, str1->len, strlen("Hello this is a constant message.\n"),
              (char *)"Hello this is a constant message.\n");

 vstr_add_fmt(str1, str1->len, "\n\nLocale information:\n"
              "name: %s\n"
              "thousands_sep: %s\n"
              "grouping:",
              setlocale(LC_ALL, NULL),
              loc->thousands_sep);

 while (scan < strlen(loc->grouping))
 {
   vstr_add_fmt(str1, str1->len, " %d", loc->grouping[scan]);
   ++scan;
 }
 vstr_add_fmt(str1, str1->len, "\n");
 
 ex1_cpy_write(str1, 1);
 
 vstr_del(str1, strlen("Hello vstr, World "), strlen(" is"));
 vstr_del(str1, strlen("Hello "), strlen(" vstr,"));

 ex1_cpy_write(str1, 1);
 
 vstr_del(str1, str1->len - strlen("Hello this is a constant message.\n"),
              strlen("Hello this is a constant message.\n"));
 
 while (str1->len)
 {
  ex1_cpy_write(str1, 1);
  vstr_del(str1, 1, 1);
 }

 /* str2 */

 /* this shows explicit memory checking ...
  *  this is nicer in some cases/styles and you can tell what failed.
  *
  * Note that you _have_ to check vstr_init() and vstr_make_*() explicitly */

 if (str1->conf->malloc_bad)
   PROBLEM("Bad malloc_bad flag\n");
 
 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 if (!(netstr_beg1 = vstr_add_netstr_beg(str2)))
   errno = ENOMEM, PROBLEM("vstr_add_netstr_beg:");
 
 if (argc > 1)
 {
  int fd = open(argv[1], O_RDONLY);
  ex1_mmap_ref *ref = malloc(sizeof(ex1_mmap_ref));
  caddr_t addr = NULL;
  struct stat stat_buf;

  if (!ref)
    errno = ENOMEM, PROBLEM("malloc ex1_mmap:");

  if (fd == -1)
    PROBLEM("open:");

  if (fstat(fd, &stat_buf) == -1)
    PROBLEM("fstat:");
  ref->len = stat_buf.st_size;

  addr = mmap(NULL, ref->len, PROT_READ, MAP_SHARED, fd, 0);
  if (addr == (caddr_t)-1)
    PROBLEM("mmap:");
  
  if (close(fd) == -1)
    PROBLEM("close:");
  
  ref->ref.func = ex1_ref_munmap;
  ref->ref.ptr = (char *)addr;
  ref->ref.ref = 0;

  if (offsetof(ex1_mmap_ref, ref))
    PROBLEM("assert");
  
  if (!vstr_add_ref(str2, str2->len, ref->len, &ref->ref, 0))
    errno = ENOMEM, PROBLEM("vstr_add_ref:");

  have_mmaped_file = 1;
 }

 if (!(netstr_beg2 = vstr_add_netstr2_beg(str2)))
   errno = ENOMEM, PROBLEM("vstr_add_netstr2_beg:");
 
 if (!vstr_add_ptr(str2, str2->len,
                   strlen("Hello this is a constant message.\n"),
                   (char *)"Hello this is a constant message.\n"))
   errno = ENOMEM, PROBLEM("vstr_add_ptr:");
 
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
                    UINTMAX_MAX
                    ))
    errno = ENOMEM, PROBLEM("vstr_add_fmt:");

  /* gcc doesn't understand i18n param numbers */
  fool_gcc_fmt = (" Testing i18n explicit param numbers...\n"
                  " 1=%1$*1$.*1$d\n"
                  " 2=%2$*2$.*2$d\n"
                  " 2=%2$*4$.*1$d\n"
                  " 3=%3$*1$.*2$jd\n"
                  " 4=%4$*4$.*4$d\n"
                  " 1=%1$*2$.*4$d\n");
  if (!vstr_add_fmt(str2, str2->len, fool_gcc_fmt, 1, 2, (intmax_t)3, 4))
    errno = ENOMEM, PROBLEM("vstr_add_fmt:");

  if (!vstr_add_fmt(str2, str2->len, " Testing wchar_t support...\n"
                    "%s: %S\n"
                    "%c: %C\n",
                    "abcd", L"abcd",
                    'z', (wint_t)L'z'))
    errno = ENOMEM, PROBLEM("vstr_add_fmt:");
}
 
 if (!vstr_add_fmt(str2, str2->len, " Testing %%p=%p.\n", str2))
   errno = ENOMEM, PROBLEM("vstr_add_fmt:");
 
 if (!vstr_add_netstr2_end(str2, netstr_beg2))
   errno = ENOMEM, PROBLEM("vstr_add_netstr2_end:");
 if (!vstr_add_netstr_end(str2, netstr_beg1))
   errno = ENOMEM, PROBLEM("vstr_add_netstr_end:");
 
 if (!(netstr_beg1 = vstr_add_netstr_beg(str2)))
   errno = ENOMEM, PROBLEM("vstr_add_netstr_beg:");
 if (!vstr_add_netstr_end(str2, netstr_beg1))
   errno = ENOMEM, PROBLEM("vstr_add_netstr_end:");
 
 if (!(netstr_beg2 = vstr_add_netstr2_beg(str2)))
   errno = ENOMEM, PROBLEM("vstr_add_netstr2_beg:");
 if (!vstr_add_netstr2_end(str2, netstr_beg2))
   errno = ENOMEM, PROBLEM("vstr_add_netstr2_end:");

 if (!vstr_add_buf(str2, str2->len, 1, "\n"))
   errno = ENOMEM, PROBLEM("vstr_add_buf:");

 while (str2->len)
 {
  ex1_cpy_write(str2, 1);
  vstr_del(str2, 1, 1);
 }

 if (have_mmaped_file)
   PROBLEM("bad munmap");
 
 exit (EXIT_SUCCESS);
}
