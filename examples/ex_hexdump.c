#define _GNU_SOURCE
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "ex_utils.h"

/* hexdump in "readable" format ... note this is a bit more fleshed out than
 * some of the other examples mainly because I actually use it */

#define MAX_R_DATA_INCORE (1024 * 1024)
#define MAX_W_DATA_INCORE (1024 * 8)

static void ex_hexdump_process(Vstr_base *str1, Vstr_base *str2, int last)
{
  static unsigned int addr = 0;

  while (str2->len >= 16)
  {
    unsigned char buf[16];
    
    vstr_export_buf(str2, 1, 16, buf, sizeof(buf));
    
    vstr_add_fmt(str1, str1->len, "%08X:", addr);
    vstr_add_fmt(str1, str1->len,
                 " %02X%02X %02X%02X"
                 " %02X%02X %02X%02X"
                 " %02X%02X %02X%02X"
                 " %02X%02X %02X%02X"
                 "  ",
                 buf[ 0], buf[ 1], buf[ 2], buf[ 3],
                 buf[ 4], buf[ 5], buf[ 6], buf[ 7],
                 buf[ 8], buf[ 9], buf[10], buf[11],
                 buf[12], buf[13], buf[14], buf[15]);
    
    vstr_conv_unprintable_chr(str2, 1, 16, VSTR_FLAG_CONV_UNPRINTABLE_DEF, '.');
    vstr_add_vstr(str1, str1->len, str2, 1, 16, VSTR_TYPE_ADD_ALL_BUF);
    VSTR_ADD_CSTR_BUF(str1, str1->len, "\n");
    vstr_del(str2, 1, 16);
    
    addr += 0x10;

    if (str1->len > MAX_W_DATA_INCORE)
      return;
  }

  if (last && str2->len)
  {
    unsigned char buf[16];
    size_t got = str2->len;
    size_t missing = 16 - str2->len;
    const char *ptr = buf;

    missing -= (missing % 2);
    vstr_export_buf(str2, 1, str2->len, buf, sizeof(buf));

    vstr_add_fmt(str1, str1->len, "%08X:", addr);
    
    while (got >= 2)
    {
      vstr_add_fmt(str1, str1->len, " %02X%02X", ptr[0], ptr[1]);
      got -= 2;
      ptr += 2;
    }
    if (got)
    {
      vstr_add_fmt(str1, str1->len, " %02X  ", ptr[0]);
      got -= 2;
    }
    
    vstr_add_fmt(str1, str1->len,
                 "%*s", (missing * 2) + (missing / 2) + 2, "");
    
    vstr_conv_unprintable_chr(str2, 1, str2->len,
                              VSTR_FLAG_CONV_UNPRINTABLE_DEF, '.');
    vstr_add_vstr(str1, str1->len, str2, 1, str2->len, VSTR_TYPE_ADD_ALL_BUF);

    VSTR_ADD_CSTR_BUF(str1, str1->len, "\n");

    vstr_del(str2, 1, 16);
  }

  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("adding data:");
}

int ex_hexdump_set_o_nonblock(int fd)
{
  int flags = 0;

  if ((flags = fcntl(fd, F_GETFL)) == -1)
    return (0);

  if (!(flags & O_NONBLOCK) &&
      (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1))
    return (0);

  return (1);
}

static void ex_hexdump_read_fd_write_stdout(Vstr_base *str1, Vstr_base *str2,
                                            int fd)
{
  unsigned int err = 0;
  
  while (TRUE)
  {
    if (str2->len < MAX_R_DATA_INCORE)
    {
      vstr_sc_read_fd(str2, str2->len, fd, 2, 32, &err);
      if (err == VSTR_TYPE_SC_READ_FD_ERR_EOF)
        break;
      if (err)
        DIE("read:");
    }
    
    ex_hexdump_process(str1, str2, FALSE);

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
{ /* This is "hexdump", as it should be by default */
  Vstr_conf *conf = NULL;
  Vstr_base *str1 = NULL;
  Vstr_base *str2 = NULL;
  int count = 1; /* skip the program name */
  struct stat stat_buf;
  
  if (!vstr_init())
    errno = ENOMEM, DIE("vstr_init:");
  
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

  ex_hexdump_set_o_nonblock(1);
  
  if (count == argc)  /* use stdin */
  {
    ex_hexdump_set_o_nonblock(0);
    ex_hexdump_read_fd_write_stdout(str1, str2, 0);
  }
  
  while (count < argc)
  {
    unsigned int err = 0;
    
    vstr_sc_add_file(str2, str2->len, argv[count], &err);
    if ((err == VSTR_TYPE_SC_ADD_FD_ERR_MMAP_ERRNO) ||
        (err == VSTR_TYPE_SC_ADD_FD_ERR_TOO_LARGE))
    {
      int fd = open(argv[count], O_RDONLY | O_LARGEFILE | O_NOCTTY);
      
      if (fd == -1)
        DIE("open:");

      ex_hexdump_read_fd_write_stdout(str1, str2, fd);
      
      close(fd);
    }
    else if (err && (err != VSTR_TYPE_SC_ADD_FILE_ERR_CLOSE_ERRNO))
      DIE("add:");
    else
      ex_hexdump_process(str1, str2, FALSE);

    if (!vstr_sc_write_fd(str1, 1, str1->len, 1, NULL))
    { /* should really use socket_poll ...
       * but can't be bothered requireing other library */
      struct pollfd one;
      
      if (errno != EAGAIN)
        DIE("write:");

      one.fd = 1;
      one.events = POLLOUT;
      one.revents = 0;

      while (poll(&one, 1, -1) == -1) /* can't timeout */
      {
        if (errno != EINTR)
          DIE("poll:");
      }
    }
    
    ++count;
  }

  while (str2->len)
  {
    ex_hexdump_process(str1, str2, TRUE);

    if (!vstr_sc_write_fd(str1, 1, str1->len, 1, NULL))
    { /* should really use socket_poll ...
       * but can't be bothered requireing other library */
      struct pollfd one;
      
      if (errno != EAGAIN)
        DIE("write:");

      one.fd = 1;
      one.events = POLLOUT;
      one.revents = 0;

      while (poll(&one, 1, -1) == -1) /* can't timeout */
      {
        if (errno != EINTR)
          DIE("poll:");
      }
    }
  }
  
  vstr_free_base(str2);

  while (str1->len)
    if (!vstr_sc_write_fd(str1, 1, str1->len, 1, NULL))
      DIE("write:");
  
  vstr_free_base(str1);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
