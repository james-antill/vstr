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

#include <vstr.h>

#include "ex_utils.h"

int main(int argc, char *argv[])
{ /* This is "cat", without any command line options */
 Vstr_conf *conf = NULL;
 Vstr_base *str1 = NULL;
 int count = 1; /* skip the program name */
 
 if (!vstr_init())
   errno = ENOMEM, DIE("vstr_init:");
 
 if (!(conf = vstr_make_conf()))
   errno = ENOMEM, DIE("vstr_make_conf:");

 vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ,
                4096 - sizeof(Vstr_node_buf));

 str1 = vstr_make_base(conf);
 if (!str1)
   errno = ENOMEM, DIE("vstr_make_base:");

 if (count == argc)
 {/* use stdin */
  int fd = 0;
  
  while (ex_utils_read(str1, fd))
    ex_utils_write(str1, 1);
 }

 while (count < argc)
 {
   ex_utils_append_file(str1, argv[count], 1, 1024 * 16);
   ex_utils_write(str1, 1);
  
  ++count;
 }
 
 while (str1->len)
   ex_utils_write(str1, 1);
 
 ex_utils_check();
 
 exit (EXIT_SUCCESS);
}
