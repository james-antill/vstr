#ifndef HTTPD_CONF_REQ_H
#define HTTPD_CONF_REQ_H

#include <vstr.h>

struct Con;
struct Httpd_opts;
struct Httpd_req_data;

extern int httpd_conf_req_parse_file(Vstr_base *, struct Httpd_opts *,
                                     struct Con *, struct Httpd_req_data *);

#endif
