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

typedef struct ex2_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex2_mmap_ref;

#if 0
static void ex2_ref_munmap(Vstr_ref *passed_ref)
{
 ex2_mmap_ref *ref = (ex2_mmap_ref *)passed_ref;
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

static void ex2_cpy_write(Vstr_base *base, int fd)
{
 static Vstr_base *cpy = NULL;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;

 if (base->conf->malloc_bad)
   errno = ENOMEM, PROBLEM("before ex2_cpy_write:");
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
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 Vstr_base *str2 = NULL;
 /* size_t netstr_beg1 = 0;
    size_t netstr_beg2 = 0; */
 int count = 1;
 
 if (!vstr_init())
   errno = ENOMEM, PROBLEM("vstr_init:");
 
 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, PROBLEM("vstr_make_conf:");
 
 /* have only 1 character per _buf node */
 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 4);
 vstr_cntl_opt(VSTR_CNTL_OPT_SET_CONF, conf);
 
 str1 = vstr_make_base(NULL);
 if (!str1)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 str2 = vstr_make_base(NULL);
 if (!str2)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 /* This is all implicit memory checking ...
  *  this is nicer in some cases/styles but you can't tell what failed.
  * The checking is done inside ex2_cpy_write() via. base->conf->malloc_bad
  *
  * Note that you _have_ to check vstr_init(), and vstr_make_*() explicitly */

 vstr_add_fmt(str1, str1->len, "%s", "Test cmp Y ");
 vstr_add_non(str1, str1->len, 1);
 vstr_add_fmt(str1, str1->len, "aaaa%uY\n", UINT_MAX);

 while (count < 80)
 {
  int val = count % 10;
  
  if (val)
    vstr_add_fmt(str2, str2->len, "%d", val);
  else
    vstr_add_fmt(str2, str2->len, "%c", ' ');
  ++count;
 }
 vstr_add_fmt(str2, str2->len, "%c", '\n');

 vstr_add_fmt(str2, str2->len, "Using string "
              "(where 'X' is a non character)...\n\n");
 vstr_add_vstr(str2, str2->len, str1, 1, strlen("Test cmp Y "),
               VSTR_TYPE_ADD_BUF_PTR);
 vstr_add_buf(str2, str2->len, "X", 1);
 vstr_add_vstr(str2, str2->len, str1, strlen("Test cmp Y Xa"),
               str1->len - strlen("Test cmp Y X"), VSTR_TYPE_ADD_BUF_PTR);
 vstr_add_fmt(str2, str2->len, "\n");
 
 vstr_add_fmt(str2, str2->len, "Testing vstr_srch_*\n");

 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "chr(%d,%zu,%s): %zu %zu %zu\n",
              1, str1->len, "\\n",
              str1->len,
              vstr_srch_chr_fwd(str1, 1, str1->len, '\n'),
              vstr_srch_chr_rev(str1, 1, str1->len, '\n'));
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 vstr_add_fmt(str2, str2->len, "chr(%d,%zu,%c): %d %zu %zu\n",
              1, str1->len, 'T',
              1,
              vstr_srch_chr_fwd(str1, 1, str1->len, 'T'),
              vstr_srch_chr_rev(str1, 1, str1->len, 'T'));

 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 vstr_add_fmt(str2, str2->len, "chr(%d,%zu,%c): %d %zu %zu\n",
              1, str1->len, 'Z',
              0,
              vstr_srch_chr_fwd(str1, 1, str1->len, 'Z'),
              vstr_srch_chr_rev(str1, 1, str1->len, 'Z'));

 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 vstr_add_fmt(str2, str2->len, "chr(%d,%zu,%c): (%zu,%zu) (%zu,%zu)\n",
              1, str1->len, 'Y',
              strlen("Test cmp Y"), str1->len - 1,
              vstr_srch_chr_fwd(str1, 1, str1->len, 'Y'),
              vstr_srch_chr_rev(str1, 1, str1->len, 'Y'));

 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "buf(%d,%zu,%s): %d %zu %zu\n",
              1, str1->len, "Test",
              1,
              vstr_srch_buf_fwd(str1, 1, str1->len, "Test", strlen("Test")),
              vstr_srch_buf_rev(str1, 1, str1->len, "Test", strlen("Test")));

 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 vstr_add_fmt(str2, str2->len, "buf(%d,%zu,%s): %zu %zu %zu\n",
              1, str1->len, "aaaa",
              strlen("Test cmp Y Xa"),
              vstr_srch_buf_fwd(str1, 1, str1->len, "aaaa", strlen("aaaa")),
              vstr_srch_buf_rev(str1, 1, str1->len, "aaaa", strlen("aaaa")));

 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "buf(%d,%zu,%s): %zu %zu %zu\n",
              1, str1->len, "a4",
              strlen("Test cmp Y Xaaaa"),
              vstr_srch_buf_fwd(str1, 1, str1->len, "a4", strlen("a4")),
              vstr_srch_buf_rev(str1, 1, str1->len, "a4", strlen("a4")));

 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "spn(%d,%zu,%s): %d %zu %zu\n",
              1, str1->len, "a",
              0,
              vstr_spn_buf_fwd(str1, 1, str1->len, "a", 1),
              vstr_spn_buf_rev(str1, 1, str1->len, "a", 1)); 
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "spn(%zu,%zu,%s): %d %zu %zu\n",
              strlen("Test cmp Y X"), str1->len - strlen("Test cmp Y "), "a",
              0,
              vstr_spn_buf_fwd(str1, strlen("Test cmp Y "),
                               str1->len - strlen("Test cmp Y "), "a", 1),
              vstr_spn_buf_rev(str1, 1, strlen("Test cmp Y "), "a", 1));
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "spn(%zu,%zu,%s): %d %zu %zu\n",
              strlen("Test cmp Y Xa"),
              str1->len - strlen("Test cmp Y Xa"), "a",
              4,
              vstr_spn_buf_fwd(str1, strlen("Test cmp Y Xa"),
                               str1->len - strlen("Test cmp Y Xa"), "a", 1),
              vstr_spn_buf_rev(str1, 1, strlen("Test cmp Y Xaaaa"), "a", 1));
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "spn(%zu,%zu,%s): %d %zu %zu\n",
              strlen("Test cmp Y Xa"),
              str1->len - (int)strlen("Test cmp Y Xa"), "a1234567890",
              14,
              vstr_spn_buf_fwd(str1, strlen("Test cmp Y Xa"),
                               str1->len - strlen("Test cmp Y Xa"),
                               "a1234567890", 11),
              vstr_spn_buf_rev(str1, 1, strlen("Test cmp Y Xaaaa1111111111"),
                               "a1234567890", 11));
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);
 
 vstr_add_fmt(str2, str2->len, "cspn(%d,%zu,%s): %d %zu %zu\n",
              1, str1->len, "\\nT",
              0,
              vstr_cspn_buf_fwd(str1, 1, str1->len, "\nT", 2),
              vstr_cspn_buf_rev(str1, 1, str1->len, "\nT", 2)); 
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 vstr_add_fmt(str2, str2->len, "cspn(%d,%zu,%s): %zu %zu %zu\n",
              1, str1->len, "Z",
              str1->len,
              vstr_cspn_buf_fwd(str1, 1, str1->len, "Z", 1),
              vstr_cspn_buf_rev(str1, 1, str1->len, "Z", 1)); 
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 vstr_add_fmt(str2, str2->len, "cspn(%d,%zu,%s): %zu %zu %zu\n",
              1, str1->len, "a",
              strlen("Test cmp Y X"),
              vstr_cspn_buf_fwd(str1, 1, str1->len, "a", 1),
              vstr_cspn_buf_rev(str1, 1, strlen("Test cmp Y X"), "a", 1)); 
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 vstr_add_fmt(str2, str2->len, "cspn(%zu,%zu,%s): %d %zu %zu\n",
              strlen("Test cmp Y Xa"),
              str1->len - strlen("Test cmp Y Xa"), "1234567890",
              4,
              vstr_cspn_buf_fwd(str1, strlen("Test cmp Y Xa"),
                                str1->len - strlen("Test cmp Y Xa"),
                                "1234567890", 10),
              vstr_cspn_buf_rev(str1, 1, strlen("Test cmp Y Xaaaa"), NULL, 1));
 
 ex2_cpy_write(str2, 1);
 vstr_del(str2, 1, str2->len);

 exit (EXIT_SUCCESS);
}
