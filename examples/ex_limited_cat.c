/* This is a limited cat program.
 * Reads from stdin if no args are given.
 */
#define EX_UTILS_NO_USE_BLOCK 1
#define EX_UTILS_NO_USE_LIMIT 1
#include "ex_utils.h"

#include <sys/time.h>
#include <time.h>

struct Ex_limcat_stats
{
 unsigned int  total;
 unsigned int *data;
 time_t       *stmp;
 size_t        sz;
 size_t        pos;
};

typedef struct Ex_limcat_opts
{
  unsigned int ilimit;
  unsigned int olimit;
  unsigned int period;

 /* stats */
 struct Ex_limcat_stats stat_i;
 struct Ex_limcat_stats stat_o;
} Ex_limcat_opts;

#define EX_LIM_SECS(x, y) (difftime(x, y))

static void lim_clear(struct Ex_limcat_stats *stats)
{
  time_t now = time(NULL);
  
  stats->total = 0;
  stats->pos   = 0;

  while (stats->pos < stats->sz)
  {
    stats->data[stats->pos] = 0;
    stats->stmp[stats->pos] = now;
    ++stats->pos;
  }

  stats->pos   = 0;
}

static void lim_init(struct Ex_limcat_stats *stats, unsigned int sz)
{
  if (!(stats->data = malloc(sizeof(unsigned int) * sz)))
    err(EXIT_FAILURE, "init");

  if (!(stats->stmp = malloc(sizeof(time_t) * sz)))
    err(EXIT_FAILURE, "init");

  stats->sz    = sz;
  lim_clear(stats);
}

static void lim_add(struct Ex_limcat_stats *stats, time_t now, unsigned int val)
{
  unsigned int secs = EX_LIM_SECS(now, stats->stmp[stats->pos]);
  
  if (secs >= stats->sz)
    secs = stats->sz;
  
  do
  {
    stats->pos = (stats->pos + 1) % stats->sz;

    ASSERT(stats->total >= stats->data[stats->pos]);
    
    stats->total            -= stats->data[stats->pos];
    stats->data[stats->pos]  = 0;
    stats->stmp[stats->pos]  = now;
  } while (--secs);

  stats->data[stats->pos] += val;
  stats->total            += val;
}

static int lim_calc_ok(struct Ex_limcat_stats *stats, unsigned int limit)
{
  if (stats->total < limit)
    return (TRUE);

  return (FALSE);
}

/* block waiting for IO read, write or both... */
static void io_block(int io_r_fd, int io_w_fd, int wait_secs)
{
  struct pollfd ios_beg[2];
  struct pollfd *ios = ios_beg;
  unsigned int num = 0;
  
  ios[0].revents = ios[1].revents = 0;
  
  if (io_r_fd == io_w_fd)
  { /* block on both read and write, same fds */
    num = 1;
    ios[0].events = POLLIN | POLLOUT;
    ios[0].fd     = io_w_fd;
  }
  else
  { /* block on read or write or both */
    if (io_r_fd != -1)
    {
      ios->events = POLLIN;
      ios->fd     = io_r_fd;
      ++num; ++ios;
    }
    if (io_w_fd != -1)
    {
      ios->events = POLLOUT;
      ios->fd     = io_w_fd;
      ++num; ++ios;
    }
  }

  if (poll(ios_beg, num, wait_secs) == -1)
  {
    if (errno != EINTR)
      err(EXIT_FAILURE, "poll");
  }
}

  /* block read or writting, depending on limits */
static void io_limit(int io_r_state, int io_r_fd,
                     int io_w_state, int io_w_fd, Vstr_base *s_w)
{
  if (io_w_state == IO_BLOCK) /* allow 16k to build up */
  {
    if (io_r_state == IO_BLOCK) /* block to either get or put some data */
      io_block(io_r_fd, io_w_fd, -1);
    else if (s_w->len > EX_MAX_W_DATA_INCORE)
      io_block(-1, io_w_fd, -1); /* block to put more data */
  }
  else if ((io_w_state == IO_NONE) && (io_r_state == IO_BLOCK))
    io_block(io_r_fd, -1, -1); /* block to get more data */
}


static void ex_limcat_read_fd_write_stdout(Ex_limcat_opts *opts,
                                           Vstr_base *s1, int fd)
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

static void ex_limcat_write_stdout(Ex_limcat_opts *opts, Vstr_base *s1)
{
  int state = IO_NONE;

  while ((state = io_put(s1, STDOUT_FILENO)) != IO_NONE)
  {
    if (state == IO_BLOCK)
      io_block(-1, STDOUT_FILENO, -1);
  }
}

static unsigned int arg_lim(int *count, int argc, char *argv[])
{
  char *end = NULL;
  unsigned int ret = 0;
  
  if (++*count == argc)
    errx(EXIT_FAILURE, "No argument for limit.");

  ret = strtoul(argv[*count], &end, 10);

  if (!end[0] || !end[1])
    switch (*end)
    {
      case 0:   return (ret);
      case 'b': return (ret);
      case 'k': return (ret * 1024);
      case 'm': return (ret * 1024 * 1024);
      case 'g': return (ret * 1024 * 1024 * 1024);
      default:
        break;
    }

  errx(EXIT_FAILURE, "Bad format for limit.");
}

static unsigned int arg_period(int *count, int argc, char *argv[])
{
  char *end = NULL;
  unsigned int ret = 0;
  
  if (++*count == argc)
    errx(EXIT_FAILURE, "No argument for period.");

  ret = strtoul(argv[*count], &end, 10);

  if (!end[0] || !end[1])
    switch (*end)
    {
      case 0:   return (ret);
      case 's': return (ret);
      case 'm': return (ret * 60);
      case 'h': return (ret * 60 * 60);
      case 'd': return (ret * 60 * 60 * 24);
      default:
        break;
    }

  errx(EXIT_FAILURE, "Bad format for period.");
}

int main(int argc, char *argv[])
{ 
  Vstr_base *s1 = ex_init(NULL); /* init the library etc. */
  int count = 1; /* skip the program name */
  unsigned int use_mmap = FALSE;
  static Ex_limcat_opts opts[1] = {{2 * 1024, 2 * 1024, 15}};

  while (count < argc)
  {
    if (!strcmp("--", argv[count]))
    {
      ++count;
      break;
    }
    
    else if (!strcmp("--mmap", argv[count]))
      use_mmap = !use_mmap;
    
    else if (!strcmp("--ilimit", argv[count]))
      opts->ilimit = arg_lim(&count, argc, argv);
    else if (!strcmp("--olimit", argv[count]))
      opts->olimit = arg_lim(&count, argc, argv);
    else if (!strcmp("--iolimit", argv[count]))
      opts->ilimit = opts->olimit = arg_lim(&count, argc, argv);
    
    else if (!strcmp("--period", argv[count]))
      opts->period = arg_period(&count, argc, argv);
    
    else if (!strcmp("--version", argv[count]))
    { /* print version and exit */
      vstr_add_fmt(s1, 0, "%s", "\
jlimcat 1.0.0\n\
Written by James Antill\n\
\n\
Uses Vstr string library.\n\
");
      goto out;
    }
    else if (!strcmp("--help", argv[count]))
    { /* print version and exit */
      vstr_add_fmt(s1, 0, "%s", "\
Usage: jlimcat [FILENAME]...\n\
   or: jlimcat OPTION\n\
Output filenames.\n\
\n\
      --help     Display this help and exit\n\
      --version  Output version information and exit\n\
      --ilimit   Limit of input per second (default 2k).\n\
      --olimit   Limit of output per second (default 2k).\n\
      --iolimit  Limit of input and output per second (default 2k).\n\
      --period   Period of time given as a buffer (default 15s).\n\
      --mmap     Toggle use of mmap() to load input files.\n\
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
    ex_limcat_read_fd_write_stdout(opts, s1, STDIN_FILENO);
  }
  
  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    unsigned int ern = 0;
    
    /* try to mmap the file */
    if (use_mmap)
      vstr_sc_mmap_file(s1, s1->len, argv[count], 0, 0, &ern);
 
    if (!use_mmap ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_FSTAT_ERRNO) ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_MMAP_ERRNO) ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_TOO_LARGE))
    { /* if mmap didn't work ... do a read/alter/write loop */
      int fd = io_open(argv[count]);

      ex_limcat_read_fd_write_stdout(opts, s1, fd);

      if (close(fd) == -1)
        warn("close(%s)", argv[count]);
    }
    else if (ern && (ern != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
      err(EXIT_FAILURE, "mmap");
    else if (s1->len > EX_MAX_R_DATA_INCORE)
      ex_limcat_write_stdout(opts, s1);

    ++count;
  }

  /* output all remaining data */
  ex_limcat_write_stdout(opts, s1);

  exit (ex_exit(s1, NULL));
}
