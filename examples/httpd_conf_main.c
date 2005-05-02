
#include "httpd.h"
#include "httpd_policy.h"

#define EX_UTILS_NO_FUNCS 1
#include "ex_utils.h"

#include "mk.h"

#define HTTPD_CONF_MAIN_MATCH_CON 1
#define HTTPD_CONF_MAIN_MATCH_REQ 2

#define HTTPD_POLICY_CON_POLICY 3
#define HTTPD_POLICY_REQ_POLICY 4

#define HTTPD_POLICY_CLIENT_IPV4_CIDR_EQ 5
#define HTTPD_POLICY_SERVER_IPV4_CIDR_EQ 6

static int httpd__policy_connection_tst_d1(struct Con *con,
                                           Conf_parse *conf, Conf_token *token,
                                           int *matches)
{
  ASSERT(matches);
  
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

  if (token->type >= CONF_TOKEN_TYPE_USER_BEG)
  {
    unsigned int type = token->type - CONF_TOKEN_TYPE_USER_BEG;
    Vstr_ref *ref = conf_token_get_user_value(conf, token);
    
    switch (type)
    {
      case HTTPD_POLICY_CLIENT_IPV4_CIDR_EQ:
      {
        struct sockaddr *sa = EVNT_SA(con->evnt);
        *matches = httpd_policy_ipv4_cidr_eq(con, NULL, ref->ptr, sa);
      }
      break;
        
      case HTTPD_POLICY_SERVER_IPV4_CIDR_EQ:
      {
        struct Acpt_data *acpt_data = con->acpt_sa_ref->ptr;
        struct sockaddr *sa = (struct sockaddr *)acpt_data->sa;
        *matches = httpd_policy_ipv4_cidr_eq(con, NULL, ref->ptr, sa);
      }
      break;
        
      default:
        vstr_ref_del(ref);
        return (FALSE);
    }

    vstr_ref_del(ref);
    conf_parse_end_token(conf, token, token->depth_num);
  }
  
  else if (OPT_SERV_SYM_EQ("not") || OPT_SERV_SYM_EQ("!"))
  {
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!httpd__policy_connection_tst_d1(con, conf, token, matches))
      return (FALSE);
    *matches = !*matches;
  }
  else if (OPT_SERV_SYM_EQ("or") || OPT_SERV_SYM_EQ("||"))
  {
    unsigned int depth = token->depth_num;

    CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
    ++depth;
    while (conf_token_list_num(token, depth))
    {
      int or_matches = TRUE;
    
      CONF_SC_PARSE_DEPTH_TOKEN_RET(conf, token, depth, FALSE);

      if (!httpd__policy_connection_tst_d1(con, conf, token, &or_matches))
        return (FALSE);

      if (or_matches)
      {
        conf_parse_end_token(conf, token, depth);
        return (TRUE);
      }
    }

    *matches = FALSE;
  }
  else if (OPT_SERV_SYM_EQ("policy-eq"))
    return (httpd_policy_name_eq(conf, token, con->policy, matches));
  else if (OPT_SERV_SYM_EQ("client-ipv4-cidr-eq"))
    return (httpd_policy_ipv4_make(con, NULL, conf, token,
                                   HTTPD_POLICY_CLIENT_IPV4_CIDR_EQ,
                                   EVNT_SA(con->evnt), matches));
  else if (OPT_SERV_SYM_EQ("server-ipv4-cidr-eq"))
  {
    struct Acpt_data *acpt_data = con->acpt_sa_ref->ptr;
    return (httpd_policy_ipv4_make(con, NULL, conf, token,
                                   HTTPD_POLICY_SERVER_IPV4_CIDR_EQ,
                                   (struct sockaddr *)acpt_data->sa, matches));
  }
  
  else
    return (FALSE);

  return (TRUE);
}

static int httpd__policy_connection_d1(struct Con *con,
                                       Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
  {
    if (OPT_SERV_SYM_EQ("<close>"))
    {
      evnt_close(con->evnt);
      return (TRUE);
    }
    
    return (FALSE);
  }
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

  if (token->type >= CONF_TOKEN_TYPE_USER_BEG)
  {
    unsigned int type = token->type - CONF_TOKEN_TYPE_USER_BEG;
    Vstr_ref *ref = conf_token_get_user_value(conf, token);

    switch (type)
    {
      case HTTPD_POLICY_CON_POLICY:
        httpd_policy_change_con(con, ref->ptr);
        break;
        
      default:
        vstr_ref_del(ref);
        return (FALSE);
    }

    vstr_ref_del(ref);
    conf_parse_end_token(conf, token, token->depth_num);
  }
  
  else if (OPT_SERV_SYM_EQ("policy"))
  {
    Httpd_policy_opts *policy = NULL;
    Vstr_ref *ref = NULL;
    Conf_token save;

    save = *token;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!(policy = httpd_policy_find(con->policy, conf, token)))
      return (FALSE);

    ref = policy->ref;
    if (!conf_token_set_user_value(conf, &save, HTTPD_POLICY_CON_POLICY, ref))
      return (FALSE);
    
    httpd_policy_change_con(con, policy);
  }
  else if (OPT_SERV_SYM_EQ("Vary:_*"))
    OPT_SERV_X_TOGGLE(con->vary_star);
  
  else
    return (FALSE);
  
  return (TRUE);
}

static int httpd__policy_connection_d0(struct Con *con,
                                       Conf_parse *conf, Conf_token *token)
{
  unsigned int depth = token->depth_num;
  int matches = TRUE;

  CONF_SC_PARSE_SLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
  ++depth;
  while (conf_token_list_num(token, depth))
  {
    CONF_SC_PARSE_DEPTH_TOKEN_RET(conf, token, depth, FALSE);

    if (!httpd__policy_connection_tst_d1(con, conf, token, &matches))
      return (FALSE);

    if (!matches)
      return (TRUE);
  }
  --depth;
  
  while (conf_token_list_num(token, depth))
  {
    CONF_SC_PARSE_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
    
    if (!httpd__policy_connection_d1(con, conf, token))
      return (FALSE);
    if (con->evnt->flag_q_closed) /* don't do anything else */
      return (TRUE);
  }

  return (TRUE);
}

int httpd_policy_connection(struct Con *con,
                            Conf_parse *conf, const Conf_token *beg_token)
{
  Conf_token token[1];
  unsigned int num = 0;
  Vstr_ref *ref = NULL;
  
  if (!beg_token->num) /* not been parsed */
    return (TRUE);
  
  *token = *beg_token;

  num = token->num;
  do {
    if (!conf_parse_num_token(conf, token, num))
      goto conf_fail;
    
    assert(token->type == (CONF_TOKEN_TYPE_USER_BEG+HTTPD_CONF_MAIN_MATCH_CON));

    if ((ref = conf_token_get_user_value(conf, token)))
      num = *(unsigned int *)ref->ptr;
    vstr_ref_del(ref);
    
    if (!httpd__policy_connection_d0(con, conf, token))
      goto conf_fail;

  } while (ref);

  vstr_del(conf->tmp, 1, conf->tmp->len);
  return (TRUE);

 conf_fail:
  vstr_del(conf->tmp, 1, conf->tmp->len);
  conf_parse_backtrace(conf->tmp, "<policy-connection>", conf, token);
  return (FALSE);
}

static int httpd__policy_request_tst_d1(struct Con *con,
                                        struct Httpd_req_data *req,
                                        Conf_parse *conf, Conf_token *token,
                                        int *matches)
{
  Vstr_base *http_data = con->evnt->io_r;
  
  ASSERT(matches);
  
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

  ASSERT(con);
  
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
        
      case HTTPD_POLICY_REQ_BWEN_BEG:
      case HTTPD_POLICY_REQ_BWEN_END:
      case HTTPD_POLICY_REQ_BWEN_EQ:
        
      case HTTPD_POLICY_REQ_BWES_BEG:
      case HTTPD_POLICY_REQ_BWES_END:
      case HTTPD_POLICY_REQ_BWES_EQ:
        
      case HTTPD_POLICY_REQ_EXTN_BEG:
      case HTTPD_POLICY_REQ_EXTN_END:
      case HTTPD_POLICY_REQ_EXTN_EQ:
        
      case HTTPD_POLICY_REQ_EXTS_BEG:
      case HTTPD_POLICY_REQ_EXTS_END:
      case HTTPD_POLICY_REQ_EXTS_EQ:
      {
        size_t pos = 1;
        size_t len = req->fname->len;
        unsigned int lim = httpd_policy_path_req2lim(type);

        vstr_ref_add(ref);
        *matches = httpd_policy_path_lim_eq(req->fname, &pos, &len, lim,
                                            req->vhost_prefix_len, ref);
      }
      break;
      
      case HTTPD_POLICY_CLIENT_IPV4_CIDR_EQ:
      {
        struct sockaddr *sa = EVNT_SA(con->evnt);
        *matches = httpd_policy_ipv4_cidr_eq(con, NULL, ref->ptr, sa);
      }
      break;
        
      case HTTPD_POLICY_SERVER_IPV4_CIDR_EQ:
      {
        struct Acpt_data *acpt_data = con->acpt_sa_ref->ptr;
        struct sockaddr *sa = (struct sockaddr *)acpt_data->sa;
        *matches = httpd_policy_ipv4_cidr_eq(con, NULL, ref->ptr, sa);
      }
        
      default:
        vstr_ref_del(ref);
        return (FALSE);
    }

    vstr_ref_del(ref);
    conf_parse_end_token(conf, token, token->depth_num);
  }

  else if (OPT_SERV_SYM_EQ("not") || OPT_SERV_SYM_EQ("!"))
  {
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!httpd__policy_request_tst_d1(con, req, conf, token, matches))
      return (FALSE);
    *matches = !*matches;
  }
  else if (OPT_SERV_SYM_EQ("or") || OPT_SERV_SYM_EQ("||"))
  {
    unsigned int depth = token->depth_num;

    CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
    ++depth;
    while (conf_token_list_num(token, depth))
    {
      int or_matches = TRUE;
    
      CONF_SC_PARSE_DEPTH_TOKEN_RET(conf, token, depth, FALSE);

      if (!httpd__policy_request_tst_d1(con, req, conf, token, &or_matches))
        return (FALSE);

      if (or_matches)
      {
        conf_parse_end_token(conf, token, depth);
        return (TRUE);
      }
    }

    *matches = FALSE;
  }
  else if (OPT_SERV_SYM_EQ("policy-eq"))
    return (httpd_policy_name_eq(conf, token, req->policy, matches));
  else if (OPT_SERV_SYM_EQ("client-ipv4-cidr-eq"))
    return (httpd_policy_ipv4_make(con, req, conf, token,
                                   HTTPD_POLICY_CLIENT_IPV4_CIDR_EQ,
                                   EVNT_SA(con->evnt), matches));
  else if (OPT_SERV_SYM_EQ("server-ipv4-cidr-eq"))
  {
    struct Acpt_data *acpt_data = con->acpt_sa_ref->ptr;
    return (httpd_policy_ipv4_make(con, req, conf, token,
                                   HTTPD_POLICY_SERVER_IPV4_CIDR_EQ,
                                   (struct sockaddr *)acpt_data->sa, matches));
  }
  else if (OPT_SERV_SYM_EQ("hostname-eq"))
  { /* doesn't do escaping because DNS is ASCII */
    Vstr_sect_node *h_h = req->http_hdrs->hdr_host;
    Vstr_base *d_h = req->policy->default_hostname;

    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (h_h->len)
      *matches = conf_token_cmp_case_val_eq(conf, token,
                                            http_data, h_h->pos, h_h->len);
    else
      *matches = conf_token_cmp_case_val_eq(conf, token,
                                            d_h, 1, d_h->len);
  }
  else if (OPT_SERV_SYM_EQ("user-agent-eq") || OPT_SERV_SYM_EQ("UA-eq"))
  { /* doesn't do escaping because URLs are ASCII */
    Vstr_sect_node *h_ua = req->http_hdrs->hdr_ua;

    req->vary_ua = TRUE;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    *matches = conf_token_cmp_val_eq(conf, token, http_data, 1, h_ua->len);
  }
  else if (OPT_SERV_SYM_EQ("user-agent-search-eq") ||
           OPT_SERV_SYM_EQ("user-agent-srch-eq") ||
           OPT_SERV_SYM_EQ("UA-srch-eq"))
  { /* doesn't do escaping because URLs are ASCII */
    Vstr_sect_node *h_ua = req->http_hdrs->hdr_ua;

    req->vary_ua = TRUE;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    *matches = !!conf_token_srch_val(conf, token, http_data, 1, h_ua->len);
  }
  else if (OPT_SERV_SYM_EQ("referrer-eq") || OPT_SERV_SYM_EQ("referer-eq"))
  { /* doesn't do escaping because URLs are ASCII */
    Vstr_sect_node *h_ref = req->http_hdrs->hdr_referer;

    req->vary_rf = TRUE;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    *matches = conf_token_cmp_case_val_eq(conf, token,
                                          http_data, 1, h_ref->len);
  }
  else if (OPT_SERV_SYM_EQ("referrer-beg") || OPT_SERV_SYM_EQ("referer-beg"))
  { /* doesn't do escaping because URLs are ASCII */
    Vstr_sect_node *h_ref = req->http_hdrs->hdr_referer;

    req->vary_rf = TRUE;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    *matches = conf_token_cmp_case_val_beg_eq(conf, token,
                                              http_data, 1, h_ref->len);
  }
  else if (OPT_SERV_SYM_EQ("referrer-search-eq") ||
           OPT_SERV_SYM_EQ("referrer-srch-eq") ||
           OPT_SERV_SYM_EQ("referer-search-eq") ||
           OPT_SERV_SYM_EQ("referer-srch-eq"))
  { /* doesn't do escaping because URLs are ASCII */
    Vstr_sect_node *h_ref = req->http_hdrs->hdr_referer;

    req->vary_rf = TRUE;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    *matches = !!conf_token_srch_val(conf, token, http_data, 1, h_ref->len);
  }
  else if (OPT_SERV_SYM_EQ("http-0.9-eq"))
    *matches =  req->ver_0_9;
  else if (OPT_SERV_SYM_EQ("http-1.0-eq"))
    *matches = !req->ver_0_9 && !req->ver_1_1;
  else if (OPT_SERV_SYM_EQ("http-1.1-eq"))
    *matches = !req->ver_0_9 &&  req->ver_1_1;
  else if (OPT_SERV_SYM_EQ("method-eq"))
  { /* doesn't do escaping because methods are ASCII */
    Vstr_sect_node *meth = VSTR_SECTS_NUM(req->sects, 1);
    
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    *matches = conf_token_cmp_case_val_eq(conf, token,
                                          http_data, meth->pos, meth->len);
  }
  else
  { /* spend time doing path/name/extn/bnwe */
    Vstr_ref *ref = NULL;
    unsigned int type = HTTPD_POLICY_REQ_PATH_BEG;
    unsigned int lim  = 0;
    size_t pos = 1;
    size_t len = req->fname->len;
    
    if (0) { }
    else if (OPT_SERV_SYM_EQ("path-beg"))
      type = HTTPD_POLICY_REQ_PATH_BEG;
    else if (OPT_SERV_SYM_EQ("path-end"))
      type = HTTPD_POLICY_REQ_PATH_END;
    else if (OPT_SERV_SYM_EQ("path-eq"))
      type = HTTPD_POLICY_REQ_PATH_EQ;
    else if (OPT_SERV_SYM_EQ("basename-beg"))
      type = HTTPD_POLICY_REQ_NAME_BEG;
    else if (OPT_SERV_SYM_EQ("basename-end"))
      type = HTTPD_POLICY_REQ_NAME_END;
    else if (OPT_SERV_SYM_EQ("basename-eq"))
      type = HTTPD_POLICY_REQ_NAME_EQ;
    else if (OPT_SERV_SYM_EQ("extension-beg"))
      type = HTTPD_POLICY_REQ_EXTN_BEG;
    else if (OPT_SERV_SYM_EQ("extension-end"))
      type = HTTPD_POLICY_REQ_EXTN_END;
    else if (OPT_SERV_SYM_EQ("extension-eq"))
      type = HTTPD_POLICY_REQ_EXTN_EQ;
    else if (OPT_SERV_SYM_EQ("extensions-beg"))
      type = HTTPD_POLICY_REQ_EXTS_BEG;
    else if (OPT_SERV_SYM_EQ("extensions-end"))
      type = HTTPD_POLICY_REQ_EXTS_END;
    else if (OPT_SERV_SYM_EQ("extensions-eq"))
      type = HTTPD_POLICY_REQ_EXTS_EQ;
    else if (OPT_SERV_SYM_EQ("basename-without-extension-beg"))
      type = HTTPD_POLICY_REQ_BWEN_BEG;
    else if (OPT_SERV_SYM_EQ("basename-without-extension-end"))
      type = HTTPD_POLICY_REQ_BWEN_END;
    else if (OPT_SERV_SYM_EQ("basename-without-extension-eq"))
      type = HTTPD_POLICY_REQ_BWEN_EQ;
    else if (OPT_SERV_SYM_EQ("basename-without-extensions-beg"))
      type = HTTPD_POLICY_REQ_BWES_BEG;
    else if (OPT_SERV_SYM_EQ("basename-without-extensions-end"))
      type = HTTPD_POLICY_REQ_BWES_END;
    else if (OPT_SERV_SYM_EQ("basename-without-extensions-eq"))
      type = HTTPD_POLICY_REQ_BWES_EQ;
    else
      return (FALSE);
    
    if (!httpd_policy_path_make(con, req, conf, token, type, &ref))
      return (FALSE);

    lim = httpd_policy_path_req2lim(type);
    *matches = httpd_policy_path_lim_eq(req->fname, &pos, &len, lim,
                                        req->vhost_prefix_len, ref);
  }

  return (TRUE);
}

static int httpd__policy_request_d1(struct Con *con, struct Httpd_req_data *req,
                                    Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
  {
    if (OPT_SERV_SYM_EQ("<close>"))
    {
      evnt_close(con->evnt);
      return (TRUE);
    }
    
    return (FALSE);
  }
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

  if (token->type >= CONF_TOKEN_TYPE_USER_BEG)
  {
    unsigned int type = token->type - CONF_TOKEN_TYPE_USER_BEG;
    Vstr_ref *ref = conf_token_get_user_value(conf, token);
    
    switch (type)
    {
      case HTTPD_POLICY_REQ_POLICY:
        httpd_policy_change_req(req, ref->ptr);
        break;
        
      default:
        vstr_ref_del(ref);
        return (FALSE);
    }

    vstr_ref_del(ref);
    conf_parse_end_token(conf, token, token->depth_num);
  }
  
  else if (OPT_SERV_SYM_EQ("policy"))
  {
    Httpd_policy_opts *policy = NULL;
    Vstr_ref *ref = NULL;
    Conf_token save;

    save = *token;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!(policy = httpd_policy_find(req->policy, conf, token)))
      return (FALSE);

    ref = policy->ref;
    if (!conf_token_set_user_value(conf, &save, HTTPD_POLICY_REQ_POLICY, ref))
      return (FALSE);
    
    httpd_policy_change_req(req, policy);
  }
  else if (OPT_SERV_SYM_EQ("org.and.jhttpd-conf-req-1.0"))
    return (httpd_conf_req_d0(con, req, /* server beg time is "close engouh" */
                              con->policy->beg->beg_time, conf, token));
  
  else
    return (FALSE);
  
  return (TRUE);
}

static int httpd__policy_request_d0(struct Con *con, struct Httpd_req_data *req,
                                    Conf_parse *conf, Conf_token *token)
{
  unsigned int depth = token->depth_num;
  int matches = TRUE;

  CONF_SC_PARSE_SLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
  ++depth;
  while (conf_token_list_num(token, depth))
  {
    CONF_SC_PARSE_DEPTH_TOKEN_RET(conf, token, depth, FALSE);

    if (!httpd__policy_request_tst_d1(con, req, conf, token, &matches))
      return (FALSE);

    if (!matches)
      return (TRUE);
  }
  --depth;
  
  while (conf_token_list_num(token, depth))
  {
    CONF_SC_PARSE_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
    
    if (!httpd__policy_request_d1(con, req, conf, token))
      return (FALSE);
    if (con->evnt->flag_q_closed) /* don't do anything else */
      return (TRUE);
  }

  return (TRUE);
}

int httpd_policy_request(struct Con *con, struct Httpd_req_data *req,
                         Conf_parse *conf, const Conf_token *beg_token)
{
  Conf_token token[1];
  unsigned int num = 0;
  Vstr_ref *ref = NULL;
  
  if (!beg_token->num) /* not been parsed */
    return (TRUE);
  
  *token = *beg_token;

  num = token->num;
  do {
    if (!conf_parse_num_token(conf, token, num))
      goto conf_fail;
    
    assert(token->type == (CONF_TOKEN_TYPE_USER_BEG+HTTPD_CONF_MAIN_MATCH_REQ));

    if ((ref = conf_token_get_user_value(conf, token)))
      num = *(unsigned int *)ref->ptr;
    vstr_ref_del(ref);

    if (!httpd__policy_request_d0(con, req, conf, token))
      goto conf_fail;
    
  } while (ref);

  vstr_del(conf->tmp, 1, conf->tmp->len);
  return (TRUE);

 conf_fail:
  vstr_del(conf->tmp, 1, conf->tmp->len);
  if (!req->error_code)
  {
    conf_parse_backtrace(conf->tmp, "<policy-request>", conf, token);
    HTTPD_ERR(req, 500);
  }
  return (FALSE);
}

/* NOTE: doesn't unlink */
static void httpd_conf_main_policy_free(Httpd_policy_opts *opts)
{
  if (!opts)
    return;

  vstr_free_base(opts->policy_name);       opts->policy_name             = NULL;
  vstr_free_base(opts->document_root);     opts->document_root           = NULL;
  vstr_free_base(opts->server_name);       opts->server_name             = NULL;
  vstr_free_base(opts->dir_filename);      opts->dir_filename            = NULL;
  vstr_free_base(opts->mime_types_def_ct); opts->mime_types_def_ct       = NULL;
  vstr_free_base(opts->mime_types_main);   opts->mime_types_main         = NULL;
  vstr_free_base(opts->mime_types_xtra);   opts->mime_types_xtra         = NULL;
  vstr_free_base(opts->default_hostname);  opts->default_hostname        = NULL;
  vstr_free_base(opts->req_conf_dir);      opts->req_conf_dir            = NULL;
  vstr_free_base(opts->auth_realm);        opts->auth_realm              = NULL;
  vstr_free_base(opts->auth_token);        opts->auth_token              = NULL;

  vstr_ref_del(opts->ref);
}

static int httpd_conf_main_policy_init(Httpd_opts *httpd_opts,
                                       Httpd_policy_opts *opts, Vstr_ref *ref)
{
  if (!ref)
    return (FALSE);

  opts->ref = NULL;
  opts->beg = httpd_opts;
  
  if (!(opts->policy_name       = vstr_make_base(NULL)) ||
      !(opts->document_root     = vstr_make_base(NULL)) ||
      !(opts->server_name       = vstr_make_base(NULL)) ||
      !(opts->dir_filename      = vstr_make_base(NULL)) ||
      !(opts->mime_types_def_ct = vstr_make_base(NULL)) ||
      !(opts->mime_types_main   = vstr_make_base(NULL)) ||
      !(opts->mime_types_xtra   = vstr_make_base(NULL)) ||
      !(opts->default_hostname  = vstr_make_base(NULL)) ||
      !(opts->req_conf_dir      = vstr_make_base(NULL)) ||
      !(opts->auth_realm        = vstr_make_base(NULL)) ||
      !(opts->auth_token        = vstr_make_base(NULL)) ||
      FALSE)
  {
    httpd_conf_main_policy_free(opts);
    return (FALSE);
  }
  
  opts->ref = vstr_ref_add(ref);
  
  return (TRUE);
}

#define HTTPD_CONF_MAIN_POLICY_CP_VSTR(x)                               \
    vstr_sub_vstr(dst-> x , 1, dst-> x ->len, src-> x , 1, src-> x ->len, 0)
#define HTTPD_CONF_MAIN_POLICY_CP_VAL(x)        \
    dst-> x = src-> x

static int httpd_conf_main_policy_copy(Httpd_policy_opts *dst,
                                       Httpd_policy_opts *src)
{
  HTTPD_CONF_MAIN_POLICY_CP_VSTR(document_root);
  HTTPD_CONF_MAIN_POLICY_CP_VSTR(server_name);
  HTTPD_CONF_MAIN_POLICY_CP_VSTR(dir_filename);

  HTTPD_CONF_MAIN_POLICY_CP_VSTR(mime_types_def_ct);
  HTTPD_CONF_MAIN_POLICY_CP_VSTR(mime_types_main);
  HTTPD_CONF_MAIN_POLICY_CP_VSTR(mime_types_xtra);
  HTTPD_CONF_MAIN_POLICY_CP_VSTR(default_hostname);
  HTTPD_CONF_MAIN_POLICY_CP_VSTR(req_conf_dir);

  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_mmap);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_sendfile);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_keep_alive);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_keep_alive_1_0);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_vhosts);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_range);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_range_1_0);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_public_only);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_gzip_content_replacement);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_err_406);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_canonize_host);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_host_err_400);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_chk_host_err);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(remove_url_frag);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(remove_url_query);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_posix_fadvise);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(use_req_conf);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(chk_hdr_split);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(chk_hdr_nil);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(chk_dot_dir);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(max_header_sz);
  HTTPD_CONF_MAIN_POLICY_CP_VAL(max_req_conf_sz);

  return (!dst->document_root->conf->malloc_bad);
}

static int httpd__conf_main_policy_d1(Httpd_policy_opts *opts,
                                      const Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

  if (0) { }
  else if (OPT_SERV_SYM_EQ("directory-filename"))
    OPT_SERV_X_VSTR(opts->dir_filename);
  else if (OPT_SERV_SYM_EQ("document-root") ||
           OPT_SERV_SYM_EQ("doc-root"))
    OPT_SERV_X_VSTR(opts->document_root);
  else if (OPT_SERV_SYM_EQ("unspecified-hostname"))
    OPT_SERV_X_VSTR(opts->default_hostname);
  else if (OPT_SERV_SYM_EQ("MIME/types-default-type"))
    OPT_SERV_X_VSTR(opts->mime_types_def_ct);
  else if (OPT_SERV_SYM_EQ("MIME/types-filename-main"))
    OPT_SERV_X_VSTR(opts->mime_types_main);
  else if (OPT_SERV_SYM_EQ("MIME/types-filename-extra") ||
           OPT_SERV_SYM_EQ("MIME/types-filename-xtra"))
    OPT_SERV_X_VSTR(opts->mime_types_xtra);
  else if (OPT_SERV_SYM_EQ("request-configuration-directory") ||
           OPT_SERV_SYM_EQ("req-conf-dir"))
    OPT_SERV_X_VSTR(opts->req_conf_dir);
  else if (OPT_SERV_SYM_EQ("server-name"))
    OPT_SERV_X_VSTR(opts->server_name);
  else if (OPT_SERV_SYM_EQ("authorization") || OPT_SERV_SYM_EQ("auth"))
  { /* token is output of: echo -n foo:bar | openssl enc -base64 */
    /* see it with: echo token | openssl enc -d -base64 && echo */
    unsigned int depth = token->depth_num;
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!OPT_SERV_SYM_EQ("basic-encoded")) return (FALSE);

    while (conf_token_list_num(token, depth))
    {
      CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
      CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
      
      if (0) { }
      else if (OPT_SERV_SYM_EQ("realm")) OPT_SERV_X_VSTR(opts->auth_realm);
      else if (OPT_SERV_SYM_EQ("token")) OPT_SERV_X_VSTR(opts->auth_token);
      else return (FALSE);
    }
  }

  else if (OPT_SERV_SYM_EQ("mmap"))
    OPT_SERV_X_TOGGLE(opts->use_mmap);
  else if (OPT_SERV_SYM_EQ("sendfile"))
    OPT_SERV_X_TOGGLE(opts->use_sendfile);
  else if (OPT_SERV_SYM_EQ("keep-alive"))
    OPT_SERV_X_TOGGLE(opts->use_keep_alive);
  else if (OPT_SERV_SYM_EQ("keep-alive-1.0"))
    OPT_SERV_X_TOGGLE(opts->use_keep_alive_1_0);
  else if (OPT_SERV_SYM_EQ("virtual-hosts") ||
           OPT_SERV_SYM_EQ("vhosts"))
    OPT_SERV_X_TOGGLE(opts->use_vhosts);
  else if (OPT_SERV_SYM_EQ("range"))
    OPT_SERV_X_TOGGLE(opts->use_range);
  else if (OPT_SERV_SYM_EQ("range-1.0"))
    OPT_SERV_X_TOGGLE(opts->use_range_1_0);
  else if (OPT_SERV_SYM_EQ("public-only"))
    OPT_SERV_X_TOGGLE(opts->use_public_only);
  else if (OPT_SERV_SYM_EQ("gzip-content-type-replacement"))
    OPT_SERV_X_TOGGLE(opts->use_gzip_content_replacement);
  else if (OPT_SERV_SYM_EQ("error-406"))
    OPT_SERV_X_TOGGLE(opts->use_err_406);
  else if (OPT_SERV_SYM_EQ("canonize-host"))
    OPT_SERV_X_TOGGLE(opts->use_canonize_host);
  else if (OPT_SERV_SYM_EQ("error-host-400"))
    OPT_SERV_X_TOGGLE(opts->use_host_err_400);
  else if (OPT_SERV_SYM_EQ("check-host"))
    OPT_SERV_X_TOGGLE(opts->use_chk_host_err);
  else if (OPT_SERV_SYM_EQ("posix-fadvise"))
    OPT_SERV_X_TOGGLE(opts->use_posix_fadvise);
  else if (OPT_SERV_SYM_EQ("allow-request-configuration"))
    OPT_SERV_X_TOGGLE(opts->use_req_conf);
  else if (OPT_SERV_SYM_EQ("check-header-splitting"))
    OPT_SERV_X_TOGGLE(opts->chk_hdr_split);
  else if (OPT_SERV_SYM_EQ("check-header-NIL"))
    OPT_SERV_X_TOGGLE(opts->chk_hdr_nil);
  else if (OPT_SERV_SYM_EQ("check-dot-directory") ||
           OPT_SERV_SYM_EQ("check-dot-dir") ||
           OPT_SERV_SYM_EQ("check-.-dir"))
    OPT_SERV_X_TOGGLE(opts->chk_dot_dir);
  else if (OPT_SERV_SYM_EQ("url-remove-fragment"))
    OPT_SERV_X_TOGGLE(opts->remove_url_frag);
  else if (OPT_SERV_SYM_EQ("url-remove-query"))
    OPT_SERV_X_TOGGLE(opts->remove_url_query);

  else if (OPT_SERV_SYM_EQ("max-header-size") ||
           OPT_SERV_SYM_EQ("max-header-sz"))
    OPT_SERV_X_UINT(opts->max_header_sz);
  else if (OPT_SERV_SYM_EQ("max-request-configuration-size") ||
           OPT_SERV_SYM_EQ("max-request-configuration-sz") ||
           OPT_SERV_SYM_EQ("max-req-conf-size") ||
           OPT_SERV_SYM_EQ("max-req-conf-sz"))
    OPT_SERV_X_UINT(opts->max_req_conf_sz);
  
  else
    return (FALSE);
  
  return (TRUE);
}

static int httpd__conf_main_policy(Httpd_opts *httpd_opts,
                                   const Conf_parse *conf, Conf_token *token)
{
  Httpd_policy_opts *opts = httpd_opts->def_policy;
  unsigned int cur_depth = token->depth_num;
  Conf_token save;
  const Vstr_sect_node *pv = NULL;

  /* name first */
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
  
  if (!(pv = conf_token_value(token)))
    return (FALSE);
      
  if (!conf_token_cmp_val_cstr_eq(conf, token, HTTPD_CONF_DEF_POLICY_NAME))
  {
    Httpd_policy_opts *nxt_opts = NULL;
    
    if (!(nxt_opts = httpd_policy_find(opts, conf, token)))
    {
      Vstr_base *s1 = NULL;
      
      if (!(nxt_opts = httpd_conf_main_policy_make(opts->beg)))
        return (FALSE);

      ASSERT(pv);
      s1 = nxt_opts->policy_name;
      if (!vstr_sub_vstr(s1, 1, s1->len, conf->data, pv->pos, pv->len,
                         VSTR_TYPE_SUB_BUF_REF))
        return (FALSE);
      OPT_SERV_X__ESC_VSTR(s1, 1, pv->len);
    }
    
    opts = nxt_opts;
  }

  save = *token;
  if (!conf_parse_token(conf, token) || (token->depth_num < cur_depth))
    return (TRUE);
  if (token->type != CONF_TOKEN_TYPE_SLIST)
    *token = save; /* restore ... */
  else
  { /* allow set of attributes */
    unsigned int depth = token->depth_num;

    while (conf_token_list_num(token, depth))
    {
      CONF_SC_PARSE_CLIST_DEPTH_TOKEN_RET(conf, token, depth, FALSE);
      CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

      if (0) { }
      else if (OPT_SERV_SYM_EQ("inherit"))
      {
        Httpd_policy_opts *frm_opts = NULL;
        CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
        if (!(frm_opts = httpd_policy_find(opts, conf, token)))
          return (FALSE);
        if (!httpd_conf_main_policy_copy(opts, frm_opts))
          return (FALSE);
      }
    
      else
        return (FALSE);
    }
  }
  
  while (conf_token_list_num(token, cur_depth))
  {
    conf_parse_token(conf, token);
    if (conf_token_at_depth(token) != cur_depth)
      return (FALSE);
    if (!httpd__conf_main_policy_d1(opts, conf, token))
      return (FALSE);
  }
  
  return (TRUE);
}

static int httpd__conf_main_d1(Httpd_opts *httpd_opts,
                               Conf_parse *conf, Conf_token *token)
{
  if (token->type != CONF_TOKEN_TYPE_CLIST)
    return (FALSE);
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

  if (OPT_SERV_SYM_EQ("org.and.daemon-conf-1.0"))
  {
    if (!opt_serv_conf(httpd_opts->s, conf, token))
      return (FALSE);
  }
  
  else if (OPT_SERV_SYM_EQ("policy"))
  {
    if (!httpd__conf_main_policy(httpd_opts, conf, token))
      return (FALSE);
  }
  
  else if (OPT_SERV_SYM_EQ("match-connection"))
  {
    if (!conf_token_set_user_value(conf, token,
                                   HTTPD_CONF_MAIN_MATCH_CON, NULL))
      return (FALSE);
    
    if (!httpd_opts->match_connection->num)
      *httpd_opts->match_connection = *token;
    else
    { /* already have one, add this to end... */
      Vstr_ref *ref = vstr_ref_make_malloc(sizeof(unsigned int));

      if (!ref)
        return (FALSE);
      *(unsigned int *)ref->ptr = token->num;
      conf_token_set_user_value(conf, httpd_opts->tmp_match_connection,
                                HTTPD_CONF_MAIN_MATCH_CON, ref);
      vstr_ref_del(ref);
    }
    *httpd_opts->tmp_match_connection = *token;
    
    conf_parse_end_token(conf, token, token->depth_num);
  }
  else if (OPT_SERV_SYM_EQ("match-request"))
  {
    if (!conf_token_set_user_value(conf, token,
                                   HTTPD_CONF_MAIN_MATCH_REQ, NULL))
      return (FALSE);
    
    if (!httpd_opts->match_request->num)
      *httpd_opts->match_request = *token;
    else
    { /* already have one, add this to end... */
      Vstr_ref *ref = vstr_ref_make_malloc(sizeof(unsigned int));

      if (!ref)
        return (FALSE);
      *(unsigned int *)ref->ptr = token->num;
      conf_token_set_user_value(conf, httpd_opts->tmp_match_request,
                                HTTPD_CONF_MAIN_MATCH_REQ, ref);
      vstr_ref_del(ref);
    }
    *httpd_opts->tmp_match_request = *token;
    
    conf_parse_end_token(conf, token, token->depth_num);
  }
  
  else
    return (FALSE);
  
  return (TRUE);
}

int httpd_conf_main(Httpd_opts *opts, Conf_parse *conf, Conf_token *token)
{
  unsigned int cur_depth = token->depth_num;
  
  if (!OPT_SERV_SYM_EQ("org.and.jhttpd-conf-main-1.0"))
    return (FALSE);
  
  while (conf_token_list_num(token, cur_depth))
  {
    conf_parse_token(conf, token);
    if (conf_token_at_depth(token) != cur_depth)
      return (FALSE);
    if (!httpd__conf_main_d1(opts, conf, token))
      return (FALSE);
  }
  
  /* And they all live together ... dum dum */
  if (conf->data->conf->malloc_bad)
    return (FALSE);
  
  return (TRUE);
}

#define HTTPD_CONF__BEG_APP() do {                                      \
      ASSERT(!opts->conf_num || (conf->state == CONF_PARSE_STATE_END)); \
      /* reinit */                                                      \
      conf->state = CONF_PARSE_STATE_BEG;                               \
    } while (FALSE)

/* restore the previous parsing we've done and skip parsing it again */
#define HTTPD_CONF__END_APP() do {                                      \
      if (opts->conf_num)                                               \
      {                                                                 \
        unsigned int tmp__conf_num = opts->conf_num;                    \
        while (tmp__conf_num--)                                         \
        {                                                               \
          if (!conf_parse_token(conf, token))                           \
            goto conf_fail;                                             \
                                                                        \
          if ((token->type != CONF_TOKEN_TYPE_CLIST) ||                 \
              (token->depth_num != 1))                                  \
            goto conf_fail;                                             \
          if (!conf_parse_end_token(conf, token, token->depth_num))     \
            goto conf_fail;                                             \
        }                                                               \
      }                                                                 \
    } while (FALSE)
    

int httpd_conf_main_parse_cstr(Vstr_base *out,
                               Httpd_opts *opts, const char *data)
{
  Conf_parse *conf    = opts->conf;
  Conf_token token[1] = {CONF_TOKEN_INIT};
  size_t pos = 1;
  size_t len = 0;

  ASSERT(opts && data);

  if (!conf && !(conf = conf_parse_make(NULL)))
    goto conf_malloc_fail;

  pos = conf->data->len + 1;
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len,
                         "(org.and.jhttpd-conf-main-1.0 "))
    goto read_malloc_fail;
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len, data))
    goto read_malloc_fail;
  if (!vstr_add_cstr_ptr(conf->data, conf->data->len,
                         ")"))
    goto read_malloc_fail;
  len = vstr_sc_posdiff(pos, conf->data->len);

  HTTPD_CONF__BEG_APP();
  
  if (!conf_parse_lex(conf, pos, len))
    goto conf_fail;

  HTTPD_CONF__END_APP();
  
  conf_parse_token(conf, token);
  
  if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
    goto conf_fail;

  if (!conf_parse_token(conf, token))
    goto conf_fail;
    
  ASSERT(OPT_SERV_SYM_EQ("org.and.jhttpd-conf-main-1.0"));
  
  if (!httpd_conf_main(opts, conf, token))
    goto conf_fail;

  if (token->num != conf->sects->num)
    goto conf_fail;

  opts->conf = conf;
  opts->conf_num++;
  
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, data, conf, token);
 read_malloc_fail:
  conf_parse_free(conf);
 conf_malloc_fail:
  return (FALSE);
}

int httpd_conf_main_parse_file(Vstr_base *out,
                               Httpd_opts *opts, const char *fname)
{
  Conf_parse *conf    = opts->conf;
  Conf_token token[1] = {CONF_TOKEN_INIT};
  size_t pos = 1;
  size_t len = 0;
  
  ASSERT(opts && fname);
  
  if (!conf && !(conf = conf_parse_make(NULL)))
    goto conf_malloc_fail;

  pos = conf->data->len + 1;
  if (!vstr_sc_read_len_file(conf->data, conf->data->len, fname, 0, 0, NULL))
    goto read_malloc_fail;
  len = vstr_sc_posdiff(pos, conf->data->len);

  HTTPD_CONF__BEG_APP();
  
  if (!conf_parse_lex(conf, pos, len))
    goto conf_fail;

  HTTPD_CONF__END_APP();
  
  while (conf_parse_token(conf, token))
  {
    if ((token->type != CONF_TOKEN_TYPE_CLIST) || (token->depth_num != 1))
      goto conf_fail;

    if (!conf_parse_token(conf, token))
      goto conf_fail;
    
    if (!OPT_SERV_SYM_EQ("org.and.jhttpd-conf-main-1.0"))
      goto conf_fail;
  
    if (!httpd_conf_main(opts, conf, token))
      goto conf_fail;
    opts->conf_num++;
  }

  opts->conf = conf;
  
  return (TRUE);
  
 conf_fail:
  conf_parse_backtrace(out, fname, conf, token);
 read_malloc_fail:
  conf_parse_free(conf);
 conf_malloc_fail:
  return (FALSE);
}

Httpd_policy_opts *httpd_conf_main_policy_make(Httpd_opts *httpd_opts)
{
  Vstr_ref *ref = vstr_ref_make_malloc(sizeof(Httpd_policy_opts));
  int ret = httpd_conf_main_policy_init(httpd_opts, ref->ptr, ref);
  Httpd_policy_opts *opts = NULL;

  vstr_ref_del(ref);

  if (!ret)
    return (NULL);

  opts = ref->ptr;
  if (!httpd_conf_main_policy_copy(opts, httpd_opts->def_policy))
  {
    vstr_ref_del(ref);
    return (NULL);
  }
  
  opts->next = httpd_opts->def_policy->next;
  httpd_opts->def_policy->next = opts;
  
  return (opts);
}

int httpd_conf_main_init(Httpd_opts *httpd_opts)
{
  Httpd_policy_opts *opts = httpd_opts->def_policy;
  static Vstr_ref ref[1];
  
  ref->func = vstr_ref_cb_free_nothing;
  ref->ptr  = NULL;
  ref->ref  = 1;
  
  if (!httpd_conf_main_policy_init(httpd_opts, opts, ref))
    return (FALSE);
  httpd_opts->def_policy->next = NULL;
  
  vstr_add_cstr_ptr(opts->policy_name,       0, HTTPD_CONF_DEF_POLICY_NAME);
  vstr_add_cstr_ptr(opts->server_name,       0, HTTPD_CONF_DEF_SERVER_NAME);
  vstr_add_cstr_ptr(opts->dir_filename,      0, HTTPD_CONF_DEF_DIR_FILENAME);
  vstr_add_cstr_ptr(opts->mime_types_def_ct, 0, HTTPD_CONF_MIME_TYPE_DEF_CT);
  vstr_add_cstr_ptr(opts->mime_types_main,   0, HTTPD_CONF_MIME_TYPE_MAIN);
  vstr_add_cstr_ptr(opts->mime_types_xtra,   0, HTTPD_CONF_MIME_TYPE_XTRA);

  if (opts->policy_name->conf->malloc_bad)
    return (FALSE);

  return (TRUE);
}

void httpd_conf_main_free(Httpd_opts *opts)
{
  Httpd_policy_opts *scan = opts->def_policy;

  while (scan)
  {
    Httpd_policy_opts *scan_next = scan->next;
    
    mime_types_exit(scan->mime_types);
    httpd_conf_main_policy_free(scan);
    
    scan = scan_next;  
  }
  
  conf_parse_free(opts->conf); opts->conf = NULL;
}
