#ifndef DATE_H
#define DATE_H

extern const char *date_rfc1123(time_t);
extern const char *date_rfc850(time_t);
extern const char *date_asctime(time_t);

#endif
