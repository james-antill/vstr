#ifndef DATE_H
#define DATE_H

#include <vstr.h>

#if VSTR_AUTOCONF_TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if VSTR_AUTOCONF_HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

extern const char *date_rfc1123(time_t);
extern const char *date_rfc850(time_t);
extern const char *date_asctime(time_t);
extern const char *date_syslog(time_t);

#endif
