#ifndef HTTPD_CONF_MAIN_H
#define HTTPD_CONF_MAIN_H

#ifndef HTTPD_H
# error "include just httpd.h"
#endif

#include "opt_serv.h"
#include "mime_types.h"

/* **** defaults for runtime conf **** */
#define HTTPD_CONF_VERSION "0.9.99"

#define HTTPD_CONF_DEF_SERVER_NAME "jhttpd/" HTTPD_CONF_VERSION " (Vstr)"
#define HTTPD_CONF_DEF_POLICY_NAME "<default>"
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
#define HTTPD_CONF_DEF_MIME_TYPE "" /* "application/octet-stream" */
#define HTTPD_CONF_MIME_TYPE_MAIN "/etc/mime.types"
#define HTTPD_CONF_MIME_TYPE_XTRA NULL
#define HTTPD_CONF_USE_ERR_406 TRUE
#define HTTPD_CONF_USE_CANONIZE_HOST FALSE
#define HTTPD_CONF_USE_HOST_ERR_400 TRUE
#define HTTPD_CONF_USE_CHK_HOST_ERR TRUE
#define HTTPD_CONF_USE_REMOVE_FRAG TRUE
#define HTTPD_CONF_USE_REMOVE_QUERY FALSE
#define HTTPD_CONF_USE_POSIX_FADVISE FALSE /* NOTE that this SEGV's on FC1 */
#define HTTPD_CONF_USE_REQ_CONFIGURATION TRUE
#define HTTPD_CONF_USE_CHK_HDR_SPLIT TRUE
#define HTTPD_CONF_USE_CHK_HDR_NIL TRUE
#define HTTPD_CONF_REQ_CONF_MAXSZ (16 * 1024)

/* this is configurable, but is much higher than EX_MAX_R_DATA_INCORE due to
 * allowing largish requests with HTTP */
#ifdef VSTR_AUTOCONF_NDEBUG
# define HTTPD_CONF_INPUT_MAXSZ (  1 * 1024 * 1024)
#else
# define HTTPD_CONF_INPUT_MAXSZ (128 * 1024) /* debug */
#endif

typedef struct Httpd_policy_opts
{
 Vstr_ref *ref;
 struct Httpd_opts *beg;
 struct Httpd_policy_opts *next;
 Vstr_base *policy_name;

 Vstr_base   *document_root;
 Vstr_base   *server_name;
 Vstr_base   *dir_filename;
 Mime_types   mime_types[1];
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

 unsigned int use_err_406 : 1;
 unsigned int use_canonize_host : 1;
 unsigned int use_host_err_400 : 1;
 unsigned int use_chk_host_err : 1;
 unsigned int remove_url_frag : 1;
 unsigned int remove_url_query : 1;

 unsigned int use_posix_fadvise : 1;
 
 unsigned int use_req_configuration : 1;
 unsigned int chk_hdr_split : 1; /* 16th bitfield */
 unsigned int chk_hdr_nil : 1; /* 17th bitfield */
 
 unsigned int max_header_sz;

 size_t max_req_conf_sz;
} Httpd_policy_opts; /* NOTE: remember to copy changes to
                      * httpd_conf_main.c:httpd_conf_main_policy_copy */

typedef struct Httpd_opts
{
 Opt_serv_opts s[1];
 time_t beg_time;
 Conf_parse *conf;
 unsigned int conf_num;
 Conf_token match_connection[1];
 Conf_token tmp_match_connection[1];
 Conf_token match_request[1];
 Conf_token tmp_match_request[1];
 Httpd_policy_opts def_policy[1];
} Httpd_opts;
#define HTTPD_CONF_MAIN_POLICY_INIT_OPTS {                       \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
                                                                 \
    NULL,                                                        \
    NULL,                                                        \
    NULL,                                                        \
    {MIME_TYPES_INIT},                                           \
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
    HTTPD_CONF_USE_ERR_406,                                      \
    HTTPD_CONF_USE_CANONIZE_HOST,                                \
    HTTPD_CONF_USE_HOST_ERR_400,                                 \
    HTTPD_CONF_USE_CHK_HOST_ERR,                                 \
    HTTPD_CONF_USE_REMOVE_FRAG,                                  \
    HTTPD_CONF_USE_REMOVE_QUERY,                                 \
    HTTPD_CONF_USE_POSIX_FADVISE,                                \
    HTTPD_CONF_USE_REQ_CONFIGURATION,                            \
    HTTPD_CONF_USE_CHK_HDR_SPLIT,                                \
    HTTPD_CONF_USE_CHK_HDR_NIL,                                  \
    HTTPD_CONF_INPUT_MAXSZ,                                      \
    HTTPD_CONF_REQ_CONF_MAXSZ}
    
#define HTTPD_CONF_MAIN_DECL_OPTS(N, x)         \
    Httpd_opts N[1] = {                         \
     {                                          \
      {{OPT_SERV_CONF_INIT_OPTS(x)}},           \
      (time_t)-1,                               \
      NULL,                                     \
      0,                                        \
      {CONF_TOKEN_INIT},                        \
      {CONF_TOKEN_INIT},                        \
      {CONF_TOKEN_INIT},                        \
      {CONF_TOKEN_INIT},                        \
      {HTTPD_CONF_MAIN_POLICY_INIT_OPTS}        \
     }                                          \
    }

extern int  httpd_conf_main_init(Httpd_opts *);
extern void httpd_conf_main_free(Httpd_opts *);

extern Httpd_policy_opts *httpd_conf_main_policy_make(Httpd_opts *);

extern int httpd_policy_connection(struct Con *,
                                   Conf_parse *, const Conf_token *);
extern int httpd_policy_request(struct Con *, struct Httpd_req_data *,
                                Conf_parse *, const Conf_token *);


extern int httpd_conf_main(Httpd_opts *, Conf_parse *, Conf_token *);
extern int httpd_conf_main_parse_cstr(Vstr_base *, Httpd_opts *, const char *);
extern int httpd_conf_main_parse_file(Vstr_base *, Httpd_opts *, const char *);

#endif
