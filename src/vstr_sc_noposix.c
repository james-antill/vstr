#define VSTR_SC_NOPOSIX_C
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
/* STUBS for - functions which are POSIX shortcuts */
#include "main.h"

int vstr_nx_sc_add_fd(Vstr_base *base, size_t pos, int fd,
                      off_t off, size_t len,
                      unsigned int *err)
{
  if (err)
  {
    errno = ENOSYS;
    *err = VSTR_TYPE_SC_ADD_FD_ERR_MMAP_ERRNO;
  }
  
  return (FALSE);
}

int vstr_nx_sc_add_file(Vstr_base *base, size_t pos, const char *filename,
                        unsigned int *err)
{
  if (err)
  {
    errno = ENOSYS;
    *err = VSTR_TYPE_SC_ADD_FD_ERR_MMAP_ERRNO;
  }
  
  return (FALSE);
}

int vstr_nx_sc_read_fd(Vstr_base *base, size_t pos, int fd,
                       unsigned int min, unsigned int max,
                       unsigned int *err)
{
  if (err)
  {
    errno = ENOSYS;
    *err = VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO;
  }
  
  return (FALSE);
}

int vstr_nx_sc_write_fd(Vstr_base *base, size_t pos, size_t len, int fd,
                        unsigned int *err)
{
  if (err)
  {
    errno = ENOSYS;
    *err = VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO;
  }
  
  return (FALSE);
}

int vstr_nx_sc_write_file(Vstr_base *base, size_t pos, size_t len,
                          const char *filename, int open_flags, int mode,
                          unsigned int *err)
{
  if (err)
  {
    errno = ENOSYS;
    *err = VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO;
  }
  
  return (FALSE);
}
