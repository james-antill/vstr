#define VSTR_SC_C
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

static void vstr__sc_ref_munmap(Vstr_ref *passed_ref)
{
  Vstr__sc_mmap_ref *mmap_ref = (Vstr__sc_mmap_ref *)passed_ref;
  
  munmap(mmap_ref->ref.ptr, mmap_ref->mmap_len); /* this _can't_ be -1 */
  
  free(mmap_ref);
}

int vstr_sc_add_fd(Vstr_base *base, size_t pos, int fd, off_t off, size_t len,
                   unsigned int *err)
{
  int dummy_err;
  caddr_t addr = (caddr_t)-1;
  Vstr__sc_mmap_ref *mmap_ref = NULL;
  
  if (!err)
    err = &dummy_err;
  *err = 0;

  if (!len)
  {
    struct stat stat_buf;
    
    if (fstat(fd, &stat_buf) == -1)
    {
      *err = VSTR_TYPE_SC_ADD_FILE_ERR_FSTAT_ERRNO;
      return (FALSE);
    }
    len = stat_buf.st_size;
  }

  addr = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, off);
  if (addr == (caddr_t)-1)
  {
    *err = VSTR_TYPE_SC_ADD_FILE_ERR_MMAP_ERRNO;
    return (FALSE);
  }

  if (!(mmap_ref = malloc(sizeof(Vstr__sc_mmap_ref))))
    goto malloc_mmap_ref_failed;
  mmap_ref->mmap_len = len;
  mmap_ref->ref.func = vstr__sc_ref_munmap;
  mmap_ref->ref.ptr = (char *)addr;
  mmap_ref->ref.ref = 0;

  if (!vstr_add_ref(base, pos, &mmap_ref->ref, 0, len))
    goto add_ref_failed;

  return (TRUE);

 add_ref_failed:
  free(mmap_ref);
 malloc_mmap_ref_failed:
  munmap(addr, len);
  *err = VSTR_TYPE_SC_ADD_FILE_ERR_MEM;
  base->conf->malloc_bad = TRUE;
  
  return (FALSE);
}

int vstr_sc_add_file(Vstr_base *base, size_t pos, const char *filename,
                     unsigned int *err)
{
  int fd = open(filename, O_RDONLY);
  int dummy_err;
  int ret = 0;
  
  if (!err)
    err = &dummy_err;
  
  if (fd == -1)
  {
    *err = VSTR_TYPE_SC_ADD_FILE_ERR_OPEN_ERRNO;
    return (FALSE);
  }

  ret = vstr_sc_add_fd(base, pos, fd, 0, 0, err);

  if ((close(fd) == -1) && !*err)
    *err = VSTR_TYPE_SC_ADD_FILE_ERR_CLOSE_ERRNO;

  return (ret);
}

int vstr_sc_read_fd(Vstr_base *base, size_t pos, int fd,
                    unsigned int min, unsigned int max,
                    unsigned int *err)
{
  ssize_t bytes = -1;
  struct iovec *iovs = NULL;
  unsigned int num = 0;
  unsigned int dummy_err;

  if (!err)
    err = &dummy_err;
  *err = 0;
  
  if (!vstr_add_iovec_buf_beg(base, pos, min, max, &iovs, &num))
  {
    *err = VSTR_TYPE_SC_READ_FD_ERR_MEM;
    return (FALSE);
  }

  do
  {
    bytes = readv(fd, iovs, num);
  } while ((bytes == -1) && (errno == EINTR));
  
  if (bytes == -1)
  {
    vstr_add_iovec_buf_end(base, base->len, 0);
    *err = VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO;
    return (FALSE);
  }

  vstr_add_iovec_buf_end(base, base->len, (size_t)bytes);
  
  if (!bytes)
  {
    *err = VSTR_TYPE_SC_READ_FD_ERR_EOF;
    return (FALSE);
  }

  return (TRUE);
}

int vstr_sc_write_fd(Vstr_base *base, size_t pos, size_t len, int fd,
                     unsigned int *err)
{
  int dummy_err;

  if (!err)
    err = &dummy_err;
  *err = 0;
  
  if (!len)
    return (TRUE);
  
  while (len)
  {
    struct iovec cpy_vec[8];
    struct iovec *vec;
    unsigned int num = 0;
    ssize_t bytes = 0;

    if ((pos == 1) && (len == base->len))
      len = vstr_export_iovec_ptr_all(base, &vec, &num);
    else
    {
      vec = cpy_vec;
      len = vstr_export_iovec_cpy_ptr(base, pos, len, vec, 8, &num);
    }
    
    if (!len)
    {
      *err = VSTR_TYPE_SC_WRITE_FILE_ERR_MEM;
      return (FALSE);
    }
    assert(len == base->len);

    do
    {
      bytes = writev(fd, vec, num);
    } while ((bytes == -1) && (errno == EINTR));
    
    if (bytes == -1)
    {
      *err = VSTR_TYPE_SC_WRITE_FILE_ERR_WRITE_ERRNO;
      return (FALSE);
    }
    
    assert((size_t)bytes <= len);
    
    vstr_del(base, pos, (size_t)bytes);
    len -= (size_t)bytes;
  }

  return (TRUE);
}

int vstr_sc_write_file(Vstr_base *base, size_t pos, size_t len,
                       const char *filename, int open_flags, int mode,
                       unsigned int *err)
{
  int fd = -1;
  int dummy_err;
  int ret = 0;
  
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

  /* EGAIN shouldn't happen, so we're ok */
  ret = vstr_sc_write_fd(base, pos, len, fd, err);

  if ((close(fd) == -1) && !*err)
    *err = VSTR_TYPE_SC_WRITE_FILE_ERR_CLOSE_ERRNO;
  
  return (ret);
}
