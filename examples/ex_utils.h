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

/* if we didn't do anything last time around the loop,
 * wait for input, or output... */
#define EX_UTILS_LIMBLOCK_WAIT(s1, s2, fd, s1_psz, kg, dalr) do { \
    Vstr_base *ts2 = (s2); \
    \
    if ((kg) && !(dalr)) \
    { \
      if ((ts2) && \
          ((s1)->len < MAX_W_DATA_INCORE) &&  \
          ((ts2)->len < MAX_R_DATA_INCORE)) \
      { \
        if ((s1)->len >= (s1_psz)) \
          ex_utils_poll_fds_rw((fd), 1); \
        else \
          ex_utils_poll_fds_r(fd); \
      } \
      else \
        ex_utils_poll_fds_w(1); \
    } \
 } while (FALSE)

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
      ex_utils_poll_fds_w(1); \
  } \
} while ((s1)->len > MAX_W_DATA_INCORE)

#define assert(x) do { if (!(x)) DIE("Assert=\"" #x "\""); } while (FALSE)

extern void ex_utils_die(const char *, unsigned int, const char *,
                         const char *, ...)
    __attribute__ ((__format__ (__printf__, 4, 5)));
extern void ex_utils_warn(const char *, unsigned int, const char *,
                          const char *, ...)
    __attribute__ ((__format__ (__printf__, 4, 5)));
extern int ex_utils_set_o_nonblock(int fd);
extern void ex_utils_poll_fds_r(int);
extern void ex_utils_poll_fds_rw(int, int);
extern void ex_utils_poll_fds_w(int);

#endif
