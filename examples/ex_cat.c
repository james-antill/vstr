/* This is a _simple_ cat program, no command line options. And no use of mmap.
 * Reads from stdin if no args are given.
 *
 * This shows how to use the Vstr library at it's simpelest,
 * for easy and fast IO. Note however that all needed error detection is
 * included.
 *
 * This file is more commented than normal code, so as to make it easy to follow
 * while knowing almost nothing about Vstr or Linux IO programming.
 */
#include "ex_utils.h"

/*  Keep reading on the file descriptor until there is no more data (ERR_EOF)
 * abort if there is an error reading or writing */
static void ex_cat_read_fd_write_stdout(Vstr_base *s1, int fd)
{
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s1, fd);

    if (io_r_state == IO_EOF)
      break;
    
    io_w_state = io_put(s1, 1);

    io_limit(io_r_state, fd, io_w_state, 1, s1);    
  }
}

int main(int argc, char *argv[])
{ /* This is "cat", without any command line options */
  
  Vstr_base *s1 = ex_init(NULL); /* init the library etc. */
  int count = 1; /* skip the program name */

  /* if no arguments are given just do stdin to stdout */
  if (count >= argc)
  {
    io_fd_set_o_nonblock(STDIN_FILENO);
    ex_cat_read_fd_write_stdout(s1, STDIN_FILENO);
  }

  
  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    int fd = io_open(argv[count]);

    ex_cat_read_fd_write_stdout(s1, fd);

    if (close(fd) == -1)
      warn("close(%s)", argv[count]);

    ++count;
  }

  /* output all remaining data */
  io_put_all(s1, STDOUT_FILENO);

  exit (ex_exit(s1, NULL));
}
