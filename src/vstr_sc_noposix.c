#define VSTR_SC_NOPOSIX_C
/*
 *  Copyright (C) 2002, 2003  James Antill
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

# define VSTR__SC_ENOSYS(x) \
  if (err) \
  { \
    errno = ENOSYS; \
    *err = (x); \
  } \
  \
  return (FALSE)

int vstr_sc_mmap_fd(Vstr_base *base __attribute__((unused)),
                    size_t pos __attribute__((unused)),
                    int fd __attribute__((unused)),
                    VSTR_AUTOCONF_off64_t off __attribute__((unused)),
                    size_t len __attribute__((unused)),
                    unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_MMAP_FD_ERR_MMAP_ERRNO); }

int vstr_sc_mmap_file(Vstr_base *base __attribute__((unused)),
                      size_t pos __attribute__((unused)),
                      const char *filename __attribute__((unused)),
                      VSTR_AUTOCONF_off64_t off __attribute__((unused)),
                      size_t len __attribute__((unused)),
                      unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_MMAP_FD_ERR_MMAP_ERRNO); }

int vstr_sc_read_iov_fd(Vstr_base *base __attribute__((unused)),
                        size_t pos __attribute__((unused)),
                        int fd __attribute__((unused)),
                        unsigned int min __attribute__((unused)),
                        unsigned int max __attribute__((unused)),
                        unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO); }

int vstr_sc_read_len_fd(Vstr_base *base __attribute__((unused)),
                        size_t pos __attribute__((unused)),
                        int fd __attribute__((unused)),
                        size_t len __attribute__((unused)),
                        unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO); }

int vstr_sc_read_iov_file(Vstr_base *base __attribute__((unused)),
                          size_t pos __attribute__((unused)),
                          const char *filename __attribute__((unused)),
                          VSTR_AUTOCONF_off64_t off __attribute__((unused)),
                          unsigned int min __attribute__((unused)),
                          unsigned int max __attribute__((unused)),
                          unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_READ_FILE_ERR_READ_ERRNO); }

int vstr_sc_read_len_file(Vstr_base *base __attribute__((unused)),
                          size_t pos __attribute__((unused)),
                          const char *filename __attribute__((unused)),
                          VSTR_AUTOCONF_off64_t off __attribute__((unused)),
                          size_t len __attribute__((unused)),
                          unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_READ_FILE_ERR_READ_ERRNO); }

int vstr_sc_write_fd(Vstr_base *base __attribute__((unused)),
                     size_t pos __attribute__((unused)),
                     size_t len __attribute__((unused)),
                     int fd __attribute__((unused)),
                     unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO); }

int vstr_sc_write_file(Vstr_base *base __attribute__((unused)),
                       size_t pos __attribute__((unused)),
                       size_t len __attribute__((unused)),
                       const char *filename __attribute__((unused)),
                       int open_flags __attribute__((unused)),
                       VSTR_AUTOCONF_mode_t mode __attribute__((unused)),
                       VSTR_AUTOCONF_off64_t off __attribute__((unused)),
                       unsigned int *err)
{ VSTR__SC_ENOSYS(VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO); }

int vstr_sc_fmt_add_ipv4_ptr(Vstr_conf *conf __attribute__((unused)),
                             const char *name __attribute__((unused)))
{
  return (FALSE);
}

int vstr_sc_fmt_add_ipv6_ptr(Vstr_conf *conf __attribute__((unused)),
                             const char *name __attribute__((unused)))
{
  return (FALSE);
}
