/* This is a fairly simple hexdump program, it has command line options to
 * enable printing of high latin symbols, and/or use mmap() to load the input
 * data.
 *
 * Reads from stdin if no args are given.
 *
 * This shows how to use the Vstr library for simple data convertion.
 *
 * This file is more commented than normal code, so as to make it easy to follow
 * while knowing almost nothing about Vstr or Linux IO programming.
 */

#include "ex_utils.h"

/* hexdump in "readable" format ... note this is a bit more fleshed out than
 * some of the other examples mainly because I actually use it */

/* this is roughly equiv. to the Linux hexdump command...
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
 *  It also acts differently in that seperate files aren't merged
 * into one output line (Ie. in this version each file starts on a new line,
 * however the addresses are continuious).
 */

#define PRNT_NONE 0
#define PRNT_SPAC 1
#define PRNT_HIGH 2


/* number of characters we output per line (assumes 80 char width screen)... */
#define EX_HEXDUMP_CHRS_PER_LINE 16

/* configure what ASCII characters we print */
static unsigned int prnt_high_chars = PRNT_NONE;

#if 0
/* simple print of a number */

/* print the address */
# define EX_HEXDUMP_X8(s1, num) \
  vstr_add_fmt(s1, (s1)->len, "0x%08X:", (num))
/* print a set of two bytes */
# define EX_HEXDUMP_X2X2(s1, num1, num2) \
  vstr_add_fmt(s1, (s1)->len, " %02X%02X", (num1), (num2))
/* print a byte and spaces for the missing byte */
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

/* print the address */
# define EX_HEXDUMP_X8(s1, num) do { unsigned char xbuf[9]; \
  xbuf[8] = ':'; \
  EX_HEXDUMP_UINT(xbuf, num); \
  vstr_add_buf(s1, (s1)->len, xbuf, sizeof(xbuf)); } while (FALSE)
/* print a set of two bytes */
# define EX_HEXDUMP_X2X2(s1, num1, num2) do { unsigned char xbuf[5]; \
  xbuf[0] = ' '; \
  EX_HEXDUMP_BYTE(xbuf + 3, num2); \
  EX_HEXDUMP_BYTE(xbuf + 1, num1); \
  vstr_add_buf(s1, (s1)->len, xbuf, sizeof(xbuf)); } while (FALSE)
/* print a byte and spaces for the missing byte */
# define EX_HEXDUMP_X2__(s1, num1) do { unsigned char xbuf[5]; \
  xbuf[4] = ' '; \
  xbuf[3] = ' '; \
  EX_HEXDUMP_BYTE(xbuf + 1, num1); \
  xbuf[0] = ' '; \
  vstr_add_buf(s1, (s1)->len, xbuf, sizeof(xbuf)); } while (FALSE)
#endif


static int ex_hexdump_process(Vstr_base *s1, Vstr_base *s2, int last)
{
  static unsigned int addr = 0;
  /* normal ASCII chars, just allow COMMA and DOT flags */
  unsigned int flags = VSTR_FLAG02(CONV_UNPRINTABLE_ALLOW, COMMA, DOT);
  /* allow spaces, allow COMMA, DOT, underbar _, and space */
  unsigned int flags_sp = VSTR_FLAG04(CONV_UNPRINTABLE_ALLOW,
                                      COMMA, DOT, _, SP);
  /* high ascii too, allow
   * COMMA, DOT, underbar _, space, high space and other high characters */
  unsigned int flags_hsp = VSTR_FLAG06(CONV_UNPRINTABLE_ALLOW,
                                       COMMA, DOT, _, SP, HSP, HIGH);
  unsigned char buf[EX_HEXDUMP_CHRS_PER_LINE];

  switch (prnt_high_chars)
  {
    case PRNT_HIGH: flags = flags_hsp; break;
    case PRNT_SPAC: flags = flags_sp;  break;
    case PRNT_NONE:                    break;
    default: ASSERT(FALSE);            break;
  }

  /* we don't want to create more data, if we are over our limit */
  if (s1->len > EX_MAX_W_DATA_INCORE)
    return (FALSE);

  /* while we have a hexdump line ... */
  while (s2->len >= EX_HEXDUMP_CHRS_PER_LINE)
  {
    unsigned int count = 0;
    
    /* get a hexdump line from the vstr */
    vstr_export_buf(s2, 1, EX_HEXDUMP_CHRS_PER_LINE, buf, sizeof(buf));

    /* write out a hexdump line address */
    EX_HEXDUMP_X8(s1, addr);

    /* write out hex values */
    while (count < EX_HEXDUMP_CHRS_PER_LINE)
    {
      EX_HEXDUMP_X2X2(s1, buf[count], buf[count + 1]);
      count += 2;
    }

    vstr_add_rep_chr(s1, s1->len, ' ', 2);

    /* convert unprintable characters to the '.' character */
    vstr_conv_unprintable_chr(s2, 1, EX_HEXDUMP_CHRS_PER_LINE, flags, '.');

    /* write out characters, converting reference and pointer nodes to
     * _BUF nodes */
    vstr_add_vstr(s1, s1->len, s2, 1, EX_HEXDUMP_CHRS_PER_LINE,
                  VSTR_TYPE_ADD_ALL_BUF);
    vstr_add_rep_chr(s1, s1->len, '\n', 1);

    addr += EX_HEXDUMP_CHRS_PER_LINE;
    
    /* delete the set of characters just processed */
    vstr_del(s2, 1, EX_HEXDUMP_CHRS_PER_LINE);

    /* if any of the above memory mgmt failed, error */
    if (s1->conf->malloc_bad)
      errno = ENOMEM, err(EXIT_FAILURE, "adding data");

    /* note that we don't want to create data indefinitely, so stop
     * according to in core configuration */
    if (s1->len > EX_MAX_W_DATA_INCORE)
      return (TRUE);
  }

  if (last && s2->len)
  { /* do the same as above, but print the partial line for
     * the end of a file */
    size_t got = s2->len;
    size_t missing = EX_HEXDUMP_CHRS_PER_LINE - s2->len;
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

    /* Add spaces until the point where the characters should start */
    vstr_add_rep_chr(s1, s1->len, ' ', (missing * 2) + (missing / 2) + 2);

    vstr_conv_unprintable_chr(s2, 1, s2->len, flags, '.');
    vstr_add_vstr(s1, s1->len, s2, 1, s2->len, VSTR_TYPE_ADD_ALL_BUF);

    vstr_add_cstr_buf(s1, s1->len, "\n");

    addr += s2->len;
    vstr_del(s2, 1, s2->len);

    /* if any of the above memory mgmt failed, error */
    if (s1->conf->malloc_bad)
      errno = ENOMEM, err(EXIT_FAILURE, "adding data");

    return (TRUE);
  }

  return (FALSE);
}

static void ex_hexdump_process_limit(Vstr_base *s1, Vstr_base *s2,
                                     unsigned int lim)
{
  while (s2->len > lim)
  { /* Finish processing read data (try writing if we need memory) */
    int proc_data = ex_hexdump_process(s1, s2, !lim);

    if (!proc_data && (io_put(s1, STDOUT_FILENO) == IO_BLOCK))
      io_block(-1, STDOUT_FILENO);
  }
}

/* we process an entire file at a time... */
static void ex_hexdump_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2,
                                            int fd)
{
  /* read/process/write loop */
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s2, fd);
    
    if (io_r_state == IO_EOF)
      break;
    
    ex_hexdump_process(s1, s2, FALSE);

    io_w_state = io_put(s1, 1);

    io_limit(io_r_state, fd, io_w_state, 1, s1);
  }

  /* write out all of the end of the file,
   * so the next file starts on a new line */
  ex_hexdump_process_limit(s1, s2, 0);
}


int main(int argc, char *argv[])
{ /* This is "hexdump", as it should be by default */
  Vstr_base *s2 = NULL;
  Vstr_base *s1 = ex_init(&s2); /* init the library, and create two strings */
  int count = 1; /* skip the program name */
  unsigned int use_mmap = FALSE;

  /* parse command line arguments... */
  while (count < argc)
  { /* quick hack getopt_long */
    if (!strcmp("--", argv[count]))
    {
      ++count;
      break;
    }
    else if (!strcmp("--mmap", argv[count])) /* toggle use of mmap */
      use_mmap = !use_mmap;
    
    else if (!strcmp("--none", argv[count])) /* choose what is displayed */
      prnt_high_chars = PRNT_NONE; /* just simple 7 bit ASCII, no spaces */
    else if (!strcmp("--space", argv[count]))
      prnt_high_chars = PRNT_SPAC; /* allow spaces */
    else if (!strcmp("--high", argv[count]))
      prnt_high_chars = PRNT_HIGH; /* allow high bit characters */
    
    else if (!strcmp("--version", argv[count]))
    { /* print version and exit */
      vstr_add_fmt(s1, 0, "%s", "\
jhexdump 1.0.0\n\
Written by James Antill\n\
\n\
Uses Vstr string library.\n\
");
      goto out;
    }
    else if (!strcmp("--help", argv[count]))
    { /* print version and exit */
      vstr_add_fmt(s1, 0, "%s", "\
Usage: jhexdump [STRING]...\n\
   or: jhexdump OPTION\n\
Repeatedly output a line with all specified STRING(s), or `y'.\n\
\n\
      --help     Display this help and exit\n\
      --version  Output version information and exit\n\
      --high     Allow space and high characters in ASCII output\n\
      --none     Allow only small amount of characters ASCII output (default)\n\
      --space    Allow space characters in ASCII output\n\
      --mmap     Toggle use of mmap() to load input files\n\
      --         Treat rest of cmd line as input filenames\n\
\n\
Report bugs to James Antill <james@and.org>.\n\
");
      goto out;
    }
    else
      break;
    ++count;
  }

  /* if no arguments are given just do stdin to stdout */
  if (count >= argc)
  {
    io_fd_set_o_nonblock(STDIN_FILENO);
    ex_hexdump_read_fd_write_stdout(s1, s2, STDIN_FILENO);
  }

  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    unsigned int ern = 0;

    ASSERT(!s2->len); /* all input is fully processed before each new file */
    
    /* try to mmap the file */
    if (use_mmap)
      vstr_sc_mmap_file(s2, s2->len, argv[count], 0, 0, &ern);

    if (!use_mmap ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_FSTAT_ERRNO) ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_MMAP_ERRNO) ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_TOO_LARGE))
    { /* if mmap didn't work ... do a read/alter/write loop */
      int fd = io_open(argv[count]);
      
      ex_hexdump_read_fd_write_stdout(s1, s2, fd);

      if (close(fd) == -1)
        warn("close(%s)", argv[count]);
    }
    else if (ern && (ern != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
      err(EXIT_FAILURE, "add");
    else /* mmap worked so processes the entire file at once */
      ex_hexdump_process_limit(s1, s2, 0);
    
    ++count;
  }
  ASSERT(!s2->len); /* all input is fully processed before each new file */
  
  /* Cleanup... */
 out:
  io_put_all(s1, STDOUT_FILENO);
  
  exit (ex_exit(s1, s2));
}
