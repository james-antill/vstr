
#include "httpd.h"
#include "httpd_policy.h"

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
      if ((token->type == CONF_TOKEN_TYPE_QUOTE_ESC_D) ||               \
          (token->type == CONF_TOKEN_TYPE_QUOTE_ESC_DDD) ||             \
          (token->type == CONF_TOKEN_TYPE_QUOTE_ESC_S) ||               \
          (token->type == CONF_TOKEN_TYPE_QUOTE_ESC_SSS) ||             \
          FALSE)                                                        \
        if (!req-> x ## _vs1 ||                                         \
            !conf_sc_conv_unesc(req->xtra_content,                      \
                                req-> x ## _pos,                        \
                                req-> x ## _len, &req-> x ## _len))     \
          return (FALSE);                                               \
    } while (FALSE)


#define HTTPD_CONF_REQ__X_HDR_CHK(x, y, z) do {                 \
      if (req->policy->chk_hdr_split &&                       \
          vstr_srch_cstr_chrs_fwd(x, y, z, HTTP_EOL))           \
        return (FALSE);                                         \
      if (req->policy->chk_hdr_nil &&                           \
          vstr_srch_chr_fwd(x, y, z, 0))                        \
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

static int httpd__build_path(struct Con *con, struct Httpd_req_data *req,
                             const Conf_parse *conf, Conf_token *token,
                             Vstr_base *s1, size_t pos, size_t len,
                             unsigned int lim, int full, Vstr_ref *ref,
                             int *altered)
{
  size_t vhost_prefix_len = req->vhost_prefix_len;
    
  if (full && vhost_prefix_len)
  { /* don't want the vhost data */
    if (s1 == req->fname)
    {
      ASSERT((pos == 1) && (len == req->fname->len));
      len -= vhost_prefix_len;
    }
    vstr_del(req->fname, 1, vhost_prefix_len);
    req->vhost_prefix_len = vhost_prefix_len = 0;
  }
  req->skip_document_root = full;

  *altered = FALSE;
  if (!httpd_policy_path_lim_eq(s1, &pos, &len, lim, vhost_prefix_len, ref))
  {
    conf_parse_end_token(conf, token, conf_token_at_depth(token));
    return (TRUE);
  }
  *altered = TRUE;
  
  if (token->type != CONF_TOKEN_TYPE_CLIST)
  {
    const Vstr_sect_node *pv = conf_token_value(token);
      
    if (!pv || !vstr_sub_vstr(s1, pos, len, conf->data, pv->pos, pv->len,
                              VSTR_TYPE_SUB_BUF_REF))
      return (FALSE);
    OPT_SERV_X__ESC_VSTR(s1, pos, pv->len);

    goto fin_overwrite;
  }    

  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
  
  if (0) { }
  
  else if (OPT_SERV_SYM_EQ("assign") || OPT_SERV_SYM_EQ("="))
  {
    if (!httpd_policy_build_path(con, req, conf, token, NULL, NULL))
      return (FALSE);

    if (!conf->tmp->conf->malloc_bad &&
        !vstr_sub_vstr(s1, pos, len,
                       conf->tmp, 1, conf->tmp->len, VSTR_TYPE_SUB_BUF_REF))
      return (FALSE);
  }
  else if (OPT_SERV_SYM_EQ("append") || OPT_SERV_SYM_EQ("+="))
  {
    if (!httpd_policy_build_path(con, req, conf, token, NULL, NULL))
      return (FALSE);

    return (vstr_add_vstr(s1, vstr_sc_poslast(pos, len),
                          conf->tmp, 1, conf->tmp->len, VSTR_TYPE_ADD_BUF_REF));
  }
  else
    return (FALSE);

 fin_overwrite:
  if ((lim == HTTPD_POLICY_PATH_LIM_NONE) ||
      (lim == HTTPD_POLICY_PATH_LIM_PATH_FULL) ||
      (lim == HTTPD_POLICY_PATH_LIM_PATH_BEG) ||
      (lim == HTTPD_POLICY_PATH_LIM_PATH_EQ))
    req->vhost_prefix_len = 0; /* replaced everything */

  return (TRUE);
}

static int httpd__meta_build_path(struct Con *con, Httpd_req_data *req,
                                  Conf_parse *conf, Conf_token *token,
                                  int *full,
                                  unsigned int *lim, Vstr_ref **ret_ref)
{
  unsigned int depth = token->depth_num;

  ASSERT(ret_ref && !*ret_ref);
  
  if (token->type != CONF_TOKEN_TYPE_SLIST)
    return (TRUE);

  while (conf_token_list_num(token, depth))
  {
    CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, TRUE);
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

    if (0) { }
    else if (OPT_SERV_SYM_EQ("skip-document-root"))
      OPT_SERV_X_TOGGLE(*full);
    else if (OPT_SERV_SYM_EQ("limit"))
    {
      CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

      vstr_ref_del(*ret_ref); *ret_ref = NULL;
      if (token->type >= CONF_TOKEN_TYPE_USER_BEG)
      {
        unsigned int type = token->type - CONF_TOKEN_TYPE_USER_BEG;
        Vstr_ref *ref = conf_token_get_user_value(conf, token);
    
        switch (type)
        {
          case HTTPD_POLICY_REQ_PATH_BEG:
          case HTTPD_POLICY_REQ_PATH_END:
          case HTTPD_POLICY_REQ_PATH_EQ:

          case HTTPD_POLICY_REQ_NAME_BEG:
          case HTTPD_POLICY_REQ_NAME_END:
          case HTTPD_POLICY_REQ_NAME_EQ:
            
          case HTTPD_POLICY_REQ_BNWE_BEG:
          case HTTPD_POLICY_REQ_BNWE_END:
          case HTTPD_POLICY_REQ_BNWE_EQ:
            
          case HTTPD_POLICY_REQ_EXTN_BEG:
          case HTTPD_POLICY_REQ_EXTN_END:
          case HTTPD_POLICY_REQ_EXTN_EQ:
            *lim = httpd_policy_path_req2lim(type);
            break;

          default:
            vstr_ref_del(ref);
            return (FALSE);
        }
        *ret_ref = ref;
        conf_parse_end_token(conf, token, token->depth_num);
      }
  
      else if (OPT_SERV_SYM_EQ("<none>"))
        *lim = HTTPD_POLICY_PATH_LIM_NONE;
      else if (OPT_SERV_SYM_EQ("<path>"))
        *lim = HTTPD_POLICY_PATH_LIM_PATH_FULL;
      else if (OPT_SERV_SYM_EQ("<basename>"))
        *lim = HTTPD_POLICY_PATH_LIM_NAME_FULL;
      else if (OPT_SERV_SYM_EQ("<extension>"))
        *lim = HTTPD_POLICY_PATH_LIM_EXTN_FULL;
      else if (OPT_SERV_SYM_EQ("<basename-without-extension>"))
        *lim = HTTPD_POLICY_PATH_LIM_BNWE_FULL;
      else
      {
        unsigned int type = HTTPD_POLICY_REQ_PATH_BEG;
        
        if (0) { }
        else if (OPT_SERV_SYM_EQ("<path>-beg"))
          type = HTTPD_POLICY_REQ_PATH_BEG;
        else if (OPT_SERV_SYM_EQ("<path>-end"))
          type = HTTPD_POLICY_REQ_PATH_END;
        else if (OPT_SERV_SYM_EQ("<path>-eq"))
          type = HTTPD_POLICY_REQ_PATH_EQ;
        else if (OPT_SERV_SYM_EQ("<basename>-beg"))
          type = HTTPD_POLICY_REQ_NAME_BEG;
        else if (OPT_SERV_SYM_EQ("<basename>-end"))
          type = HTTPD_POLICY_REQ_NAME_END;
        else if (OPT_SERV_SYM_EQ("<basename>-eq"))
          type = HTTPD_POLICY_REQ_NAME_EQ;
        else if (OPT_SERV_SYM_EQ("<extension>-beg"))
          type = HTTPD_POLICY_REQ_EXTN_BEG;
        else if (OPT_SERV_SYM_EQ("<extension>-end"))
          type = HTTPD_POLICY_REQ_EXTN_END;
        else if (OPT_SERV_SYM_EQ("<extension>-eq"))
          type = HTTPD_POLICY_REQ_EXTN_EQ;
        else if (OPT_SERV_SYM_EQ("<basename-without-extension>-beg"))
          type = HTTPD_POLICY_REQ_BNWE_BEG;
        else if (OPT_SERV_SYM_EQ("<basename-without-extension>-end"))
          type = HTTPD_POLICY_REQ_BNWE_END;
        else if (OPT_SERV_SYM_EQ("<basename-without-extension>-eq"))
          type = HTTPD_POLICY_REQ_BNWE_EQ;
        else
          return (FALSE);
        
        if (!httpd_policy_path_make(con, req, conf, token, type, ret_ref))
          return (FALSE);
        *lim = httpd_policy_path_req2lim(type);
      }
    }
    else
      return (FALSE);
  }
  
  CONF_SC_PARSE_DEPTH_TOKEN_RET(conf, token, depth - 1, FALSE);
  return (TRUE);
}

static int httpd__conf_req_d1(struct Con *con, struct Httpd_req_data *req,
                              Conf_parse *conf, Conf_token *token)
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
    unsigned int lim = HTTPD_POLICY_PATH_LIM_NONE;
    Vstr_ref *ref = NULL;
    int altered = FALSE;
    
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!httpd__meta_build_path(con, req, conf, token, NULL, &lim, &ref))
      return (FALSE);
    if (!httpd__build_path(con, req, conf, token,
                           req->fname, 1, req->fname->len,
                           lim, TRUE, ref, &altered))
      return (FALSE);
    if (!altered)
      return (TRUE);
    
    req->direct_uri      = TRUE;
    req->direct_filename = FALSE;

    httpd_req_absolute_uri(con, req, req->fname, 1, req->fname->len);
    HTTPD_CONF_REQ__X_HDR_CHK(req->fname, 1, req->fname->len);
  }
  else if (OPT_SERV_SYM_EQ("Cache-Control:"))
  { 
    HTTPD_CONF_REQ__X_CONTENT_VSTR(cache_control);
    HTTPD_CONF_REQ__X_CONTENT_HDR_CHK(cache_control);
  }
  else if (OPT_SERV_SYM_EQ("Content-Location:"))
  {
    unsigned int lim = HTTPD_POLICY_PATH_LIM_NONE;
    size_t pos = 0;
    size_t len = 0;
    size_t fname_pos = 1;
    size_t fname_len = req->fname->len;
    Vstr_ref *ref = NULL;
    int altered = FALSE;
    
    fname_len -= req->vhost_prefix_len;
    fname_pos += req->vhost_prefix_len;
    
    if (!req->xtra_content && !(req->xtra_content = vstr_make_base(NULL)))
      return (FALSE);
    pos = req->xtra_content->len + 1;
    HTTPD_APP_REF_VSTR(req->xtra_content, req->fname, fname_pos, fname_len);

    len = vstr_sc_posdiff(pos, req->xtra_content->len);
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!httpd__meta_build_path(con, req, conf, token, NULL, &lim, &ref))
      return (FALSE);
    if (!httpd__build_path(con, req, conf, token, req->xtra_content, pos, len,
                           lim, FALSE, ref, &altered))
      return (FALSE);
    if (!altered)
      return (TRUE);
    
    len = vstr_sc_posdiff(pos, req->xtra_content->len);
    httpd_req_absolute_uri(con, req, req->xtra_content, pos, len);
    len = vstr_sc_posdiff(pos, req->xtra_content->len);
    
    req->content_location_vs1 = req->xtra_content;
    req->content_location_pos = pos;
    req->content_location_len = len;
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
    int full = req->skip_document_root;
    unsigned int lim = HTTPD_POLICY_PATH_LIM_NAME_FULL;
    Vstr_ref *ref = NULL;
    int altered = FALSE;
    
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!httpd__meta_build_path(con, req, conf, token, &full, &lim, &ref))
      return (FALSE);
    if ((lim != HTTPD_POLICY_PATH_LIM_NONE) && req->direct_uri)
    {
      vstr_ref_del(ref);
      return (FALSE);
    }
    if (!httpd__build_path(con, req, conf, token,
                           req->fname, 1, req->fname->len,
                           lim, full, ref, &altered))
      return (FALSE);
    if (!altered)
      return (TRUE);
    
    req->direct_uri      = FALSE;
    req->direct_filename = TRUE;
  }
  else if (OPT_SERV_SYM_EQ("parse-accept"))
    OPT_SERV_X_TOGGLE(req->parse_accept);
  else if (OPT_SERV_SYM_EQ("allow-accept-encoding"))
    OPT_SERV_X_TOGGLE(req->allow_accept_encoding);
  else if (OPT_SERV_SYM_EQ("Vary:_*"))
    OPT_SERV_X_TOGGLE(req->vary_star);
  else if (OPT_SERV_SYM_EQ("Vary:_Allow-Charset"))
    OPT_SERV_X_TOGGLE(req->vary_ac);
  else if (OPT_SERV_SYM_EQ("Vary:_Allow-Language"))
    OPT_SERV_X_TOGGLE(req->vary_al);
  else if (OPT_SERV_SYM_EQ("Vary:_Referer") ||
           OPT_SERV_SYM_EQ("Vary:_Referrer"))
    OPT_SERV_X_TOGGLE(req->vary_rf);
  else if (OPT_SERV_SYM_EQ("Vary:_User-Agent"))
    OPT_SERV_X_TOGGLE(req->vary_ua);
  else if (OPT_SERV_SYM_EQ("Accept:"))
  {
    /* foreach
       unsigned int qual = http_parse_accept(req, s1, pos, len);
       find highest
       do the right thing */
    return (FALSE);
  }
  else if (OPT_SERV_SYM_EQ("Accept-Charset:"))
    return (FALSE);
  else if (OPT_SERV_SYM_EQ("Accept-Language:"))
    return (FALSE);
  else
    return (FALSE);
  
  return (TRUE);
}

int httpd_conf_req_d0(struct Con *con, Httpd_req_data *req,
                      Conf_parse *conf, Conf_token *token)
{
  unsigned int cur_depth = token->depth_num;
  
  if (!OPT_SERV_SYM_EQ("org.and.jhttpd-conf-req-1.0"))
    return (FALSE);
  
  while (conf_token_list_num(token, cur_depth))
  {
    conf_parse_token(conf, token);
    if (!httpd__conf_req_d1(con, req, conf, token))
      return (FALSE);
  }
  
  return (TRUE);
}

int httpd_conf_req_parse_file(Conf_parse *conf,
                              struct Con *con, Httpd_req_data *req)
{
  Conf_token token[1] = {CONF_TOKEN_INIT};
  Vstr_base *dir   = req->policy->req_configuration_dir;
  Vstr_base *fname = req->fname;
  Vstr_base *s1 = NULL;
  const char *fname_cstr = NULL;
  int fd = -1;
  struct stat64 cf_stat[1];

  ASSERT(conf && con && req && dir && fname && fname->len);
  
  if (!dir->len ||
      !req->policy->use_req_configuration || req->skip_document_root)
    return (TRUE);
  
  s1 = conf->data;
  HTTPD_APP_REF_VSTR(s1, dir, 1, dir->len);
  ASSERT((fname->len >= 1) && vstr_cmp_cstr_eq(fname, 1, 1, "/"));
  HTTPD_APP_REF_VSTR(s1, fname, 1, fname->len);

  if (s1->conf->malloc_bad ||
      !(fname_cstr = vstr_export_cstr_ptr(s1, 1, s1->len)))
    goto read_malloc_fail;
  
  if ((fd = io_open_nonblock(fname_cstr)) == -1)
    goto fin_ok; /* ignore missing req config file */
  vstr_del(s1, 1, s1->len); fname_cstr = NULL;

  if ((fstat64(fd, cf_stat) == -1) ||
      S_ISDIR(cf_stat->st_mode) ||
      (cf_stat->st_size < strlen("org.and.jhttpd-conf-req-1.0")) ||
      (cf_stat->st_size > req->policy->max_req_conf_sz))
  {
    close(fd);
    goto fin_ok; /* ignore */
  }

  conf->data = s1;
  while (cf_stat->st_size > s1->len)
  {
    size_t len = cf_stat->st_size - s1->len;
    
    if (!vstr_sc_read_len_fd(s1, s1->len, fd, len, NULL) ||
        (len == (cf_stat->st_size - s1->len)))
    {
      close(fd);
      goto read_malloc_fail;
    }
  }
  
  close(fd);
    
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
  
    if (!httpd_conf_req_d0(con, req, conf, token))
      goto conf_fail;
  }

  /* And they all live together ... dum dum */
  if (conf->data->conf->malloc_bad)
    goto read_malloc_fail;
  
 fin_ok:
  return (TRUE);
  
 conf_fail:
  vstr_del(conf->tmp, 1, conf->tmp->len);
  if (!req->error_code)
    conf_parse_backtrace(conf->tmp, "<conf-request>", conf, token);
 read_malloc_fail:
  if (!req->error_code)
    HTTPD_ERR(req, 500);
  return (FALSE);
}
