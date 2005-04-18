
#include <stdlib.h>
#include <vstr.h>

#include <err.h>

#include "date.h"

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

/* FIXME: fix this to use constant data which is always in the C locale */

#define DATE__RET_SZ    128
#define DATE__CACHE_NUM   2

/* use macro so we pass a constant fmt string for gcc's checker */
#define SERV__STRFTIME(val, gettime, fmt) do {                   \
      static char ret[DATE__CACHE_NUM][DATE__RET_SZ];            \
      static time_t saved_val[DATE__CACHE_NUM] = {0,};           \
      static unsigned int saved_count = DATE__CACHE_NUM - 1;     \
      struct tm store_tm_val[1];                                 \
      struct tm *tm_val = NULL;                                  \
      unsigned int num = 0;                                      \
                                                                 \
      while (num < DATE__CACHE_NUM)                              \
      {                                                          \
        if (saved_val[num] == val)                               \
        {                                                        \
          saved_count = num;                                     \
          return (ret[num]);                                     \
        }                                                        \
        ++num;                                                   \
      }                                                          \
      saved_count = (saved_count + 1) % DATE__CACHE_NUM;         \
                                                                 \
      if (!(tm_val = gettime(&val, store_tm_val)))               \
        err(EXIT_FAILURE, #gettime );                            \
                                                                 \
      saved_val[saved_count] = val;                              \
      strftime(ret[saved_count], DATE__RET_SZ, fmt, tm_val);     \
                                                                 \
      return (ret[saved_count]);                                 \
    } while (FALSE)

const char *date_rfc1123(time_t val)
{
  SERV__STRFTIME(val, gmtime_r,    "%a, %d %b %Y %T GMT");
}
const char *date_rfc850(time_t val)
{
  SERV__STRFTIME(val, gmtime_r,    "%A, %d-%b-%y %T GMT");
}
const char *date_asctime(time_t val)
{
  SERV__STRFTIME(val, gmtime_r,    "%a %b %e %T %Y");
}
const char *date_syslog(time_t val)
{
  SERV__STRFTIME(val, localtime_r, "%b %e %T");
}

