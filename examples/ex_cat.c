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

#define MAX_R_DATA_INCORE (1024 * 1024)
#define MAX_W_DATA_INCORE (1024 * 8)

static void ex_cat_read_fd_write_stdout(Vstr_base *str1, int fd)
{
  unsigned int err = 0;
  int keep_going = TRUE;
  
  while (keep_going)
  {
    EX_UTILS_LIMBLOCK_READ_ALL(str1, fd, keep_going);
    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
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

 vstr_free_conf(conf);
 
 if (count == argc)  /* use stdin */
 {
   ex_utils_set_o_nonblock(0);
   ex_cat_read_fd_write_stdout(str1, 0);
 }
 
 while (count < argc)
 {
   unsigned int err = 0;

   if (str1->len < MAX_R_DATA_INCORE)
     vstr_sc_mmap_file(str1, str1->len, argv[count], 0, 0, &err);
   
   if ((err == VSTR_TYPE_SC_MMAP_FD_ERR_MMAP_ERRNO) ||
       (err == VSTR_TYPE_SC_MMAP_FD_ERR_TOO_LARGE))
   {
     int fd = open(argv[count], O_RDONLY | O_LARGEFILE | O_NOCTTY);

     if (fd == -1)
       DIE("open:");

     ex_utils_set_o_nonblock(fd);
     ex_cat_read_fd_write_stdout(str1, fd);
     
     close(fd);
   }
   else if (err && (err != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
     DIE("add:");

   EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
   
  ++count;
 }
 
 while (str1->len)
   EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);

 vstr_free_base(str1);
 
 vstr_exit();
 
 exit (EXIT_SUCCESS);
}
