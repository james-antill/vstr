
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "ex_utils.h"


/* die with error */
void ex_utils_die(const char *msg, ...)
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
    va_list ap;
    
    va_start(ap, msg);
    vstr_add_vfmt(str, str->len, msg, ap);
    va_end(ap);
    
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

/* io */

int ex_utils_read(Vstr_base *str1, int fd)
{
  ssize_t bytes = -1;
  struct iovec *iovs = NULL;
  unsigned int num = 0;
  
  if (!vstr_add_iovec_buf_beg(str1, str1->len, 4, 32, &iovs, &num))
    errno = ENOMEM, DIE("vstr_add_iovec_buf_beg:");
  
  bytes = readv(fd, iovs, num);
  if ((bytes == -1) && (errno == EAGAIN))
  {
    vstr_add_iovec_buf_end(str1, str1->len, 0);
    return (TRUE);
  }
  
  if (bytes == -1)
    DIE("readv:");
  
  vstr_add_iovec_buf_end(str1, str1->len, bytes);
  
  return (!!bytes);
}

int ex_utils_write(Vstr_base *base, int fd)
{
  struct iovec *vec;
  unsigned int num = 0;
  size_t len = 0;
  ssize_t bytes = 0;
  
  len = vstr_export_iovec_ptr_all(base, &vec, &num);
  if (!len)
    errno = ENOMEM, DIE("vstr_export_iovec_ptr_all:");
  
  bytes = writev(fd, vec, num);
  
  if ((bytes == -1) && (errno != EAGAIN))
    DIE("writev:");
  if (bytes == -1)
    bytes = 0;
  
  assert((size_t)bytes <= len);
  
  vstr_del(base, 1, (size_t)bytes);
  
  return (!!bytes);
}

void ex_utils_cpy_write_all(Vstr_base *base, int fd)
{
  static Vstr_base *cpy = NULL;
  
  if (base->conf->malloc_bad)
    errno = ENOMEM, DIE("before ex2_cpy_write:");
  if ((!cpy && !(cpy = vstr_make_base(NULL))) || cpy->conf->malloc_bad)
    errno = ENOMEM, DIE("vstr_make_base:");
  
  vstr_add_vstr(cpy, 0, base, 1, base->len, VSTR_TYPE_ADD_BUF_PTR);
  if (cpy->conf->malloc_bad)
    errno = ENOMEM, DIE("vstr_add_vstr:");
  
  while (cpy->len) ex_utils_write(cpy, fd);
}

/* mmap file */

typedef struct ex_utils_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex_utils_mmap_ref;

static int have_mmaped_file = 0;

static void ex_utils_ref_munmap(Vstr_ref *passed_ref)
{
  ex_utils_mmap_ref *ref = (ex_utils_mmap_ref *)passed_ref;
  munmap(ref->ref.ptr, ref->len);
  free(ref);
  
  assert(have_mmaped_file > 0);
  --have_mmaped_file;
}

void ex_utils_append_file(Vstr_base *str1, const char *filename,
                          int out_fd, size_t max)
{
  int fd = open(filename, O_RDONLY);
  ex_utils_mmap_ref *ref = malloc(sizeof(ex_utils_mmap_ref));
  caddr_t addr = NULL;
  struct stat stat_buf;
  
  if (!ref)
    errno = ENOMEM, DIE("malloc ex_utils_mmap:");
  
  if (fd == -1)
    DIE("open:");
  
  if (fstat(fd, &stat_buf) == -1)
    DIE("fstat:");

  if (!S_ISREG(stat_buf.st_mode))
  {
    int tmp_fcntl_flags = -1;
    
    if ((tmp_fcntl_flags = fcntl(fd, F_GETFL)) == -1)
      DIE("fcntl(GET NONBLOCK):");
    if (!(tmp_fcntl_flags & O_NONBLOCK) &&
        (fcntl(fd, F_SETFL, tmp_fcntl_flags | O_NONBLOCK) == -1))
      DIE("fcntl(SET NONBLOCK):");
    
    while (ex_utils_read(str1, fd))
      if (out_fd != -1)
      {
        while (str1->len >= max)
          ex_utils_write(str1, out_fd);
      }
    
    if (close(fd) == -1)
      DIE("close:");
    return;
  }
  
  ref->len = stat_buf.st_size;
  
  addr = mmap(NULL, ref->len, PROT_READ, MAP_SHARED, fd, 0);
  if (addr == (caddr_t)-1)
    DIE("mmap:");
  
  if (close(fd) == -1)
    DIE("close:");
  
  ref->ref.func = ex_utils_ref_munmap;
  ref->ref.ptr = (char *)addr;
  ref->ref.ref = 0;
  
  if (offsetof(ex_utils_mmap_ref, ref))
    assert(FALSE);
  
  if (!vstr_add_ref(str1, str1->len, &ref->ref, 0, ref->len))
    errno = ENOMEM, DIE("vstr_add_ref:");
  
  ++have_mmaped_file;
}

/* check everything went ok */

void ex_utils_check(void)
{
  assert(!have_mmaped_file);
}
