#define _GNU_SOURCE

/* This is a cat program, no command line options
 * but does stdin if no args are given */

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
#include <sys/poll.h>
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

#if 1
# define MIN_BEFORE_WRITE (1024 * 8)
#else
# define MIN_BEFORE_WRITE 1
#endif

typedef struct ex_cat_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex_cat_mmap_ref;

static int have_mmaped_file = 0;

static void ex_cat_ref_munmap(Vstr_ref *passed_ref)
{
 ex_cat_mmap_ref *ref = (ex_cat_mmap_ref *)passed_ref;
 munmap(ref->ref.ptr, ref->len);
 free(ref);

 --have_mmaped_file;
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

static int ex_cat_readv(Vstr_base *str1, int fd)
{
 ssize_t bytes = -1;
 struct iovec *iovs = NULL;
 unsigned int num = 0;
 
 if (!vstr_add_iovec_buf_beg(str1, 4, 32, &iovs, &num))
   errno = ENOMEM, PROBLEM("vstr_add_iovec_buf_beg:");
 
 bytes = readv(fd, iovs, num);
 if ((bytes == -1) && (errno == EAGAIN))
   bytes = 0;
 if (bytes == -1)
   PROBLEM("readv:");
 
 vstr_add_iovec_buf_end(str1, bytes);

 return (!!bytes);
}

static void ex_cat_del_write(Vstr_base *base, int fd)
{
 Vstr_base *cpy = base;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;
 
 len = vstr_export_iovec_ptr_all(cpy, &vec, &num);
 if (!len)
   errno = ENOMEM, PROBLEM("vstr_export_iovec_ptr_all:");

 if ((size_t)(bytes = writev(fd, vec, num)) != len)
 {
  if ((bytes == -1) && (errno != EAGAIN))
    PROBLEM("writev:");
  if (bytes == -1)
    return;
 }
 
 vstr_del(cpy, 1, (size_t)bytes);
}

#if 1
/* this sort of kludly works like something that stops accepting write()
 * Ie. pipeing to slowcat or netcat ... but not too well.
 * Then again I've no idea how to pipe commands up for testing in gdb */
# define ex_cat_del_slow_write(x, y) ex_cat_del_write(x, y)
#else
static void ex_cat_del_slow_write(Vstr_base *base, int fd)
{ /* only write 1 char at a time */
 Vstr_base *cpy = base;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;

 if (base->len < MIN_BEFORE_WRITE)
   return;
 
 len = vstr_export_iovec_ptr_all(cpy, &vec, &num);
 if (!len)
   errno = ENOMEM, PROBLEM("vstr_export_iovec_ptr_all:");
 
 if ((size_t)(bytes = write(fd, vec[0].iov_base, 1)) != len)
 {
  if ((bytes == -1) && (errno != EAGAIN))
    PROBLEM("writev:");
  if (bytes == -1)
    return;
 }
 
 vstr_del(cpy, 1, (size_t)bytes);
}
#endif

static void ex_cat_mmap_file(Vstr_base *str1, int fd, size_t len)
{
 ex_cat_mmap_ref *ref = NULL;
 caddr_t addr = NULL;

 if (!len)
   return;
 
 if (!(ref = malloc(sizeof(ex_cat_mmap_ref))))
   errno = ENOMEM, PROBLEM("malloc ex_cat_mmap:");
 
 ref->len = len;
 
 addr = mmap(NULL, ref->len, PROT_READ, MAP_SHARED, fd, 0);
 if (addr == (caddr_t)-1)
   PROBLEM("mmap:");
 
 if (close(fd) == -1)
   PROBLEM("close:");
 
 ref->ref.func = ex_cat_ref_munmap;
 ref->ref.ptr = (char *)addr;
 ref->ref.ref = 0;
 
 if (offsetof(ex_cat_mmap_ref, ref))
   PROBLEM("assert");
 
 if (!vstr_add_ref(str1, str1->len, ref->len, &ref->ref, 0))
   errno = ENOMEM, PROBLEM("vstr_add_ref:");

 ++have_mmaped_file;
}

int main(int argc, char *argv[])
{ /* This is "cat", without any command line options */
 Vstr_base *str1 = NULL;
 int count = 1; /* skip the program name */
 int fcntl_flags = 0;
 
 if (!vstr_init())
   errno = ENOMEM, PROBLEM("vstr_init:");
 
 str1 = vstr_make_base(NULL);
 if (!str1)
   errno = ENOMEM, PROBLEM("vstr_make_base:");

 if (count == argc)
 {/* use stdin */
  int fd = 0;
  
  while (ex_cat_readv(str1, fd))
    ex_cat_del_write(str1, 1);
 }

 if ((fcntl_flags = fcntl(1, F_GETFL)) == -1)
   PROBLEM("fcntl(GET NONBLOCK):");
 if (!(fcntl_flags & O_NONBLOCK) &&
     (fcntl(1, F_SETFL, fcntl_flags | O_NONBLOCK) == -1))
   PROBLEM("fcntl(SET NONBLOCK):");

 while (count < argc)
 {
  int fd = open(argv[count], O_RDONLY);
  struct stat stat_buf;

  if (fd == -1)
    PROBLEM("open:");

  if (fstat(fd, &stat_buf) == -1)
    PROBLEM("fstat:");

  if (S_ISREG(stat_buf.st_mode))
  {
   ex_cat_mmap_file(str1, fd, stat_buf.st_size);
   ex_cat_del_slow_write(str1, 1);
  }
  else
  {
   int tmp_fcntl_flags = 0;
   
   if ((tmp_fcntl_flags = fcntl(fd, F_GETFL)) == -1)
     PROBLEM("fcntl(GET NONBLOCK):");
   if (!(tmp_fcntl_flags & O_NONBLOCK) &&
       (fcntl(fd, F_SETFL, tmp_fcntl_flags | O_NONBLOCK) == -1))
     PROBLEM("fcntl(SET NONBLOCK):");
   
   while (ex_cat_readv(str1, fd))
     ex_cat_del_slow_write(str1, 1);
   
   if (close(fd) == -1)
     PROBLEM("close:");
  }
  
  ++count;
 }
 
 if (str1->len) /* set it back to blocking, so we don't eat CPU */
   if (fcntl(1, F_SETFL, fcntl_flags & ~O_NONBLOCK) == -1)
     PROBLEM("fcntl(SET BLOCK):");
 
 while (str1->len)
   ex_cat_del_slow_write(str1, 1);
 
 assert(!have_mmaped_file);
 
 exit (EXIT_SUCCESS);
}
