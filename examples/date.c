
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

/* use macro so we pass a constant fmt string for gcc's checker */
#define SERV__STRFTIME(val, loc, fmt) do {      \
      static char ret[4096];                    \
      struct tm *tm_val = gmtime(&val);         \
                                                \
      if (!tm_val)                              \
        err(EXIT_FAILURE, "gmtime");            \
                                                \
      strftime(ret, sizeof(ret), fmt, tm_val);  \
                                                \
      return (ret);                             \
    } while (FALSE)

const char *date_rfc1123(time_t val)
{
  SERV__STRFTIME(val, FALSE, "%a, %d %b %Y %T GMT");
}
const char *date_rfc850(time_t val)
{
  SERV__STRFTIME(val, FALSE, "%A, %d-%b-%y %T GMT");
}
const char *date_asctime(time_t val)
{
  SERV__STRFTIME(val, FALSE, "%a %b %e %T %Y");
}
const char *date_syslog(time_t val)
{
  SERV__STRFTIME(val, TRUE, "%b %e %T");
}

