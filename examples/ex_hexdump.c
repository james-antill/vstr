/* This is a fairly simple hexdump program, no command line options. mmap()
 * is used if it can be.
 * Reads from stdin if no args are given.
 *
 * This shows how to use the Vstr library for simple data convertion.
 *
 * This file is more commented than normal code, so as to make it easy to follow
 * while knowing almost nothing about Vstr or Linux IO programming.
 */

#define _GNU_SOURCE 1 /* make Linux behave like normal systems,
                       * and export extentions */

#define VSTR_COMPILE_INCLUDE 1 /* make Vstr include it's own system headers */
#include <vstr.h>

#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>


/* some utils shared among the more complex examples ... mainly for
 * die(), the read/write, fcntl() for NONBLOCK and poll() calls which are the
 * same in all the examples.
 */
#include "ex_utils.h"

/* hexdump in "readable" format ... note this is a bit more fleshed out than
 * some of the other examples mainly because I actually use it */

/* this is roughly equiv. to the second command...
% rpm -qf /usr/bin/hexdump
util-linux-2.11r-10
% hexdump -e '"%08_ax:"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"'
        -e '"  " 16 "%_p" "\n"'

 * ...except that it prints the address in big hex digits, and it doesn't take
 * you 30 minutes to remember how to type it out.
 *  It also probably acts differently in that seperate files aren't merged
 * into one output line
 */

/* how much in core memory to use for read and write operations */
#define MAX_R_DATA_INCORE (1024 * 1024)
#define MAX_W_DATA_INCORE (1024 * 8)

/* configure if we print high ASCII characters */
#define PRNT_HIGH_CHARS TRUE

#if 0
/* simple print of a number ... not used */
# define EX_HEXDUMP_X8(s1, num) \
  vstr_add_fmt(s1, (s1)->len, "%08X:", (num))
# define EX_HEXDUMP_X2X2(s1, num1, num2) \
  vstr_add_fmt(s1, (s1)->len, " %02X%02X", (num1), (num2))
# define EX_HEXDUMP_X2__(s1, num1) \
  vstr_add_fmt(s1, (s1)->len, " %02X  ",   (num1))
#else
/* sick but fast print of a number ... used because I actually use this program
 */
# define EX_HEXDUMP_X8(s1, num) do { unsigned char xbuf[9]; \
  const char *digs = "0123456789ABCDEF"; \
  xbuf[8] = ':'; \
  xbuf[7] = digs[(((num) >>  0) & 0xf)]; \
  xbuf[6] = digs[(((num) >>  4) & 0xf)]; \
  xbuf[5] = digs[(((num) >>  8) & 0xf)]; \
  xbuf[4] = digs[(((num) >> 12) & 0xf)]; \
  xbuf[3] = digs[(((num) >> 16) & 0xf)]; \
  xbuf[2] = digs[(((num) >> 20) & 0xf)]; \
  xbuf[1] = digs[(((num) >> 24) & 0xf)]; \
  xbuf[0] = digs[(((num) >> 28) & 0xf)]; \
  vstr_add_buf(s1, (s1)->len, xbuf, 9); } while (FALSE)
# define EX_HEXDUMP_X2X2(s1, num1, num2) do { unsigned char xbuf[5]; \
  const char *digs = "0123456789ABCDEF"; \
  xbuf[0] = ' '; \
  xbuf[4] = digs[(((num2) >> 0) & 0xf)]; \
  xbuf[3] = digs[(((num2) >> 4) & 0xf)]; \
  xbuf[2] = digs[(((num1) >> 0) & 0xf)]; \
  xbuf[1] = digs[(((num1) >> 4) & 0xf)]; \
  vstr_add_buf(s1, (s1)->len, xbuf, 5); } while (FALSE)
# define EX_HEXDUMP_X2__(s1, num1) do { unsigned char xbuf[5]; \
  const char *digs = "0123456789ABCDEF"; \
  xbuf[4] = ' '; \
  xbuf[3] = ' '; \
  xbuf[2] = digs[(((num1) >> 0) & 0xf)]; \
  xbuf[1] = digs[(((num1) >> 4) & 0xf)]; \
  xbuf[0] = ' '; \
  vstr_add_buf(s1, (s1)->len, xbuf, 5); } while (FALSE)
#endif


static void ex_hexdump_process(Vstr_base *s1, Vstr_base *s2, int last)
{
  static unsigned int addr = 0;
  unsigned int flags = VSTR_FLAG04(CONV_UNPRINTABLE_ALLOW,
		                   COMMA, DOT, _, SP); /* normal ASCII chars */
  unsigned int flags_hsp = VSTR_FLAG06(CONV_UNPRINTABLE_ALLOW, /* high ascii too */
                                       COMMA, DOT, _, SP, HSP, HIGH);

  if (PRNT_HIGH_CHARS) /* config option */
    flags = flags_hsp;

  /* note that we don't want to create more data, if we are over our limit */
  if (s1->len > MAX_W_DATA_INCORE)
    return;
  
  /* while we have a hexdump line ... */
  while (s2->len >= 16)
  {
    unsigned char buf[16];

    /* get a hexdump line from the vstr */
    vstr_export_buf(s2, 1, 16, buf, sizeof(buf));

    /* write out a hexdump line address */
    EX_HEXDUMP_X8(s1, addr);

    /* write out hex values */
    EX_HEXDUMP_X2X2(s1, buf[ 0], buf[ 1]);
    EX_HEXDUMP_X2X2(s1, buf[ 2], buf[ 3]);
    EX_HEXDUMP_X2X2(s1, buf[ 4], buf[ 5]);
    EX_HEXDUMP_X2X2(s1, buf[ 6], buf[ 7]);
    EX_HEXDUMP_X2X2(s1, buf[ 8], buf[ 9]);
    EX_HEXDUMP_X2X2(s1, buf[10], buf[11]);
    EX_HEXDUMP_X2X2(s1, buf[12], buf[13]);
    EX_HEXDUMP_X2X2(s1, buf[14], buf[15]);
    
    VSTR_ADD_CSTR_BUF(s1, s1->len, "  ");

    /* write out characters */
    vstr_conv_unprintable_chr(s2, 1, 16, flags, '.');
    vstr_add_vstr(s1, s1->len, s2, 1, 16, VSTR_TYPE_ADD_ALL_BUF);
    vstr_add_rep_chr(s1, s1->len, '\n', 1);

    addr += 16;
    vstr_del(s2, 1, 16); /* delete the line just processed */

    /* note that we don't want to create data indefinitely, so stop
     * according to in core configuration */
    if (s1->len > MAX_W_DATA_INCORE)
      return;
  }

  if (last && s2->len)
  { /* do the same as above, but print the partial line for
     * the end of files */
    unsigned char buf[16];
    size_t got = s2->len;
    size_t missing = 16 - s2->len;
    const char *ptr = buf;

    missing -= (missing % 2);
    vstr_export_buf(s2, 1, s2->len, buf, sizeof(buf));

    EX_HEXDUMP_X8(s1, addr);
    
    while (got >= 2)
    {
      EX_HEXDUMP_X2X2(s1, ptr[0], ptr[1]);
      got -= 2;
      ptr += 2;
    }
    if (got)
    {
      EX_HEXDUMP_X2__(s1, ptr[0]);
      got -= 2;
    }

    /* easy way to add X amount of ' ' characters */
    vstr_add_rep_chr(s1, s1->len, ' ', (missing * 2) + (missing / 2) + 2);
    
    vstr_conv_unprintable_chr(s2, 1, s2->len, flags, '.');
    vstr_add_vstr(s1, s1->len, s2, 1, s2->len, VSTR_TYPE_ADD_ALL_BUF);

    VSTR_ADD_CSTR_BUF(s1, s1->len, "\n");

    addr += s2->len;
    vstr_del(s2, 1, s2->len);
  }

  /* if any of the above memory mgmt failed, error */
  if (s1->conf->malloc_bad)
    errno = ENOMEM, DIE("adding data:");
}

static void ex_hexdump_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2,
                                            int fd)
{
  unsigned int err = 0;
  int keep_going = TRUE;

  /* read/process/write loop */
  while (keep_going)
  {
    EX_UTILS_LIMBLOCK_READ_ALL(s2, fd, keep_going);
    
    ex_hexdump_process(s1, s2, !keep_going);

    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);
  }
}


int main(int argc, char *argv[])
{ /* This is "hexdump", as it should be by default */
  Vstr_base *s1 = NULL;
  Vstr_base *s2 = NULL;
  int count = 1; /* skip the program name */
  struct stat stat_buf;
  
  /* init the Vstr string library, note that if this fails we can't call DIE
   * or the program will crash */
  
  if (!vstr_init())
    errno = ENOMEM, DIE("vstr_init:");
  
  /*  Change the default Vstr configuration, so we can have a node buffer size
   * that is whatever the stdout block size is */
  if (fstat(1, &stat_buf) == -1)
    stat_buf.st_blksize = 4 * 1024;
  if (!stat_buf.st_blksize)
    stat_buf.st_blksize = 4 * 1024; /* defualt 4k -- proc etc. */
  
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);
  vstr_make_spare_nodes(NULL, VSTR_TYPE_NODE_BUF, 32);
  
  /* create two Vstr strings for doing the IO with */  
  s1 = vstr_make_base(NULL);
  if (!s1)
    errno = ENOMEM, DIE("vstr_make_base:");
  
  s2 = vstr_make_base(NULL);
  if (!s2)
    errno = ENOMEM, DIE("vstr_make_base:");

  /* set output to non-blocking mode */
  ex_utils_set_o_nonblock(1);
  
  if (count >= argc)  /* if no arguments -- use stdin */
  {
    /* set input to non-blocking mode */
    ex_utils_set_o_nonblock(0);
    ex_hexdump_read_fd_write_stdout(s1, s2, 0);
  }
  
  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    unsigned int err = 0;

    /* try to mmap the file, as that is faster ... */
    if (s2->len < MAX_R_DATA_INCORE)
      vstr_sc_mmap_file(s2, s2->len, argv[count], 0, 0, &err);

    if ((err == VSTR_TYPE_SC_MMAP_FILE_ERR_MMAP_ERRNO) ||
        (err == VSTR_TYPE_SC_MMAP_FILE_ERR_TOO_LARGE))
    {
      int fd = open(argv[count], O_RDONLY | O_LARGEFILE | O_NOCTTY);
      
      if (fd == -1)
        WARN("open(%s):", argv[count]);

      /* if mmap didn't work ... set file to nonblocking mode and do a
       * read/alter/write loop */
      ex_utils_set_o_nonblock(fd);
      ex_hexdump_read_fd_write_stdout(s1, s2, fd);
      
      close(fd);
    }
    else if (err && (err != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
      DIE("add:");
    else
      /* mmap worked so processes the entire file at once */
      ex_hexdump_process(s1, s2, FALSE);

    /* write some of what we've created */
    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);
    
    ++count;
  }

  while (s2->len)
  { /* No more data to read ...
     * finish processing read data and writing some of it */
    ex_hexdump_process(s1, s2, TRUE);

    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);
  }
  
  /* The vstr_free_base() and vstr_exit() calls are only really needed to
   * make memory checkers happy.
   */
  
  vstr_free_base(s2);

  while (s1->len)
    /* finish outputting processed data */
    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);
  
  vstr_free_base(s1);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
