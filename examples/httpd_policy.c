
#include "httpd.h"
#include "httpd_policy.h"

#define EX_UTILS_NO_FUNCS 1
#include "ex_utils.h"

static void httpd__policy_path_ref_free(Vstr_ref *ref)
{
  Httpd_policy_path *path = NULL;

  if (!ref)
    return;
  
  path = ref->ptr;
  vstr_free_base(path->s1);
  (*path->ref_func)(ref);
}

static int httpd__policy_path_init(Httpd_policy_path *path,
                                   Vstr_base *s1, size_t pos, size_t len, 
                                   Vstr_ref *ref)
{
  size_t tmp = 0;
  
  ASSERT(ref->ref == 1);
  
  if (!(s1 = vstr_dup_vstr(s1->conf, s1, pos, len, 0)))
    return (FALSE);
  path->s1 = s1;

  /* remove any double slashes... */
  while ((tmp = vstr_srch_cstr_buf_fwd(s1, 1, s1->len, "//")))
    if (!vstr_del(s1, tmp, 1))
    {
      vstr_free_base(s1);
      return (FALSE);
    }
  
  path->ref_func = ref->func;
  
  ref->func = httpd__policy_path_ref_free;

  return (TRUE);
}

static int httpd_policy__build_parent_path(Vstr_base *s1, Vstr_base *s2)
{
  size_t pos = 0;
  size_t len = 0;

  ASSERT(s1 && s2);
  
  if (!s2->len)
    return (TRUE);
      
  if (s2->len == 1)
  {
    ASSERT(vstr_cmp_cstr_eq(s2, 1, s2->len, "/"));
    vstr_add_cstr_ptr(s1, s1->len, "/");
    return (TRUE);
  }
      
  if (!(pos = vstr_srch_chr_rev(s2, 1, s2->len - 1, '/')))
  {
    vstr_add_cstr_ptr(s1, s1->len, "./");
    return (TRUE);
  }
      
  len = vstr_sc_posdiff(1, pos);
  if (len > 1)
    --len;
  HTTPD_APP_REF_VSTR(s1, s2, pos, len);

  return (TRUE);
}

/* builds into conf->tmp */
int httpd_policy_build_path(struct Con *con, Httpd_req_data *req,
                            const Conf_parse *conf, Conf_token *token,
                            int *used_policy, int *used_req)
{
  unsigned int cur_depth = token->depth_num;
  int dummy_used_policy;
  int dummy_used_req;

  if (!used_policy) used_policy = &dummy_used_policy;
  if (!used_req)    used_req    = &dummy_used_req;
  *used_policy = *used_req = FALSE;
  
  vstr_del(conf->tmp, 1, conf->tmp->len);
  while (conf_token_list_num(token, cur_depth))
  {
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    
    if (0) { }
    
    else if (OPT_SERV_SYM_EQ("<basename>"))
    {
      size_t pos = 1;
      size_t len = req->fname->len;
      
      *used_req = TRUE;
      httpd_policy_path_mod_name(req->fname, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, req->fname, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<url-basename>"))
    {
      size_t pos = req->path_pos;
      size_t len = req->path_len;
      
      *used_req = TRUE;
      httpd_policy_path_mod_name(con->evnt->io_r, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, con->evnt->io_r, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<basename-without-extension>"))
    {
      size_t pos = 1;
      size_t len = req->fname->len;

      *used_req = TRUE;
      httpd_policy_path_mod_bwen(req->fname, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, req->fname, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<url-basename-without-extension>"))
    {
      size_t pos = req->path_pos;
      size_t len = req->path_len;

      *used_req = TRUE;
      httpd_policy_path_mod_bwen(con->evnt->io_r, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, con->evnt->io_r, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<basename-without-extensions>"))
    {
      size_t pos = 1;
      size_t len = req->fname->len;

      *used_req = TRUE;
      httpd_policy_path_mod_bwes(req->fname, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, req->fname, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<url-basename-without-extensions>"))
    {
      size_t pos = req->path_pos;
      size_t len = req->path_len;

      *used_req = TRUE;
      httpd_policy_path_mod_bwes(con->evnt->io_r, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, con->evnt->io_r, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<dirname>"))
    {
      size_t pos = 1;
      size_t len = req->fname->len;
      
      *used_req = TRUE;
      httpd_policy_path_mod_dirn(req->fname, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, req->fname, 1, len);
    }
    else if (OPT_SERV_SYM_EQ("<url-dirname>"))
    {
      size_t pos = req->path_pos;
      size_t len = req->path_len;
      
      *used_req = TRUE;
      httpd_policy_path_mod_dirn(con->evnt->io_r, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, con->evnt->io_r, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<document-root>") ||
             OPT_SERV_SYM_EQ("<doc-root>"))
    {
      Vstr_base *s1 = req->policy->document_root;
      *used_policy = TRUE;
      HTTPD_APP_REF_VSTR(conf->tmp, s1, 1, s1->len);
    }
    else if (OPT_SERV_SYM_EQ("<document-root/..>") ||
             OPT_SERV_SYM_EQ("<doc-root/..>"))
    {
      ASSERT(req->policy->document_root->len);
      if (!httpd_policy__build_parent_path(conf->tmp,
                                           req->policy->document_root))
        return (FALSE);
      *used_policy = TRUE;
    }
    else if (OPT_SERV_SYM_EQ("<extension>"))
    {
      size_t pos = 1;
      size_t len = req->fname->len;

      *used_req = TRUE;
      httpd_policy_path_mod_extn(req->fname, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, req->fname, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<url-extension>"))
    {
      size_t pos = req->path_pos;
      size_t len = req->path_len;
      
      *used_req = TRUE;
      httpd_policy_path_mod_extn(con->evnt->io_r, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, con->evnt->io_r, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<extensions>"))
    {
      size_t pos = 1;
      size_t len = req->fname->len;

      *used_req = TRUE;
      httpd_policy_path_mod_exts(req->fname, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, req->fname, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<url-extensions>"))
    {
      size_t pos = req->path_pos;
      size_t len = req->path_len;
      
      *used_req = TRUE;
      httpd_policy_path_mod_exts(con->evnt->io_r, &pos, &len);
      HTTPD_APP_REF_VSTR(conf->tmp, con->evnt->io_r, pos, len);
    }
    else if (OPT_SERV_SYM_EQ("<hostname>"))
    {
      Vstr_base *http_data = con->evnt->io_r;
      Vstr_sect_node *h_h = req->http_hdrs->hdr_host;
      
      *used_req = TRUE;
      if (h_h->len)
        HTTPD_APP_REF_VSTR(conf->tmp, http_data, h_h->pos, h_h->len);
      else
        httpd_sc_add_default_hostname(req->policy, conf->tmp, conf->tmp->len);
    }
    else if (OPT_SERV_SYM_EQ("<request-configuration-directory>") ||
             OPT_SERV_SYM_EQ("<req-conf-dir>"))
    {
      Vstr_base *s1 = req->policy->req_configuration_dir;
      *used_policy = TRUE;
      HTTPD_APP_REF_VSTR(conf->tmp, s1, 1, s1->len);
    }
    else if (OPT_SERV_SYM_EQ("<request-configuration-directory/..>") ||
             OPT_SERV_SYM_EQ("<req-conf-dir/..>"))
    {
      if (!httpd_policy__build_parent_path(conf->tmp,
                                           req->policy->req_configuration_dir))
        return (FALSE);
      *used_policy = TRUE;
    }
    else if (OPT_SERV_SYM_EQ("<file-path>") || OPT_SERV_SYM_EQ("<path>"))
    {
      *used_req = TRUE;
      HTTPD_APP_REF_VSTR(conf->tmp, req->fname, 1, req->fname->len);
    }
    else if (OPT_SERV_SYM_EQ("<url-path>"))
    {
      *used_req = TRUE;
      HTTPD_APP_REF_VSTR(conf->tmp,
                         con->evnt->io_r, req->path_pos, req->path_len);
    }
    else if (OPT_SERV_SYM_EQ("<content-type-extension>") ||
             OPT_SERV_SYM_EQ("<content-type-path>"))
    {
      const Vstr_base *s1 = req->ext_vary_a_vs1;
      size_t pos          = req->ext_vary_a_pos;
      size_t len          = req->ext_vary_a_len;
      
      *used_req = TRUE;
      if (s1 && len)
        HTTPD_APP_REF_VSTR(conf->tmp, s1, pos, len);
    }
    else
    { /* unknown symbol or string */
      size_t pos = conf->tmp->len + 1;
      const Vstr_sect_node *pv = conf_token_value(token);
      
      if (!pv || !HTTPD_APP_REF_VSTR(conf->tmp, conf->data, pv->pos, pv->len))
        return (FALSE);
      
      OPT_SERV_X__ESC_VSTR(conf->tmp, pos, pv->len);
    }
  }

  if (!con->policy->beg->def_policy->next)
    *used_policy = FALSE;
  /* FIXME: if we are in here inside a group with a test for policy-eq
   * then we can act like we didn't use the policy.
   */
  
  return (TRUE);
}

int httpd_policy_path_make(struct Con *con, Httpd_req_data *req,
                           Conf_parse *conf, Conf_token *token,
                           unsigned int type, Vstr_ref **ret_ref)
{
  Conf_token save = *token;
  Vstr_ref *ref = NULL;
  int ret = FALSE;
  int used_pol = FALSE;
  int used_req = FALSE;
  
  ASSERT(ret_ref);
  *ret_ref     = NULL;

  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);

  if (token->type == CONF_TOKEN_TYPE_CLIST)
  {
    CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
    if (!OPT_SERV_SYM_EQ("assign") && !OPT_SERV_SYM_EQ("="))
      return (FALSE);
    if (!httpd_policy_build_path(con, req, conf, token, &used_pol, &used_req))
      return (FALSE);
  }
  else
  {
    const Vstr_sect_node *pv = conf_token_value(token);
    Vstr_base *s1 = conf->tmp;
    if (!pv || !vstr_sub_vstr(s1, 1, s1->len, conf->data, pv->pos, pv->len,
                              VSTR_TYPE_SUB_BUF_REF))
      return (FALSE);
    OPT_SERV_X__ESC_VSTR(s1, 1, pv->len);    
  }
  
  if (!(ref = vstr_ref_make_malloc(sizeof(Httpd_policy_path))))
    return (FALSE);

  ret = httpd__policy_path_init(ref->ptr, conf->tmp, 1, conf->tmp->len, ref);
  if (ret && !(used_pol || used_req))
    ret = conf_token_set_user_value(conf, &save, type, ref);
  
  if (ret)
    *ret_ref = ref;

  return (ret);
}

int httpd_policy_ipv4_make(struct Con *con, Httpd_req_data *req,
                           Conf_parse *conf, Conf_token *token,
                           unsigned int type, struct sockaddr *sa, int *matches)
{
  Conf_token save = *token;
  Vstr_ref *ref = NULL;
  Httpd_policy_ipv4 *data = NULL;
  int ret = FALSE;
  
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, FALSE);
  
  if (!(ref = vstr_ref_make_malloc(sizeof(Httpd_policy_ipv4))))
    return (FALSE);
  data = ref->ptr;
  
  ret = vstr_parse_ipv4(conf->data, token->u.node->pos, token->u.node->len,
                        data->ipv4, &data->cidr,
                        VSTR_FLAG05(PARSE_IPV4, CIDR, CIDR_FULL,
                                    NETMASK, NETMASK_FULL, ONLY), NULL, NULL);

  if (ret)
  {
    *matches = httpd_policy_ipv4_cidr_eq(con, req, data, sa);
    
    ret = conf_token_set_user_value(conf, &save, type, ref);
  }
  
  vstr_ref_del(ref);

  if (!ret)
    return (FALSE);
  
  return (TRUE);
}

int httpd_policy_ipv4_cidr_eq(struct Con *con, Httpd_req_data *req,
                              Httpd_policy_ipv4 *data, struct sockaddr *sa)
{
  struct sockaddr_in *sa_in = NULL;
  uint32_t tst_addr_ipv4;
  unsigned char tst_ipv4[4];
  unsigned int scan = 0;
  unsigned int cidr = data->cidr;
  
  ASSERT(cidr <= 32);

  if (sa == EVNT_SA(con->evnt))
  {
    if (req)
      req->vary_star = TRUE;
    else
      con->vary_star = TRUE;
  }
  
  if (!sa || (sa->sa_family != AF_INET))
    return (FALSE);
  sa_in = EVNT_SA_IN(con->evnt);
    
  tst_addr_ipv4 = ntohl(sa_in->sin_addr.s_addr);
    
  tst_ipv4[3] = (tst_addr_ipv4 >>  0) & 0xFF;
  tst_ipv4[2] = (tst_addr_ipv4 >>  8) & 0xFF;
  tst_ipv4[1] = (tst_addr_ipv4 >> 16) & 0xFF;
  tst_ipv4[0] = (tst_addr_ipv4 >> 24) & 0xFF;

  scan = 0;
  while (cidr >= 8)
  {
    if (tst_ipv4[scan] != data->ipv4[scan])
      return (FALSE);
    
    ++scan;
    cidr -= 8;
  }
  ASSERT(!cidr || (scan < 4));
  
  if (cidr)
  {
    cidr = (1 << cidr) - 1; /* x/7 == (1 << 7) - 1 == 0b0111_1111 */
    if ((tst_ipv4[scan] & cidr) != (data->ipv4[scan] & cidr))
      return (FALSE);
  }

  return (TRUE);
}

