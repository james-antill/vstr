
#include "httpd.h"
#include "date.h"

#include <err.h>

#define EX_UTILS_NO_USE_INIT  1
#define EX_UTILS_NO_USE_EXIT  1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_INPUT 1
#define EX_UTILS_NO_USE_BLOCK 1
#define EX_UTILS_NO_USE_PUT   1
#define EX_UTILS_NO_USE_BLOCKING_OPEN 1
#define EX_UTILS_USE_NONBLOCKING_OPEN 1
#define EX_UTILS_RET_FAIL     1
#include "ex_utils.h"

#define HTTPD_CONF_REQ__X_CONTENT_VSTR(x) do {                          \
      if (!req->xtra_content && !(req->xtra_content = vstr_make_base(NULL))) \
        return (FALSE);                                                 \
                                                                        \
      if (conf_sc_token_app_vstr(conf, token, req->xtra_content,        \
                                 &req-> x ## _vs1,                      \
                                 &req-> x ## _pos,                      \
                                 &req-> x ## _len))                     \
        return (FALSE);                                                 \
                                                                        \
      if ((token->type >= CONF_TOKEN_TYPE_QUOTE_DDD) &&                 \
          (token->type <= CONF_TOKEN_TYPE_QUOTE_S))                     \
        if (!conf_sc_conv_unesc(req->xtra_content,                      \
                                req-> x ## _pos,                        \
                                req-> x ## _len, &req-> x ## _len) ||   \
            req-> x ## _vs1 ->conf->malloc_bad)                         \
          return (FALSE);                                               \
    } while (FALSE)


#define HTTPD_CONF_REQ__X_HDR_CHK(x, y, z) do {                 \
      if (vstr_srch_cstr_chrs_fwd(x, y, z, HTTP_EOL))           \
        return (FALSE);                                         \
    } while (FALSE)

#define HTTPD_CONF_REQ__X_CONTENT_HDR_CHK(x)                            \
    HTTPD_CONF_REQ__X_HDR_CHK(req-> x ## _vs1, req-> x ## _pos, req-> x ## _len)

static void httpd__conf_req_reset_expires(struct Httpd_req_data *req,
                                          time_t now)
{
  if (vstr_sub_cstr_buf(req->xtra_content, req->expires_pos, req->expires_len,
                        date_rfc1123(now)))
    req->expires_len = vstr_sc_posdiff(req->expires_pos,req->xtra_content->len);
}


static int httpd__conf_req_d1(Httpd_opts *httpd_opts,
                              struct Con *con, struct Httpd_req_data *req,
                              const Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  if (!conf_token_list_num(token, token->depth_num))
    return (FALSE);
  conf_parse_token(conf, token);

  if (0) { }
  else if (OPT_SERV_SYM_EQ("return"))
  {
    unsigned int code = 0;
    
    if (conf_sc_token_parse_uint(conf, token, &code))
      return (FALSE);
    req->user_return_error_code = TRUE;
    switch (code)
    {
      case 301: if (!req->error_code && req->direct_uri) HTTPD_ERR_301(req);
      case 302: if (!req->error_code && req->direct_uri) HTTPD_ERR_302(req);
      case 303: if (!req->error_code && req->direct_uri) HTTPD_ERR_303(req);
      case 307: if (!req->error_code && req->direct_uri) HTTPD_ERR_307(req); 
                if (!req->error_code) req->user_return_error_code = FALSE;
                return (FALSE);
        
      case 400: HTTPD_ERR_RET(req, 400, FALSE);
      case 403: HTTPD_ERR_RET(req, 403, FALSE);
      case 404: HTTPD_ERR_RET(req, 404, FALSE);
      case 410: HTTPD_ERR_RET(req, 410, FALSE);
      case 503: HTTPD_ERR_RET(req, 503, FALSE);
      default:  req->user_return_error_code = FALSE;
      case 500: HTTPD_ERR_RET(req, 500, FALSE);
    }
  }
  else if (OPT_SERV_SYM_EQ("Location:"))
  {
    if (req->direct_filename) return (FALSE);
    
    req->direct_uri = TRUE;
    OPT_SERV_X_VSTR(req->fname);
    HTTPD_CONF_REQ__X_HDR_CHK(req->fname, 1, req->fname->len);
    httpd_req_absolute_uri(httpd_opts, con, req, req->fname);
  }
  else if (OPT_SERV_SYM_EQ("Cache-Control:"))
  { 
    HTTPD_CONF_REQ__X_CONTENT_VSTR(cache_control);
    HTTPD_CONF_REQ__X_CONTENT_HDR_CHK(cache_control);
  }
  else if (OPT_SERV_SYM_EQ("Content-Location:"))
  { 
    HTTPD_CONF_REQ__X_CONTENT_VSTR(content_location);
    HTTPD_CONF_REQ__X_CONTENT_HDR_CHK(content_location);
  }
  else if (OPT_SERV_SYM_EQ("Content-MD5:"))
  { 
    HTTPD_CONF_REQ__X_CONTENT_VSTR(content_md5);
    HTTPD_CONF_REQ__X_CONTENT_HDR_CHK(content_md5);
  }
  else if (OPT_SERV_SYM_EQ("Content-Type:"))
  {  /* FIXME: needs to work with Accept: */
    HTTPD_CONF_REQ__X_CONTENT_VSTR(content_type);
    HTTPD_CONF_REQ__X_CONTENT_HDR_CHK(content_type);
  }
  else if (OPT_SERV_SYM_EQ("Expires:"))
  {
    HTTPD_CONF_REQ__X_CONTENT_VSTR(expires);
    HTTPD_CONF_REQ__X_CONTENT_HDR_CHK(expires);

    /* note that rfc2616 says only go upto a year into the future */
    if (vstr_cmp_cstr_eq(req->expires_vs1, req->expires_pos, req->expires_len,
                         "<never>"))
      httpd__conf_req_reset_expires(req, req->now + (60 * 60 * 24 * 365));
    else
      if (vstr_cmp_cstr_eq(req->expires_vs1, req->expires_pos, req->expires_len,
                           "<now>"))
        httpd__conf_req_reset_expires(req, req->now);
  }
  /*  else if (OPT_SERV_SYM_EQ("Etag:"))
      X_CONTENT_VSTR(etag); -- needs server support */
  else if (OPT_SERV_SYM_EQ("filename"))
  {
    size_t pos = vstr_srch_chr_rev(req->fname, 1, req->fname->len, '/');
    ASSERT(pos); ++pos;
    OPT_SERV_X__VSTR(req->fname, pos, vstr_sc_posdiff(pos, req->fname->len));
    HTTPD_CONF_REQ__X_HDR_CHK(req->fname, 1, req->fname->len); /* ignore NIL */
  }
  else if (OPT_SERV_SYM_EQ("filepath"))
  {
    if (req->direct_uri) return (FALSE);
    
    req->direct_filename = TRUE;
    OPT_SERV_X_VSTR(req->fname);
    HTTPD_CONF_REQ__X_HDR_CHK(req->fname, 1, req->fname->len); /* ignore NIL */
  }
  else if (OPT_SERV_SYM_EQ("parse-accept"))
    OPT_SERV_X_TOGGLE(req->parse_accept);
  else if (OPT_SERV_SYM_EQ("allow-accept-encoding"))
    OPT_SERV_X_TOGGLE(req->allow_accept_encoding);
  else
    return (FALSE);
  
  return (TRUE);
}

static int httpd__conf_req(Httpd_opts *httpd_opts,
                           struct Con *con, struct Httpd_req_data *req,
                           const Conf_parse *conf, Conf_token *token)
{
  unsigned int cur_depth = token->depth_num;
  
  if (!OPT_SERV_SYM_EQ("org.and.jhttpd-conf-req-1.0"))
    return (FALSE);
  
  while (conf_token_list_num(token, cur_depth))
  {
    conf_parse_token(conf, token);
    if (!httpd__conf_req_d1(httpd_opts, con, req, conf, token))
      return (FALSE);
  }
  
  return (TRUE);
}

int httpd_conf_req_parse_file(Vstr_base *out, Httpd_opts *httpd_opts,
                              struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *s1 = NULL;
  Vstr_base *dir   = httpd_opts->req_configuration_dir;
  Vstr_base *fname = req->fname;
  const char *fname_cstr = NULL;
  Conf_parse conf[1]  = {CONF_PARSE_INIT};
  Conf_token token[1] = {CONF_TOKEN_INIT};
  int fd = -1;
  struct stat64 cf_stat[1];

  if (!dir->len)
    return (TRUE);
  
  if (!(s1 = vstr_make_base(NULL)))
    goto base_malloc_fail;

  vstr_add_vstr(s1, s1->len, dir, 1, dir->len, VSTR_TYPE_ADD_BUF_PTR);
  ASSERT((fname->len >= 1) && vstr_cmp_cstr_eq(fname, 1, 1, "/"));
  vstr_add_vstr(s1, s1->len, fname, 1, fname->len, VSTR_TYPE_ADD_BUF_PTR);

  if (s1->conf->malloc_bad ||
      !(fname_cstr = vstr_export_cstr_ptr(s1, 1, s1->len)))
    goto read_malloc_fail;
  
  if ((fd = io_open_nonblock(fname_cstr)) == -1)
    goto fin_ok; /* ignore missing req config file */
  vstr_del(s1, 1, s1->len); fname_cstr = NULL;

  if ((fstat64(fd, cf_stat) == -1) ||
      S_ISDIR(cf_stat->st_mode) ||
      (cf_stat->st_size < strlen("org.and.jhttpd-conf-req-1.0")) ||
      (cf_stat->st_size > (16 * 1024)))
  {
    close(fd);
    goto fin_ok; /* ignore */
  }

  conf->data = s1;
  while (cf_stat->st_size > s1->len)
  {
    size_t len = cf_stat->st_size - s1->len;
    
    if (!vstr_sc_read_len_fd(s1, s1->len, fd, len, NULL))
    {
      close(fd);
      goto read_malloc_fail;
    }
  }
  
  close(fd);
    
  if (!(conf->sects = vstr_sects_make(2)))
    goto sects_malloc_fail;

  if (!conf_parse_lex(conf))
    goto conf_fail;

  while (conf_parse_token(conf, token))
  {
    if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
      goto conf_fail;

    if (!conf_parse_token(conf, token))
      goto conf_fail;
    
    if (!OPT_SERV_SYM_EQ("org.and.jhttpd-conf-req-1.0"))
      goto conf_fail;
  
    if (!httpd__conf_req(httpd_opts, con, req, conf, token))
      goto conf_fail;
  }

 fin_ok:
  vstr_sects_free(conf->sects);
  vstr_free_base(s1);
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, "<vstr>", conf, token);
 sects_malloc_fail:
  vstr_sects_free(conf->sects);
 read_malloc_fail:
  vstr_free_base(s1);
 base_malloc_fail:
  if (!req->error_code)
    HTTPD_ERR(req, 500);
  return (FALSE);
}
