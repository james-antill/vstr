#define _GNU_SOURCE
/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "ex_utils.h"

#define SECTS_LOOP 32 /* how many sections to split into per loop */

#if 1 /* this version using split isn't any slower */
static void ex_nl_process(Vstr_base *str1, Vstr_base *str2,
                          int last __attribute__((unused)))
{
  static unsigned int count = 0;
  
  while (str2->len)
  {
    const int flags = (VSTR_FLAG_SPLIT_REMAIN |
                       VSTR_FLAG_SPLIT_BEG_NULL |
                       VSTR_FLAG_SPLIT_MID_NULL |
                       VSTR_FLAG_SPLIT_END_NULL |
                       VSTR_FLAG_SPLIT_NO_RET);
    VSTR_SECTS_DECL(sects, SECTS_LOOP);
    unsigned int num = 0;

    VSTR_SECTS_DECL_INIT(sects);
    vstr_split_buf(str2, 1, str2->len, "\n", 1, sects, sects->sz, flags);
    
    while ((++num < SECTS_LOOP) && (num <= sects->num))
    {
      size_t split_pos = VSTR_SECTS_NUM(sects, num)->pos;
      size_t split_len = VSTR_SECTS_NUM(sects, num)->len;

      vstr_add_fmt(str1, str1->len, "% 6d\t", ++count);
      if (split_len)
        vstr_add_vstr(str1, str1->len, str2, split_pos, split_len,
                      VSTR_TYPE_ADD_ALL_BUF);
      VSTR_ADD_CSTR_BUF(str1, str1->len, "\n");

      if (str1->conf->malloc_bad)
        errno = ENOMEM, DIE("adding data:");

      while (str1->len > (1024 * 16))
        if (!ex_utils_write(str1, 1))
          DIE("write:");
    }

    if (sects->num != sects->sz)
    {
      vstr_del(str2, 1, str2->len);
      return;
    }
    else
    {
      size_t pos = VSTR_SECTS_NUM(sects, sects->num)->pos;
      vstr_del(str2, 1, pos - 1);
    }
  }
}
#else
static void ex_nl_process(Vstr_base *str1, Vstr_base *str2, int last)
{
  static unsigned int count = 0;
  size_t pos = 0;

  if (!str2->len)
    return;
  
  while ((pos = vstr_srch_chr_fwd(str2, 1, str2->len, '\n')))
  {
    vstr_add_fmt(str1, str1->len, "% 6d\t", ++count);
    vstr_add_vstr(str1, str1->len, str2, 1, pos,
                  VSTR_TYPE_ADD_ALL_BUF);
    vstr_del(str2, 1, pos);
  }

  if (last)
  {
    vstr_add_fmt(str1, str1->len, "% 6d\t", ++count);
    if (str2->len)
      vstr_add_vstr(str1, str1->len, str2, 1, str2->len,
                    VSTR_TYPE_ADD_ALL_BUF);
    vstr_add_buf(str1, str1->len, "\n", 1);    
  }
}

#endif


static void ex_nl_read_fd_write_stdout(Vstr_base *str1, Vstr_base *str2, int fd)
{
  unsigned int err = 0;
  
  while (TRUE)
  {
    vstr_sc_read_iov_fd(str2, str2->len, fd, 2, 32, &err);
    if (err == VSTR_TYPE_SC_READ_FD_ERR_EOF)
      break;
    if (err)
      DIE("read:");
    
    ex_nl_process(str1, str2, FALSE);
    
    if (!vstr_sc_write_fd(str1, 1, str1->len, 1, &err))
    { /* should really use socket_poll ...
       * but can't be bothered requireing other library */
      struct pollfd one;
      
      if (errno != EAGAIN)
        DIE("write:");

      one.fd = fd;
      one.events = POLLOUT;
      one.revents = 0;

      while (poll(&one, 1, -1) == -1) /* can't timeout */
      {
        if (errno != EINTR)
          DIE("poll:");
      }
    }
  }
}

int main(int argc, char *argv[])
{
  Vstr_conf *conf = NULL;
  Vstr_base *str1 = NULL;
  Vstr_base *str2 = NULL;
  int count = 1;
  struct stat stat_buf;

  if (!vstr_init())
      exit (EXIT_FAILURE);

  if (fstat(1, &stat_buf) == -1)
    DIE("fstat:");

  if (!(conf = vstr_make_conf()))
    errno = ENOMEM, DIE("vstr_make_conf:");

  vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);
  
  str1 = vstr_make_base(conf);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");

  str2 = vstr_make_base(conf);
  if (!str2)
    errno = ENOMEM, DIE("vstr_make_base:");

  vstr_free_conf(conf);
  
  if (count == argc)  /* use stdin */
   ex_nl_read_fd_write_stdout(str1, str2, 0);

  while (count < argc)
  {
    unsigned int err = 0;
    
    vstr_sc_mmap_file(str2, str2->len, argv[count], 0, 0, &err);
    if ((err == VSTR_TYPE_SC_MMAP_FILE_ERR_MMAP_ERRNO) ||
        (err == VSTR_TYPE_SC_MMAP_FILE_ERR_TOO_LARGE))
    {
      int fd = open(argv[count], O_RDONLY | O_LARGEFILE | O_NOCTTY);
      
      if (fd == -1)
        DIE("open:");

      ex_nl_read_fd_write_stdout(str1, str2, fd);
      
      close(fd);
    }
    else if (err && (err != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
      DIE("add:");
    else
      ex_nl_process(str1, str2, FALSE);
    
    if (!vstr_sc_write_fd(str1, 1, str1->len, 1, NULL))
      DIE("write:");
    ++count;
  }

  ex_nl_process(str1, str2, TRUE);
  
  vstr_free_base(str2);

  while (str1->len)
    if (!vstr_sc_write_fd(str1, 1, str1->len, 1, NULL))
      DIE("write:");
  
  vstr_free_base(str1);

  ex_utils_check();
 
  vstr_exit();

  exit (EXIT_SUCCESS);
}
