#ifndef HTTPD_CONF_MAIN_H
#define HTTPD_CONF_MAIN_H

#include "opt_serv.h"

/* **** defaults for runtime conf **** */
#define HTTPD_CONF_VERSION "0.9.5"

#define HTTPD_CONF_DEF_NAME "jhttpd/" HTTPD_CONF_VERSION " (Vstr)"
#define HTTPD_CONF_USE_MMAP FALSE
#define HTTPD_CONF_USE_SENDFILE TRUE
#define HTTPD_CONF_USE_KEEPA TRUE
#define HTTPD_CONF_USE_KEEPA_1_0 TRUE
#define HTTPD_CONF_USE_VHOSTS FALSE
#define HTTPD_CONF_USE_RANGE TRUE
#define HTTPD_CONF_USE_RANGE_1_0 TRUE
#define HTTPD_CONF_USE_PUBLIC_ONLY FALSE
#define HTTPD_CONF_USE_GZIP_CONTENT_REPLACEMENT TRUE
#define HTTPD_CONF_DEF_DIR_FILENAME "index.html"
#define HTTPD_CONF_USE_DEFAULT_MIME_TYPE FALSE
#define HTTPD_CONF_DEF_MIME_TYPE "application/octet-stream"
#define HTTPD_CONF_MIME_TYPE_MAIN "/etc/mime.types"
#define HTTPD_CONF_MIME_TYPE_XTRA NULL
#define HTTPD_CONF_USE_ERR_406 TRUE
#define HTTPD_CONF_USE_CANONIZE_HOST FALSE
#define HTTPD_CONF_USE_HOST_ERR_400 TRUE

/* this is configurable, but is much higher than EX_MAX_R_DATA_INCORE due to
 * allowing largish requests with HTTP */
#ifdef VSTR_AUTOCONF_NDEBUG
# define HTTPD_CONF_INPUT_MAXSZ (  1 * 1024 * 1024)
#else
# define HTTPD_CONF_INPUT_MAXSZ (128 * 1024) /* debug */
#endif

typedef struct Httpd_opts
{
 Opt_serv_opts s[1];
 Vstr_base   *document_root;
 Vstr_base   *server_name;
 Vstr_base   *dir_filename;
 Vstr_base   *mime_types_default_type;
 Vstr_base   *mime_types_main; /* FIXME: free after init */
 Vstr_base   *mime_types_xtra;
 Vstr_base   *default_hostname;
 Vstr_base   *req_configuration_dir;
 
 unsigned int use_mmap : 1;
 unsigned int use_sendfile : 1;
 unsigned int use_keep_alive : 1;
 unsigned int use_keep_alive_1_0 : 1;
 unsigned int use_vhosts : 1;
 unsigned int use_range : 1;
 unsigned int use_range_1_0 : 1;
 unsigned int use_public_only : 1; /* 8th bitfield */
 unsigned int use_gzip_content_replacement : 1;
 unsigned int use_default_mime_type : 1;
 unsigned int use_err_406 : 1;
 unsigned int use_canonize_host : 1;
 unsigned int use_host_err_400 : 1; /* 12th bitfield */

 unsigned int max_header_sz;
} Httpd_opts;
#define HTTPD_CONF_MAIN_INIT_OPTS                                \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    HTTPD_CONF_USE_MMAP, HTTPD_CONF_USE_SENDFILE,                \
    HTTPD_CONF_USE_KEEPA, HTTPD_CONF_USE_KEEPA_1_0,              \
    HTTPD_CONF_USE_VHOSTS,                                       \
    HTTPD_CONF_USE_RANGE, HTTPD_CONF_USE_RANGE_1_0,              \
    HTTPD_CONF_USE_PUBLIC_ONLY,                                  \
    HTTPD_CONF_USE_GZIP_CONTENT_REPLACEMENT,                     \
    HTTPD_CONF_USE_DEFAULT_MIME_TYPE,                            \
    HTTPD_CONF_USE_ERR_406,                                      \
    HTTPD_CONF_USE_CANONIZE_HOST,                                \
    HTTPD_CONF_USE_HOST_ERR_400,                                 \
    HTTPD_CONF_INPUT_MAXSZ
#define HTTPD_CONF_MAIN_DECL_OPTS(N, x)         \
    Httpd_opts N[1] = {                         \
     {                                          \
      {{OPT_SERV_CONF_INIT_OPTS(x)}},           \
      HTTPD_CONF_MAIN_INIT_OPTS                 \
     }                                          \
    }

extern int  httpd_conf_main_init(Httpd_opts *);
extern void httpd_conf_main_free(Httpd_opts *);

extern int httpd_conf_main_parse_cstr(Vstr_base *, Httpd_opts *, const char *);
extern int httpd_conf_main_parse_file(Vstr_base *, Httpd_opts *, const char *);

#endif
