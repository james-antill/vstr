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

#define PRNT_NONE 0
#define PRNT_SPAC 1
#define PRNT_HIGH 2

/* configure what ASCII characters we print */
static unsigned int prnt_high_chars = PRNT_NONE;

#if 0
/* simple print of a number ... not used */
# define EX_HEXDUMP_X8(s1, num) \
  vstr_add_fmt(s1, (s1)->len, "0x%08X:", (num))
# define EX_HEXDUMP_X2X2(s1, num1, num2) \
  vstr_add_fmt(s1, (s1)->len, " %02X%02X", (num1), (num2))
# define EX_HEXDUMP_X2__(s1, num1) \
  vstr_add_fmt(s1, (s1)->len, " %02X  ",   (num1))
#else
/* fast print of a number ... used because I actually use this program
 */
static const char *hexnums = "0123456789ABCDEF";

# define EX_HEXDUMP_BYTE(buf, b) do { \
  (buf)[1] = hexnums[((b) >> 0) & 0xf]; \
  (buf)[0] = hexnums[((b) >> 4) & 0xf]; \
 } while (FALSE)

# define EX_HEXDUMP_UINT(buf, i) do { \
  EX_HEXDUMP_BYTE((buf) + 6, (i) >>  0); \
  EX_HEXDUMP_BYTE((buf) + 4, (i) >>  8); \
  EX_HEXDUMP_BYTE((buf) + 2, (i) >> 16); \
  EX_HEXDUMP_BYTE((buf) + 0, (i) >> 24); \
 } while (FALSE)

# define EX_HEXDUMP_X8(s1, num) do { unsigned char xbuf[9]; \
  xbuf[8] = ':'; \
  EX_HEXDUMP_UINT(xbuf, num); \
  vstr_add_buf(s1, (s1)->len, xbuf, 9); } while (FALSE)
# define EX_HEXDUMP_X2X2(s1, num1, num2) do { unsigned char xbuf[5]; \
  xbuf[0] = ' '; \
  EX_HEXDUMP_BYTE(xbuf + 3, num2); \
  EX_HEXDUMP_BYTE(xbuf + 1, num1); \
  vstr_add_buf(s1, (s1)->len, xbuf, 5); } while (FALSE)
# define EX_HEXDUMP_X2__(s1, num1) do { unsigned char xbuf[5]; \
  xbuf[4] = ' '; \
  xbuf[3] = ' '; \
  EX_HEXDUMP_BYTE(xbuf + 1, num1); \
  xbuf[0] = ' '; \
  vstr_add_buf(s1, (s1)->len, xbuf, 5); } while (FALSE)
#endif


static int ex_hexdump_process(Vstr_base *s1, Vstr_base *s2, int last)
{
  static unsigned int addr = 0;
  /* normal ASCII chars */
  unsigned int flags = VSTR_FLAG02(CONV_UNPRINTABLE_ALLOW, COMMA, DOT);
  /* allow spaces */
  unsigned int flags_sp = VSTR_FLAG04(CONV_UNPRINTABLE_ALLOW,
                                      COMMA, DOT, _, SP);
  /* high ascii too */
  unsigned int flags_hsp = VSTR_FLAG06(CONV_UNPRINTABLE_ALLOW,
                                       COMMA, DOT, _, SP, HSP, HIGH);

  switch (prnt_high_chars)
  {
    case PRNT_HIGH: flags = flags_hsp; break;
    case PRNT_SPAC: flags = flags_sp;  break;
    case PRNT_NONE:                    break;
  }
  
  /* note that we don't want to create more data, if we are over our limit */
  if (s1->len > MAX_W_DATA_INCORE)
    return (FALSE);
  
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
      return (TRUE);
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

    return (TRUE);
  }

  /* if any of the above memory mgmt failed, error */
  if (s1->conf->malloc_bad)
    errno = ENOMEM, DIE("adding data:");

  return (FALSE);
}

static void ex_hexdump_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2,
                                            int fd)
{
  unsigned int err = 0;
  int keep_going = TRUE;

  /* read/process/write loop */
  while (keep_going)
  {
    int proc_data = FALSE;
    
    EX_UTILS_LIMBLOCK_READ_ALL(s2, fd, keep_going);
    
    proc_data = ex_hexdump_process(s1, s2, !keep_going);

    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, STDOUT_FILENO);

    EX_UTILS_LIMBLOCK_WAIT(s1, s2, fd, 16, keep_going, proc_data);
  }
}


int main(int argc, char *argv[])
{ /* This is "hexdump", as it should be by default */
  Vstr_base *s1 = NULL;
  Vstr_base *s2 = NULL;
  int count = 1; /* skip the program name */
  struct stat stat_buf;
  unsigned int use_mmap = FALSE;
  
  /* init the Vstr string library, note that if this fails we can't call DIE
   * or the program will crash */
  
  if (!vstr_init())
    exit (EXIT_FAILURE);
  
  /*  Change the default Vstr configuration, so we can have a node buffer size
   * that is whatever the stdout block size is */
  if (fstat(1, &stat_buf) == -1)
    stat_buf.st_blksize = 4 * 1024;
  if (!stat_buf.st_blksize)
    stat_buf.st_blksize = 4 * 1024; /* defualt 4k -- proc etc. */
  
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);
  vstr_make_spare_nodes(NULL, VSTR_TYPE_NODE_BUF, 32);
  
  /* create two Vstr strings for doing the IO with */
  if (FALSE ||
      !(s1 = vstr_make_base(NULL)) ||
      !(s2 = vstr_make_base(NULL)) ||
      FALSE)
    errno = ENOMEM, DIE("vstr_make_base:");

  /* set output to non-blocking mode */
  ex_utils_set_o_nonblock(STDOUT_FILENO);
  
  while (count < argc)
  { /* quick hack getopt_long */
    if (!strcmp("--", argv[count]))
    {
      ++count;
      break;
    }
    else if (!strcmp("--mmap", argv[count]))
      use_mmap = !use_mmap;
    else if (!strcmp("--none", argv[count]))
      prnt_high_chars = PRNT_NONE;
    else if (!strcmp("--space", argv[count]))
      prnt_high_chars = PRNT_SPAC;
    else if (!strcmp("--high", argv[count]))
      prnt_high_chars = PRNT_HIGH;
    else if (!strcmp("--version", argv[count]))
    {
      vstr_add_fmt(s1, 0, "%s", "\
jhexdump 1.0.0\n\
Written by James Antill\n\
\n\
Uses Vstr string library.\n\
");
      goto out;
    }
    else if (!strcmp("--help", argv[count]))
    {
      vstr_add_fmt(s1, 0, "%s", "\
Usage: jhexdump [STRING]...\n\
   or: jhexdump OPTION\n\
Repeatedly output a line with all specified STRING(s), or `y'.\n\
\n\
      --help     Display this help and exit\n\
      --version  Output version information and exit\n\
      --space    Allow space characters in ASCII output\n\
      --high     Allow space and high characters in ASCII output\n\
      --none     Allow only small amount of characters ASCII output\n\
      --mmap     Toggle use of mmap() on files\n\
      --         Use rest of cmd line input regardless of if it's an option\n\
\n\
Report bugs to James Antill <james@and.org>.\n\
");
      goto out;
    }
    else
      break;
    ++count;
  }

  if (count >= argc)  /* if no arguments -- use stdin */
  {
    /* set input to non-blocking mode */
    ex_utils_set_o_nonblock(STDIN_FILENO);
    ex_hexdump_read_fd_write_stdout(s1, s2, STDIN_FILENO);
  }
  
  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    unsigned int err = 0;
    
    /* try to mmap the file, as that is faster ... */
    if (use_mmap && (s2->len < MAX_R_DATA_INCORE))
      vstr_sc_mmap_file(s2, s2->len, argv[count], 0, 0, &err);

    if (!use_mmap ||
        (err == VSTR_TYPE_SC_MMAP_FILE_ERR_FSTAT_ERRNO) ||
        (err == VSTR_TYPE_SC_MMAP_FILE_ERR_MMAP_ERRNO) ||
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
      ex_hexdump_process(s1, s2, TRUE);

    /* write some of what we've created */
    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);
    
    ++count;
  }

  while (s2->len)
  { /* No more data to read ...
     * finish processing read data and writing some of it */
    int proc_data = ex_hexdump_process(s1, s2, TRUE);

    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, STDOUT_FILENO);
    EX_UTILS_LIMBLOCK_WAIT(s1, s2, -1, 16, s2->len, proc_data);
  }
  
  /* The vstr_free_base() and vstr_exit() calls are only really needed to
   * make memory checkers happy.
   */
  
 out:
  vstr_free_base(s2);

  while (s1->len)
  { /* finish outputting processed data */
    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);
    EX_UTILS_LIMBLOCK_WAIT(s1, NULL, -1, 16, FALSE, FALSE);
  }
  
  vstr_free_base(s1);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
