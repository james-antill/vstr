/* This is a _simple_ cat program, no command line options. And no mmap
 * (see ex_nl.c or ex_hexdump.c for an example or using mmap() if you can.
 * Reads from stdin if no args are given.
 *
 * This shows how to use the Vstr library at it's simpelest,
 * for easy and fast IO.
 *
 * This file is more commented than normal code, so as to make it easy to follow
 * while knowing almost nothing about Vstr or Linux IO programming.
 */

#define VSTR_COMPILE_INCLUDE 1 /* make Vstr include it's own system headers */
#include <vstr.h>

#include <sys/types.h> /* stat + open */
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>


#define MAX_R_DATA_INCORE (1024 * 1024)
/* #define MAX_W_DATA_INCORE (1024 * 8) */


/* Creates a Vstr string, prints an error message to it and dies
 * don't really need to bother free'ing things as it's only called an a
 * major error -- but it's a good habit */
static void DIE(const char *msg, ...)
{
  Vstr_base *err = vstr_make_base(NULL);

  if (err)
  {
    va_list ap;
    
    va_start(ap, msg);
    vstr_add_vsysfmt(err, 0, msg, ap);
    va_end(ap);

    while (err->len)
      if (!vstr_sc_write_fd(err, 1, err->len, 1, NULL))
      {
        if ((errno != EAGAIN) && (errno != EINTR))
          break; /* don't recurse */
      }
  }
  vstr_free_base(err);

  exit (EXIT_FAILURE);
}




/*  Keep reading on the file descriptor until there is no more data (ERR_EOF)
 * abort if there is an error reading or writing */
static void ex_cat_read_fd_write_stdout(Vstr_base *s1, int fd)
{
  unsigned int err = 0;
  
  while (1)
  {
    if ((s1)->len < MAX_R_DATA_INCORE)
    {
      vstr_sc_read_iov_fd((s1), (s1)->len, (fd), 4, 32, &err);
      if (err == VSTR_TYPE_SC_READ_FD_ERR_EOF)
        break;
      else if ((err == VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO) &&
               ((errno == EINTR) || (errno == EAGAIN)))
        continue; /* try again ... this shouldn't happen as we are blocking */
      else if (err)
        DIE("read: %m\n");
    }

    if (!vstr_sc_write_fd(s1, 1, s1->len, 1, NULL))
    {
      if ((errno != EAGAIN) && (errno != EINTR))
        DIE("write: %m\n");
    }
  }
}

int main(int argc, char *argv[])
{ /* This is "cat", without any command line options */
  Vstr_conf *conf = NULL;
  Vstr_base *s1 = NULL;
  int count = 1; /* skip the program name */
  struct stat stat_buf;

  /* init the Vstr string library, note that if this fails we can't call DIE
   * or the program will crash */
  if (!vstr_init())
    exit (EXIT_FAILURE);


  /*  create a custom Vstr configuration, and alter the node buffer size to be
   * whatever the stdout block size is */

  if (fstat(1, &stat_buf) == -1)
    DIE("fstat(STDOUT): %m\n");
  
  if (!(conf = vstr_make_conf()))
    errno = ENOMEM, DIE("vstr_make_conf: %m\n");
  
  if (!stat_buf.st_blksize)
    stat_buf.st_blksize = 4096;
  
  vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);

  
  /* create a Vstr string for doing the IO with */  
  s1 = vstr_make_base(conf);
  if (!s1)
    errno = ENOMEM, DIE("vstr_make_base: %m\n");

  
  /*  "free" the Vstr configuration ... it is still being used by the
   * Vstr string s1, this just removes our hold on it and it will automatically
   * get truely free'd when s1 is free'd later */
  vstr_free_conf(conf);


  /* if no arguments are given just do passthrough from stdin to stdout */
  if (count >= argc)
    ex_cat_read_fd_write_stdout(s1, 0);


  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    int fd = open(argv[count], O_RDONLY | O_LARGEFILE | O_NOCTTY);
    
    if (fd == -1)
      DIE("open(%s): %m\n", argv[count]);
    
    ex_cat_read_fd_write_stdout(s1, fd);
    
    close(fd);
    
    ++count;
  }


  /* loop until all remaining data is output */
  while (s1->len)
    if (!vstr_sc_write_fd(s1, 1, s1->len, 1, NULL))
    {
      if ((errno != EAGAIN) && (errno != EINTR))
        DIE("write: %m\n");
    }


  /* These next two calls are only really needed to make valgrind happy,
   * but that does help in that any memory leaks are easier to see.
   */
  
  /* free s1, this also free's out custom Vstr configuration */
  vstr_free_base(s1);
  
  /* "exit" Vstr, this free's all internal data and no library calls apart from
   * vstr_init() should be called after this.
   */
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
