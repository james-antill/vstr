#ifndef EX_UTILS_H
#define EX_UTILS_H 1

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#ifndef __GNUC__
# define __PRETTY_FUNCTION__ "(unknown)"
#endif


#define DIE(x, args...)  \
  ex_utils_die(__FILE__, __LINE__, __FUNCTION__, x, ## args)
#define WARN(x, args...) \
  ex_utils_die(__FILE__, __LINE__, __FUNCTION__, x, ## args)

/* read from the file descriptor, die if there is an error, don't do anything
 * if we have too much data pending */
#define EX_UTILS_LIMBLOCK_READ_ALL(s1, fd, kg) do { \
  if ((s1)->len < MAX_R_DATA_INCORE) \
  { \
    vstr_sc_read_iov_fd((s1), (s1)->len, (fd), 2, 32, &err); \
    if (err == VSTR_TYPE_SC_READ_FD_ERR_EOF) \
      (kg) = FALSE; \
    else if ((err == VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO) && \
        ((errno == EINTR) || (errno == EAGAIN))) \
      break; \
    else if (err) \
      DIE("read:"); \
  } \
} while (FALSE)

/* write to the file descriptor, die if there is an error, block
 * if we have too much data pending */
#define EX_UTILS_LIMBLOCK_WRITE_ALL(s1, fd) do { \
  if (!vstr_sc_write_fd((s1), 1, (s1)->len, (fd), NULL)) \
  { \
    if ((errno != EAGAIN) && (errno != EINTR)) \
      DIE("write:"); \
    \
    if ((s1)->len > MAX_W_DATA_INCORE) \
      ex_utils_poll_stdout(); \
  } \
} while (FALSE)

#define assert(x) do { if (!(x)) DIE("Assert=\"" #x "\""); } while (FALSE)

extern void ex_utils_die(const char *, unsigned int, const char *,
                         const char *, ...)
    __attribute__ ((__format__ (__printf__, 4, 5)));
extern void ex_utils_warn(const char *, unsigned int, const char *,
                          const char *, ...)
    __attribute__ ((__format__ (__printf__, 4, 5)));
extern int ex_utils_set_o_nonblock(int fd);
extern void ex_utils_poll_stdout(void);

#endif
