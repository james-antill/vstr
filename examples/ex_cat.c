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
#include <stdint.h>
#include <sys/poll.h>

#include <vstr.h>

#include "ex_utils.h"

static void ex_cat_read_fd_write_stdout(Vstr_base *str1, int fd)
{
  unsigned int err = 0;
  
  while (TRUE)
  {
    vstr_sc_read_fd(str1, str1->len, fd, 2, 32, &err);
    if (err == VSTR_TYPE_SC_READ_FD_ERR_EOF)
      break;
    
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
{ /* This is "cat", without any command line options */
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
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

 if (count == argc)  /* use stdin */
   ex_cat_read_fd_write_stdout(str1, 0);

 while (count < argc)
 {
   unsigned int err = 0;

   vstr_sc_add_file(str1, str1->len, argv[count], &err);
   if ((err == VSTR_TYPE_SC_ADD_FD_ERR_MMAP_ERRNO) ||
       (err == VSTR_TYPE_SC_ADD_FD_ERR_TOO_LARGE))
   {
     int fd = open(argv[count], O_RDONLY | O_LARGEFILE | O_NOCTTY);

     if (fd == -1)
       DIE("open:");

     ex_cat_read_fd_write_stdout(str1, fd);
     
     close(fd);
   }
   else if (err != VSTR_TYPE_SC_ADD_FILE_ERR_CLOSE_ERRNO)
     DIE("add:");

   if (!vstr_sc_write_fd(str1, 1, str1->len, 1, NULL))
     DIE("write:");
   
  ++count;
 }
 
 while (str1->len)
   if (!vstr_sc_write_fd(str1, 1, str1->len, 1, NULL))
     DIE("write:");
 
 exit (EXIT_SUCCESS);
}
