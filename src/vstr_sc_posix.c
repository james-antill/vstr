#define VSTR_SC_POSIX_C
/*
 *  Copyright (C) 2002  James Antill
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *   
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *  email: james@and.org
 */
/* functions which are shortcuts */
#include "main.h"

#ifndef HAVE_MMAP64
# define mmap64 mmap /* FIXME: really crap */
#endif

#ifndef HAVE_MMAP
# define VSTR__SC_ENOSYS(x) \
  if (err) \
  { \
    errno = ENOSYS; \
    *err = (x); \
  } \
  \
  return (FALSE)
#else
static void vstr__sc_ref_munmap(Vstr_ref *passed_ref)
{
  Vstr__sc_mmap_ref *mmap_ref = (Vstr__sc_mmap_ref *)passed_ref;
  
  munmap(mmap_ref->ref.ptr, mmap_ref->mmap_len); /* this _can't_ be -1 */
  
  VSTR__F(mmap_ref);

  ASSERT(vstr__options.mmap_count-- > 0);
}
#endif



#ifndef HAVE_MMAP
int vstr_nx_sc_mmap_fd(Vstr_base *base __attribute__((unused)),
                       size_t pos __attribute__((unused)),
                       int fd __attribute__((unused)),
                       VSTR_AUTOCONF_off64_t off __attribute__((unused)),
                       size_t len __attribute__((unused)),
                       unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_MMAP_FD_ERR_MMAP_ERRNO); }
#else
int vstr_nx_sc_mmap_fd(Vstr_base *base, size_t pos, int fd,
                       VSTR_AUTOCONF_off64_t off, size_t len, unsigned int *err)
{
  unsigned int dummy_err;
  void *addr = NULL;
  Vstr__sc_mmap_ref *mmap_ref = NULL;

  assert(off >= 0); /* off is offset from the start of the file,
                     * not as in seek */
  
  if (!err)
    err = &dummy_err;
  *err = 0;

  if (!len)
  {
    struct stat64 stat_buf;
    
    if (fstat64(fd, &stat_buf) == -1)
    {
      *err = VSTR_TYPE_SC_MMAP_FD_ERR_FSTAT_ERRNO;
      return (FALSE);
    }
    
    if (stat_buf.st_size <= off)
      return (TRUE);

    len = (stat_buf.st_size - off);
    if (len > (SIZE_MAX - base->len))
    {
      *err = VSTR_TYPE_SC_MMAP_FD_ERR_TOO_LARGE;
      errno = EFBIG;
      return (FALSE);
    }
  }

  addr = mmap64(NULL, len, PROT_READ, MAP_PRIVATE, fd, off);
  if (addr == MAP_FAILED)
  {
    *err = VSTR_TYPE_SC_MMAP_FD_ERR_MMAP_ERRNO;
    return (FALSE);
  }

  if (!(mmap_ref = VSTR__MK(sizeof(Vstr__sc_mmap_ref))))
    goto malloc_mmap_ref_failed;
  mmap_ref->mmap_len = len;
  mmap_ref->ref.func = vstr__sc_ref_munmap;
  mmap_ref->ref.ptr = (void *)addr;
  mmap_ref->ref.ref = 0;

  if (!vstr_nx_add_ref(base, pos, &mmap_ref->ref, 0, len))
    goto add_ref_failed;

  assert(++vstr__options.mmap_count);
  
  return (TRUE);

 add_ref_failed:
  VSTR__F(mmap_ref);
 malloc_mmap_ref_failed:
  munmap(addr, len);
  *err = VSTR_TYPE_SC_MMAP_FILE_ERR_MEM;
  errno = ENOMEM;
  base->conf->malloc_bad = TRUE;
  
  return (FALSE);
}
#endif

#ifndef HAVE_MMAP
int vstr_nx_sc_mmap_file(Vstr_base *base __attribute__((unused)),
                         size_t pos __attribute__((unused)),
                         const char *filename __attribute__((unused)),
                         VSTR_AUTOCONF_off64_t off __attribute__((unused)),
                         size_t len __attribute__((unused)),
                         unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_MMAP_FD_ERR_MMAP_ERRNO); }
#else
int vstr_nx_sc_mmap_file(Vstr_base *base, size_t pos, const char *filename,
                         VSTR_AUTOCONF_off64_t off, size_t len, 
                         unsigned int *err)
{
  int fd = open(filename, O_RDONLY | O_LARGEFILE | O_NOCTTY);
  unsigned int dummy_err;
  int ret = 0;
  int saved_errno = 0;
  
  if (!err)
    err = &dummy_err;
  
  if (fd == -1)
  {
    *err = VSTR_TYPE_SC_MMAP_FILE_ERR_OPEN_ERRNO;
    return (FALSE);
  }

  ret = vstr_nx_sc_mmap_fd(base, pos, fd, off, len, err);

  if (*err)
    saved_errno = errno;
  
  if ((close(fd) == -1) && !*err)
    *err = VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO;

  if (saved_errno)
    errno = saved_errno;
  
  return (ret);
}
#endif

static int vstr__sc_read_slow_len_fd(Vstr_base *base, size_t pos, int fd,
                                     size_t len, unsigned int *err)
{
  size_t orig_pos = pos;
  ssize_t bytes = -1;
  unsigned int num = 0;
  Vstr_ref *ref = NULL;
  
  if (pos && !vstr__add_setup_pos(base, &pos, &num, NULL))
    goto mem_fail;
  pos = orig_pos;
  
  if (!(ref = vstr_nx_ref_make_malloc(len)))
    goto mem_fail;
  
  if (!base->conf->spare_ref_num &&
      !vstr_nx_make_spare_nodes(base->conf, VSTR_TYPE_NODE_REF, 1))
    goto mem_ref_fail;
    
  do
  {
    bytes = read(fd, ref->ptr, len);
  } while ((bytes == -1) && (errno == EINTR));
  
  if (bytes == -1)
  {
    *err = VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO;
    return (FALSE);
  }
  if (!bytes)
  {
    *err = VSTR_TYPE_SC_READ_FD_ERR_EOF;
    errno = ENOSPC;
    return (FALSE);
  }
    
  vstr_nx_add_ref(base, pos, ref, 0, bytes); /* must work */

  vstr_nx_ref_del(ref);
  
  return (TRUE);

 mem_ref_fail:
  assert(ref);
  vstr_nx_ref_del(ref);
 mem_fail:
  *err = VSTR_TYPE_SC_READ_FD_ERR_MEM;
  errno = ENOMEM;
  return (FALSE);
}

static int vstr__sc_read_fast_iov_fd(Vstr_base *base, size_t pos, int fd,
                                     struct iovec *iovs, unsigned int num,
                                     unsigned int *err)
{
  ssize_t bytes = -1;
  
  if (num > UIO_MAXIOV)
    num = UIO_MAXIOV;
  
  do
  {
    bytes = readv(fd, iovs, num);
  } while ((bytes == -1) && (errno == EINTR));
  
  if (bytes == -1)
  {
    vstr_nx_add_iovec_buf_end(base, pos, 0);
    *err = VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO;
    return (FALSE);
  }

  vstr_nx_add_iovec_buf_end(base, pos, (size_t)bytes);
  
  if (!bytes)
  {
    *err = VSTR_TYPE_SC_READ_FD_ERR_EOF;
    errno = ENOSPC;
    return (FALSE);
  }

  return (TRUE);
}

int vstr_nx_sc_read_iov_fd(Vstr_base *base, size_t pos, int fd,
                           unsigned int min, unsigned int max,
                           unsigned int *err)
{
  struct iovec *iovs = NULL;
  unsigned int num = 0;
  unsigned int dummy_err;

  if (!err)
    err = &dummy_err;
  *err = 0;

  assert(max >= min);

  if (!min)
    return (TRUE);
  
  if (!base->cache_available)
    return (vstr__sc_read_slow_len_fd(base, pos, fd,
                                      min * base->conf->buf_sz, err));
  
  /* use iovec internal add -- much quicker, one syscall no double copy  */
  if (!vstr_nx_add_iovec_buf_beg(base, pos, min, max, &iovs, &num))
  {
    *err = VSTR_TYPE_SC_READ_FD_ERR_MEM;
    errno = ENOMEM;
    return (FALSE);
  }

  return (vstr__sc_read_fast_iov_fd(base, pos, fd, iovs, num, err));
}

#define IOV_SZ(iovs, num) \
 (iovs[0].iov_len + ((num > 1) ? iovs[num - 1].iov_len : 0) + \
                    ((num > 2) ? ((num - 2) * base->conf->buf_sz) : 0))

static int vstr__sc_read_len_fd(Vstr_base *base, size_t pos, int fd,
                                size_t len, unsigned int *err)
{
  struct iovec *iovs = NULL;
  unsigned int num = 0;
  unsigned int ios = 0;

  if (!base->cache_available)
    return (vstr__sc_read_slow_len_fd(base, pos, fd, len, err));

  /* guess at 2 over size, as we usually lose some in the division
   * and we can lose a lot on iovs[0] */
  ios = len / base->conf->buf_sz;
  ios += 2;
  if (!vstr_nx_add_iovec_buf_beg(base, pos, ios, ios, &iovs, &num))
  {
    *err = VSTR_TYPE_SC_READ_FD_ERR_MEM;
    errno = ENOMEM;
    return (FALSE);
  }
  
  /* fixup iovs for exact size of len, if bigger */
  assert(num && ((num == 1) || (iovs[num - 1].iov_len == base->conf->buf_sz)));
  
  assert(IOV_SZ(iovs, num) >= len);
  while (IOV_SZ(iovs, num) >  len)
  {
    size_t tmp = 0;

    tmp = (IOV_SZ(iovs, num) -  len);
    if (tmp >= iovs[num - 1].iov_len)
    {
      --num;
      continue;
    }
    
    iovs[num - 1].iov_len  -= tmp;
    assert(IOV_SZ(iovs, num) == len);
  }
  assert(IOV_SZ(iovs, num) == len);

  return (vstr__sc_read_fast_iov_fd(base, pos, fd, iovs, num, err));
}
#undef IOV_SZ

int vstr_nx_sc_read_len_fd(Vstr_base *base, size_t pos, int fd,
                           size_t len, unsigned int *err)
{
  unsigned int dummy_err;
  const size_t off = 0;
  
  if (!err)
    err = &dummy_err;
  *err = 0;

  if (!len)
  {
    struct stat64 stat_buf;
    
    if (fstat64(fd, &stat_buf) == -1)
    {
      *err = VSTR_TYPE_SC_READ_FD_ERR_FSTAT_ERRNO;
      return (FALSE);
    }
    
    if (stat_buf.st_size <= off)
      return (TRUE);

    len = (stat_buf.st_size - off);
    if (len > (SIZE_MAX - base->len))
    {
      *err = VSTR_TYPE_SC_READ_FD_ERR_TOO_LARGE;
      errno = EFBIG;
      return (FALSE);
    }
  }  

  return (vstr__sc_read_len_fd(base, pos, fd, len, err));
}

int vstr_nx_sc_read_iov_file(Vstr_base *base, size_t pos,
                             const char *filename, VSTR_AUTOCONF_off64_t off,
                             unsigned int min, unsigned int max,
                             unsigned int *err)
{
  int fd = open(filename, O_RDONLY | O_LARGEFILE | O_NOCTTY);
  unsigned int dummy_err;
  int ret = 0;
  int saved_errno = 0;
  
  if (!err)
    err = &dummy_err;

  if (fd == -1)
  {
    *err = VSTR_TYPE_SC_READ_FILE_ERR_OPEN_ERRNO;
    return (FALSE);
  }
  
  if (off)
  {
    if (lseek64(fd, off, SEEK_SET) == -1)
    {
      *err = VSTR_TYPE_SC_READ_FILE_ERR_SEEK_ERRNO;      
      goto failed;
    }
  }

  ret = vstr_nx_sc_read_iov_fd(base, pos, fd, min, max, err);
    
 failed:
  if (*err)
    saved_errno = errno;
  
  if ((close(fd) == -1) && !*err)
    *err = VSTR_TYPE_SC_READ_FILE_ERR_CLOSE_ERRNO;

  if (saved_errno)
    errno = saved_errno;

  return (ret);
}

int vstr_nx_sc_read_len_file(Vstr_base *base, size_t pos,
                             const char *filename,
                             VSTR_AUTOCONF_off64_t off, size_t len,
                             unsigned int *err)
{
  int fd = open(filename, O_RDONLY | O_LARGEFILE | O_NOCTTY);
  unsigned int dummy_err;
  int ret = 0;
  int saved_errno = 0;
  
  if (!err)
    err = &dummy_err;

  if (fd == -1)
  {
    *err = VSTR_TYPE_SC_READ_FILE_ERR_OPEN_ERRNO;
    return (FALSE);
  }

  if (!len)
  {
    struct stat64 stat_buf;
    
    if (fstat64(fd, &stat_buf) == -1)
    {
      *err = VSTR_TYPE_SC_READ_FILE_ERR_FSTAT_ERRNO;
      return (FALSE);
    }

    if (stat_buf.st_size <= off)
      return (TRUE);

    len = (stat_buf.st_size - off);
    if (len > (SIZE_MAX - base->len))
    {
      *err = VSTR_TYPE_SC_READ_FILE_ERR_TOO_LARGE;
      errno = EFBIG;
      return (FALSE);
    }
  }
  
  if (off)
  {
    if (lseek64(fd, off, SEEK_SET) == -1)
    {
      *err = VSTR_TYPE_SC_READ_FILE_ERR_SEEK_ERRNO;      
      goto failed;
    }
  }

  ret = vstr__sc_read_len_fd(base, pos, fd, len, err);
    
 failed:
  if (*err)
    saved_errno = errno;
  
  if ((close(fd) == -1) && !*err)
    *err = VSTR_TYPE_SC_READ_FILE_ERR_CLOSE_ERRNO;

  if (saved_errno)
    errno = saved_errno;

  return (ret);
}

int vstr_nx_sc_write_fd(Vstr_base *base, size_t pos, size_t len, int fd,
                        unsigned int *err)
{
  unsigned int dummy_err;

  if (!err)
    err = &dummy_err;
  *err = 0;

  assert(len || (pos == 1));
  if (!len)
    return (TRUE);
  
  while (len)
  {
    struct iovec cpy_vec[32];
    struct iovec *vec;
    unsigned int num = 0;
    ssize_t bytes = 0;

    if ((pos == 1) && (len == base->len) && base->cache_available)
      len = vstr_nx_export_iovec_ptr_all(base, &vec, &num);
    else
    {
      vec = cpy_vec;
      bytes = vstr_nx_export_iovec_cpy_ptr(base, pos, len, vec, 32, &num);
      assert(bytes);
    }
    
    if (!len)
    {
      *err = VSTR_TYPE_SC_WRITE_FD_ERR_MEM;
      errno = ENOMEM;
      return (FALSE);
    }
    assert(len == base->len);

    if (num > UIO_MAXIOV)
      num = UIO_MAXIOV;
    
    do
    {
      bytes = writev(fd, vec, num);
    } while ((bytes == -1) && (errno == EINTR));
    
    if (bytes == -1)
    {
      *err = VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO;
      return (FALSE);
    }
    
    assert((size_t)bytes <= len);
    
    vstr_nx_del(base, pos, (size_t)bytes);
    len -= (size_t)bytes;
  }

  return (TRUE);
}

int vstr_nx_sc_write_file(Vstr_base *base, size_t pos, size_t len,
                          const char *filename, int open_flags,
                          VSTR_AUTOCONF_mode_t mode,
                          VSTR_AUTOCONF_off64_t off, unsigned int *err)
{
  int fd = -1;
  unsigned int dummy_err;
  int ret = 0;
  int saved_errno = 0;
  
  if (!err)
    err = &dummy_err;

  if (!open_flags) /* O_RDONLY isn't valid, for obvious reasons */
    open_flags = (O_CREAT | O_EXCL);
  
  fd = open(filename, open_flags, mode);
  if (fd == -1)
  {
    *err = VSTR_TYPE_SC_WRITE_FILE_ERR_OPEN_ERRNO;
    return (FALSE);
  }

  if (off)
  {
    if (lseek64(fd, off, SEEK_SET) == -1)
    {
      *err = VSTR_TYPE_SC_WRITE_FILE_ERR_SEEK_ERRNO;      
      goto failed;
    }
  }
  
  ret = vstr_nx_sc_write_fd(base, pos, len, fd, err);

 failed:
  if (*err)
    saved_errno = errno;
  
  if ((close(fd) == -1) && !*err)
    *err = VSTR_TYPE_SC_WRITE_FILE_ERR_CLOSE_ERRNO;

  if (saved_errno)
    errno = saved_errno;

  return (ret);
}

static int vstr__sc_fmt_add_cb_ipv4_ptr(Vstr_base *base, size_t pos,
                                        Vstr_fmt_spec *spec)
{
  struct in_addr *ipv4 = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t obj_len = 0;
  char buf[1024];
  const char *ptr = NULL;

  assert(ipv4);
  assert(sizeof(buf) >= INET_ADDRSTRLEN);
  
  ptr = inet_ntop(AF_INET, ipv4, buf, sizeof(buf));
  if (!ptr) ptr = "0.0.0.0";

  obj_len = strlen(ptr);
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &obj_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);

  if (!vstr_nx_add_buf(base, pos, ptr, obj_len))
    return (FALSE);  
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, obj_len))
    return (FALSE);

  return (TRUE);
}

static int vstr__sc_fmt_add_cb_ipv6_ptr(Vstr_base *base, size_t pos,
                                        Vstr_fmt_spec *spec)
{
  struct in6_addr *ipv6 = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t obj_len = 0;
  char buf[1024];
  const char *ptr = NULL;

  assert(ipv6);
  assert(sizeof(buf) >= INET6_ADDRSTRLEN);
  
  ptr = inet_ntop(AF_INET6, ipv6, buf, sizeof(buf));
  if (!ptr) ptr = "::";

  obj_len = strlen(ptr);
  
  if (!vstr_nx_sc_fmt_cb_beg(base, &pos, spec, &obj_len,
                             VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR))
    return (FALSE);

  if (!vstr_nx_add_buf(base, pos, ptr, obj_len))
    return (FALSE);  
  
  if (!vstr_nx_sc_fmt_cb_end(base, pos, spec, obj_len))
    return (FALSE);

  return (TRUE);
}

int vstr_nx_sc_fmt_add_ipv4_ptr(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_ipv4_ptr,
                          VSTR_TYPE_FMT_PTR_VOID,
                          VSTR_TYPE_FMT_END));
}

int vstr_nx_sc_fmt_add_ipv6_ptr(Vstr_conf *conf, const char *name)
{
  return (vstr_nx_fmt_add(conf, name, vstr__sc_fmt_add_cb_ipv6_ptr,
                          VSTR_TYPE_FMT_PTR_VOID,
                          VSTR_TYPE_FMT_END));
}
