#ifndef HTTPD_H
#define HTTPD_H

#include <vstr.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "evnt.h"

#include "httpd_conf_main.h"
#include "httpd_conf_req.h"
#include "httpd_err_codes.h"


struct Http_hdrs
{
 Vstr_sect_node hdr_ua[1];
 Vstr_sect_node hdr_referer[1]; /* NOTE: referrer */

 Vstr_sect_node hdr_expect[1];
 Vstr_sect_node hdr_host[1];
 Vstr_sect_node hdr_if_match[1];
 Vstr_sect_node hdr_if_modified_since[1];
 Vstr_sect_node hdr_if_none_match[1];
 Vstr_sect_node hdr_if_range[1];
 Vstr_sect_node hdr_if_unmodified_since[1];

 /* can have multiple headers... */
 struct Http_hdrs__multi {
  Vstr_base *combiner_store;
  Vstr_base *comb;
  Vstr_sect_node hdr_accept[1];
  Vstr_sect_node hdr_accept_encoding[1];
  Vstr_sect_node hdr_accept_language[1];
  Vstr_sect_node hdr_connection[1];
  Vstr_sect_node hdr_range[1];
 } multi[1];
};

struct Httpd_req_data
{
 struct Http_hdrs http_hdrs[1];
 Vstr_base *fname;
 size_t len;
 size_t path_pos;
 size_t path_len;
 unsigned int error_code;
 const char * error_line;
 const char * error_msg;
 size_t       error_len;
 Vstr_sects *sects;
 struct stat64 f_stat[1];
 size_t orig_io_w_len;
 Vstr_base *f_mmap;
 Vstr_base *xtra_content;
 const Vstr_base *content_type_vs1;
 size_t           content_type_pos;
 size_t           content_type_len;
 const Vstr_base *content_location_vs1;
 size_t           content_location_pos;
 size_t           content_location_len;
 const Vstr_base *content_md5_vs1; /* Note this is valid for gzip/range */
 size_t           content_md5_pos;
 size_t           content_md5_len;
 const Vstr_base *cache_control_vs1;
 size_t           cache_control_pos;
 size_t           cache_control_len;
 const Vstr_base *expires_vs1;
 size_t           expires_pos;
 size_t           expires_len;

 time_t now;
 
 unsigned int ver_0_9 : 1;
 unsigned int ver_1_1 : 1;
 unsigned int use_mmap : 1;
 unsigned int head_op : 1;

 unsigned int content_encoding_gzip  : 1;
 unsigned int content_encoding_xgzip : 1; /* only valid if gzip is TRUE */
 
 unsigned int content_encoding_identity : 1;

 unsigned int direct_uri      : 1;
 unsigned int direct_filename : 1;
 
 unsigned int parse_accept : 1;
 unsigned int allow_accept_encoding : 1;
 unsigned int output_keep_alive_hdr : 1;

 unsigned int user_return_error_code : 1;
 
 unsigned int using_req : 1;
 unsigned int done_once : 1;
 
 unsigned int malloc_bad : 1;
};

struct Con
{
 struct Evnt evnt[1];

 int f_fd;
 VSTR_AUTOCONF_uintmax_t f_off;
 VSTR_AUTOCONF_uintmax_t f_len;

 unsigned int parsed_method_ver_1_0 : 1;
 unsigned int keep_alive : 3;
 
 unsigned int use_sendfile : 1; 
};

#define HTTP_NON_KEEP_ALIVE 0
#define HTTP_1_0_KEEP_ALIVE 1
#define HTTP_1_1_KEEP_ALIVE 2

/* Linear Whitespace, a full stds. check for " " and "\t" */
#define HTTP_LWS " \t"

/* End of Line */
#define HTTP_EOL "\r\n"

/* End of Request - blank line following previous line in request */
#define HTTP_END_OF_REQUEST HTTP_EOL HTTP_EOL

/* HTTP crack -- Implied linear whitespace between tokens, note that it
 * is *LWS == *([CRLF] 1*(SP | HT)) */
#define HTTP_SKIP_LWS(s1, p, l) do {                                    \
      char http__q_tst_lws = 0;                                         \
                                                                        \
      if (!l) break;                                                    \
      http__q_tst_lws = vstr_export_chr(s1, p);                         \
      if ((http__q_tst_lws != '\r') && (http__q_tst_lws != ' ') &&      \
          (http__q_tst_lws != '\t'))                                    \
        break;                                                          \
                                                                        \
      http__skip_lws(s1, &p, &l);                                       \
    } while (FALSE)

extern int httpd_sc_add_default_hostname(struct Httpd_opts *opts,
                                         Vstr_base *lfn, size_t pos);
extern void httpd_req_absolute_uri(struct Httpd_opts *,
                                   struct Con *, struct Httpd_req_data *,
                                   Vstr_base *);

#endif
