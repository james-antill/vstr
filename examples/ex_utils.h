#ifndef EX_UTILS_H
#define EX_UTILS_H 1

/* ************************************************************************** */
/* headers: Vstr (and all supporting system headers), plus extra ones we need */
/* ************************************************************************** */

#define VSTR_COMPILE_INCLUDE 1 /* make Vstr include it's system headers */
#include <vstr.h>

#include <errno.h>

#include <err.h> /* BSD/Linux header see: man errx */

#include <sys/poll.h>

#include <sys/types.h> /* stat + open + STDXXX_FILENO */
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h> /* strncmp() etc. in GETOPT macros */

/* **************************************************************** */
/* defines: TRUE/FALSE and assert(), Note that GETOPT is used later */
/* **************************************************************** */

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

/* Simple getopt code... */
#define EX_UTILS_GETOPT_NUM(name, var) \
    else if (!strncmp("--" name, argv[count], strlen("--" name))) \
    { \
      if (!strncmp("--" name "=", argv[count], strlen("--" name "="))) \
        (var) = strtol(argv[count] + strlen("--" name "="), NULL, 0); \
      else \
      { \
        (var) = 0; \
        \
        ++count; \
        if (count >= argc) \
          break; \
        \
        (var) = strtol(argv[count], NULL, 0); \
      } \
    } \
    else if (0) ASSERT(FALSE)

#define EX_UTILS_GETOPT_CSTR(name, var) \
    else if (!strncmp("--" name, argv[count], strlen("--" name))) \
    { \
      if (!strncmp("--" name "=", argv[count], strlen("--" name "="))) \
        (var) = argv[count] + strlen("--" name "="); \
      else \
      { \
        (var) = NULL; \
        \
        ++count; \
        if (count >= argc) \
          break; \
        \
        (var) = argv[count]; \
      } \
    } \
    else if (0) ASSERT(FALSE)



#define assert(x) do { if (x) {} else errx(EXIT_FAILURE, "assert(" #x "), FAILED at line %u", __LINE__); } while (FALSE)
#define ASSERT(x) do { if (x) {} else errx(EXIT_FAILURE, "ASSERT(" #x "), FAILED at line %u", __LINE__); } while (FALSE)



/* ********************************* */
/* generic POSIX IO helper functions */
/* ********************************* */

/* limits on amount of data we keep in core -- can be overridden */
/* Note that EX_UTILS_NO_USE_INPUT should be defined if Input IO isn't needed */
#ifndef EX_MAX_R_DATA_INCORE
# define EX_MAX_R_DATA_INCORE (8 * 1024)
#endif
#ifndef EX_MAX_W_DATA_INCORE
# define EX_MAX_W_DATA_INCORE (8 * 1024)
#endif

#define IO_OK    0
#define IO_BLOCK 1
#define IO_EOF   2
#define IO_NONE  3

/* block waiting for IO read, write or both... */
static void io_block(int io_r_fd, int io_w_fd)
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
  
  while (poll(ios_beg, num, -1) == -1) /* can't timeout */
  {
    if (errno != EINTR)
      err(EXIT_FAILURE, "poll");
  }
}

/* Try and move some data from Vstr string to fd */
static int io_put(Vstr_base *io_w, int fd)
{
  if (!io_w->len)
    return (IO_NONE);

  if (!vstr_sc_write_fd(io_w, 1, io_w->len, fd, NULL))
  {
    if (errno == EAGAIN)
      return (IO_BLOCK);
    
    err(EXIT_FAILURE, "write");
  }

  return (IO_OK);
}

/* loop outputting data until empty, blocking when needed */
static void io_put_all(Vstr_base *io_w, int fd)
{
  int state = IO_NONE;

  while ((state = io_put(io_w, fd)) != IO_NONE)
  {
    if (state == IO_BLOCK)
      io_block(-1, fd);
  }
}
    
#ifndef EX_UTILS_NO_USE_INPUT
/* Try and move some data from fd to Vstr string */
static int io_get(Vstr_base *io_r, int fd)
{
  if (io_r->len < EX_MAX_R_DATA_INCORE)
  {
    unsigned int ern = 0;

    vstr_sc_read_iov_fd(io_r, io_r->len, fd, 8, 16, &ern);
    
    if (ern == VSTR_TYPE_SC_READ_FD_ERR_EOF)
      return (IO_EOF);
    else if ((ern == VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO) && (errno == EAGAIN))
      return (IO_BLOCK);
    else if (ern)
      err(EXIT_FAILURE, "read");
  }

  return (IO_OK);
}

/* block read or writting, depending on limits */
static void io_limit(int io_r_state, int io_r_fd,
                     int io_w_state, int io_w_fd, Vstr_base *s_w)
{
  if (io_w_state == IO_BLOCK) /* allow 16k to build up */
  {
    if (io_r_state == IO_BLOCK) /* block to either get or put some data */
      io_block(io_r_fd, io_w_fd);
    else if (s_w->len > EX_MAX_W_DATA_INCORE)
      io_block(-1, io_w_fd); /* block to put more data */
  }
  else if ((io_w_state == IO_NONE) && (io_r_state == IO_BLOCK))
    io_block(io_r_fd, -1); /* block to get more data */
}
#endif

/* generic POSIX IO functions that _don't_ call Vstr functions */

static int io_fd_set_o_nonblock(int fd)
{
  int flags = 0;

  /* see if the NONBLOCK flag is set... */
  if ((flags = fcntl(fd, F_GETFL)) == -1)
    return (FALSE);

  /* if it isn't try and add it to the current flags */
  if (!(flags & O_NONBLOCK) &&
      (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1))
    return (FALSE);

  return (TRUE);
}

#ifndef EX_UTILS_NO_USE_OPEN
# ifndef VSTR_AUTOCONF_HAVE_OPEN64
#  define open64 open
# endif
static int io_open(const char *filename)
{
  int fd = open64(filename, O_RDONLY | O_NOCTTY);

  if (fd == -1)
    err(EXIT_FAILURE, "open(%s)", filename);

  /* When doing IO, it should always be non-blocking */
  io_fd_set_o_nonblock(fd);

  return (fd);
}
#endif

/* ************************ */
/* generic helper functions */
/* ************************ */

/* Example init function */
static Vstr_base *ex_init(Vstr_base **s2)
{
  Vstr_base *s1 = NULL;
  struct stat stat_buf;

  if (!vstr_init()) /* init the library */
    errno = ENOMEM, err(EXIT_FAILURE, "init");

  
  /* alter the node buffer size to be whatever the stdout block size is */
  if (fstat(1, &stat_buf) == -1)
    warn("fstat(STDOUT)");
  else
  {
    if (!stat_buf.st_blksize) /* this is allowed to be Zero */
      stat_buf.st_blksize = 4096;

    if (!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_BUF_SZ,
                        stat_buf.st_blksize / 8))
      warnx("Couldn't alter node size to match block size");
  }

  /* create strings... */  
  if (!(s1 = vstr_make_base(NULL)) ||
      (s2 && !(*s2 = vstr_make_base(NULL))))
    errno = ENOMEM, err(EXIT_FAILURE, "Create string");

  /* create some data storage for _both_ of the above strings */
  vstr_make_spare_nodes(NULL, VSTR_TYPE_NODE_BUF, 32);
  
  /* Try and make stdout non-blocking, if it is a file this won't do anything */
  io_fd_set_o_nonblock(STDOUT_FILENO);

  return (s1);
}

/* Example exit function */
static int ex_exit(Vstr_base *s1, Vstr_base *s2)
{
  /* These next calls are only really needed for debugging,
   * in that when they are done any memory leaks can be seen in debugging mode.
   */

  /* As with the system free() both of these are ok if passed NULL */
  
  /* free s1, our String object */
  vstr_free_base(s1);
  /* free s2, our String object */
  vstr_free_base(s2);

  /* "exit" Vstr, this free's all internal data and no library calls apart from
   * vstr_init() should be called after this.
   */
  vstr_exit();

  return (EXIT_SUCCESS);
}

#endif
