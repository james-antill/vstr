
#define _GNU_SOURCE 1 /* needed for caddr_t */

#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/poll.h>

#include "ex_utils.h"


/* die with error */
void ex_utils_die(const char *fl, unsigned int l, const char *fu,
                  const char *msg, ...)
{
  int saved_errno = errno;
  Vstr_base *s1 = NULL;
  unsigned int err = 0;

  s1 = vstr_make_base(NULL);
  if (s1)
  {
    va_list ap;

    vstr_add_fmt(s1, s1->len, "\nDIE: at %s%s%s%s:%u\n     ",
                 fl, *fu ? "(" : "", fu, *fu ? ")" : "", l);

    va_start(ap, msg);
    vstr_add_vfmt(s1, s1->len, msg, ap);
    va_end(ap);

    if (msg[strlen(msg) - 1] == ':')
      vstr_add_fmt(s1, s1->len, " %d %s", saved_errno, strerror(saved_errno));

    vstr_add_buf(s1, s1->len, "\n", 1);

    while (s1->len)
    {
      if (!vstr_sc_write_fd(s1, 1, s1->len, STDERR_FILENO, &err))
        if ((errno != EAGAIN) && (errno != EINTR))
          break;
    }

    vstr_free_base(s1);
  }

  _exit (EXIT_FAILURE);
}

/* warn about error */
void ex_utils_warn(const char *fl, unsigned int l, const char *fu,
                   const char *msg, ...)
{
  int saved_errno = errno;
  Vstr_base *s1 = NULL;
  unsigned int err = 0;

  s1 = vstr_make_base(NULL);
  if (s1)
  {
    va_list ap;

    vstr_add_fmt(s1, s1->len, "\nWARN: at %s%s%s%s:%u\n      ",
                 fl, *fu ? "(" : "", fu, *fu ? ")" : "", l);

    va_start(ap, msg);
    vstr_add_vfmt(s1, s1->len, msg, ap);
    va_end(ap);

    if (msg[strlen(msg) - 1] == ':')
      vstr_add_fmt(s1, s1->len, " %d %s", saved_errno, strerror(saved_errno));

    vstr_add_buf(s1, s1->len, "\n", 1);

    while (s1->len)
    {
      if (!vstr_sc_write_fd(s1, 1, s1->len, STDERR_FILENO, &err))
        if ((errno != EAGAIN) && (errno != EINTR))
          DIE("write:");
    }

    vstr_free_base(s1);
  }
}

int ex_utils_set_o_nonblock(int fd)
{
  int flags = 0;

  if ((flags = fcntl(fd, F_GETFL)) == -1)
    return (0);

  if (!(flags & O_NONBLOCK) &&
      (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1))
    return (0);

  return (1);
}

void ex_utils_poll_fds_w(int fd)
{
  struct pollfd one;

  one.fd = fd;
  one.events = POLLOUT;
  one.revents = 0;

  while (poll(&one, 1, -1) == -1) /* can't timeout */
  {
    if (errno != EINTR)
      DIE("poll:");
  }
}

void ex_utils_poll_fds_r(int fd)
{
  struct pollfd one;

  one.fd = fd;
  one.events = POLLIN;
  one.revents = 0;

  while (poll(&one, 1, -1) == -1) /* can't timeout */
  {
    if (errno != EINTR)
      DIE("poll:");
  }
}

void ex_utils_poll_fds_rw(int rfd, int wfd)
{
  struct pollfd two[2];

  two->fd = rfd;
  two->events = POLLIN;
  two->revents = 0;

  two->fd = wfd;
  two->events = POLLOUT;
  two->revents = 0;

  while (poll(two, 2, -1) == -1) /* can't timeout */
  {
    if (errno != EINTR)
      DIE("poll:");
  }
}
