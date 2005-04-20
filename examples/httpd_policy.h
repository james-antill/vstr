#ifndef HTTPD_POLICY_H
#define HTTPD_POLICY_H

#include "conf.h"

#include <netinet/in.h>

#define HTTPD_POLICY__PATH_LIM_FULL 0
#define HTTPD_POLICY__PATH_LIM_BEG  1
#define HTTPD_POLICY__PATH_LIM_END  2
#define HTTPD_POLICY__PATH_LIM_EQ   3
#define HTTPD_POLICY__PATH_LIM_MASK 3
#define HTTPD_POLICY_PATH_LIM_NONE       0
#define HTTPD_POLICY_PATH_LIM_PATH_FULL (0x4  | HTTPD_POLICY__PATH_LIM_FULL)
#define HTTPD_POLICY_PATH_LIM_PATH_BEG  (0x4  | HTTPD_POLICY__PATH_LIM_BEG)
#define HTTPD_POLICY_PATH_LIM_PATH_END  (0x4  | HTTPD_POLICY__PATH_LIM_END)
#define HTTPD_POLICY_PATH_LIM_PATH_EQ   (0x4  | HTTPD_POLICY__PATH_LIM_EQ)
#define HTTPD_POLICY_PATH_LIM_NAME_FULL (0x8  | HTTPD_POLICY__PATH_LIM_FULL)
#define HTTPD_POLICY_PATH_LIM_NAME_BEG  (0x8  | HTTPD_POLICY__PATH_LIM_BEG)
#define HTTPD_POLICY_PATH_LIM_NAME_END  (0x8  | HTTPD_POLICY__PATH_LIM_END)
#define HTTPD_POLICY_PATH_LIM_NAME_EQ   (0x8  | HTTPD_POLICY__PATH_LIM_EQ)
#define HTTPD_POLICY_PATH_LIM_EXTN_FULL (0xc  | HTTPD_POLICY__PATH_LIM_FULL)
#define HTTPD_POLICY_PATH_LIM_EXTN_BEG  (0xc  | HTTPD_POLICY__PATH_LIM_BEG)
#define HTTPD_POLICY_PATH_LIM_EXTN_END  (0xc  | HTTPD_POLICY__PATH_LIM_END)
#define HTTPD_POLICY_PATH_LIM_EXTN_EQ   (0xc  | HTTPD_POLICY__PATH_LIM_EQ)
#define HTTPD_POLICY_PATH_LIM_BNWE_FULL (0x14 | HTTPD_POLICY__PATH_LIM_FULL)
#define HTTPD_POLICY_PATH_LIM_BNWE_BEG  (0x14 | HTTPD_POLICY__PATH_LIM_BEG)
#define HTTPD_POLICY_PATH_LIM_BNWE_END  (0x14 | HTTPD_POLICY__PATH_LIM_END)
#define HTTPD_POLICY_PATH_LIM_BNWE_EQ   (0x14 | HTTPD_POLICY__PATH_LIM_EQ)

/* choosing (1024) as a offset ... kinda hackyish */
#define HTTPD_POLICY_REQ_PATH_BEG (1024 +  0)
#define HTTPD_POLICY_REQ_PATH_END (1024 +  1)
#define HTTPD_POLICY_REQ_PATH_EQ  (1024 +  2)
#define HTTPD_POLICY_REQ_NAME_BEG (1024 +  3)
#define HTTPD_POLICY_REQ_NAME_END (1024 +  4)
#define HTTPD_POLICY_REQ_NAME_EQ  (1024 +  5)
#define HTTPD_POLICY_REQ_BNWE_BEG (1024 +  6)
#define HTTPD_POLICY_REQ_BNWE_END (1024 +  7)
#define HTTPD_POLICY_REQ_BNWE_EQ  (1024 +  8)
#define HTTPD_POLICY_REQ_EXTN_BEG (1024 +  9)
#define HTTPD_POLICY_REQ_EXTN_END (1024 + 10)
#define HTTPD_POLICY_REQ_EXTN_EQ  (1024 + 11)

typedef struct Httpd_policy_path
{
 Vstr_base *s1;
 void (*ref_func)(Vstr_ref *);
} Httpd_policy_path;

typedef struct Httpd_policy_ipv4
{
  unsigned char ipv4[4];
  unsigned int cidr; 
} Httpd_policy_ipv4;

extern void httpd_policy_change_con(struct Con *, Httpd_policy_opts *);
extern void httpd_policy_change_req(Httpd_req_data *, Httpd_policy_opts *);

extern Httpd_policy_opts *httpd_policy_find(Httpd_policy_opts *,
                                            const Conf_parse *,
                                            Conf_token *);

extern int httpd_policy_name_eq(const Conf_parse *, Conf_token *,
                                Httpd_policy_opts *, int *);
   
extern int httpd_policy_build_path(struct Con *, Httpd_req_data *,
                                   const Conf_parse *, Conf_token *,
                                   int *, int *);
extern int httpd_policy_path_make(struct Con *con, Httpd_req_data *req,
                                  Conf_parse *, Conf_token *, unsigned int,
                                  Vstr_ref **);

extern int httpd_policy_path_eq(const Vstr_base *,
                                const Vstr_base *, size_t *, size_t *);
extern int httpd_policy_path_beg_eq(const Vstr_base *,
                                    const Vstr_base *, size_t *, size_t *);
extern int httpd_policy_path_end_eq(const Vstr_base *,
                                    const Vstr_base *, size_t *, size_t *);

extern void httpd_policy_path_mod_name(const Vstr_base *, size_t *, size_t *);
extern void httpd_policy_path_mod_extn(const Vstr_base *, size_t *, size_t *);
extern void httpd_policy_path_mod_bnwe(const Vstr_base *, size_t *, size_t *);
extern int  httpd_policy_path_lim_eq(const Vstr_base *, size_t *, size_t *,
                                     unsigned int, size_t, Vstr_ref *);

extern int httpd_policy_path_req2lim(unsigned int);

extern int httpd_policy_ipv4_make(struct Con *, Httpd_req_data *,
                                  Conf_parse *, Conf_token *,
                                  unsigned int, struct sockaddr *, int *);
extern int httpd_policy_ipv4_cidr_eq(struct Con *, Httpd_req_data *,
                                     Httpd_policy_ipv4 *, struct sockaddr *);

#if !defined(HTTPD_POLICY_COMPILE_INLINE)
# ifdef VSTR_AUTOCONF_NDEBUG
#  define HTTPD_POLICY_COMPILE_INLINE 1
# else
#  define HTTPD_POLICY_COMPILE_INLINE 0
# endif
#endif

#if defined(VSTR_AUTOCONF_HAVE_INLINE) && HTTPD_POLICY_COMPILE_INLINE

#ifndef VSTR_AUTOCONF_NDEBUG
# define HTTPD_POLICY__ASSERT ASSERT
#else
# define HTTPD_POLICY__ASSERT(x)
#endif

#define HTTPD_POLICY__TRUE  1
#define HTTPD_POLICY__FALSE 0

extern inline void httpd_policy_change_con(struct Con *con,
                                           Httpd_policy_opts *policy)
{
  con->use_sendfile          = policy->use_sendfile;
  con->policy                = policy;
}

extern inline void httpd_policy_change_req(Httpd_req_data *req,
                                           Httpd_policy_opts *policy)
{
  req->parse_accept          = policy->use_err_406;
  req->allow_accept_encoding = policy->use_gzip_content_replacement;
  if (req->vhost_prefix_len && !policy->use_vhosts)
  { /* NOTE: doesn't do chk_host properly */
    vstr_del(req->fname, 1, req->vhost_prefix_len);
    req->vhost_prefix_len = 0;
  }
  req->policy                = policy;
}

extern inline Httpd_policy_opts *httpd_policy_find(Httpd_policy_opts *opts,
                                                   const Conf_parse *conf,
                                                   Conf_token *token)
{
  opts = opts->beg->def_policy;

  while (opts)
  {
    Vstr_base *tmp = opts->policy_name;
    
    if (conf_token_cmp_val_eq(conf, token, tmp, 1, tmp->len))
      return (opts);

    opts = opts->next;
  }
  
  return (NULL);
}

extern inline int httpd_policy_name_eq(const Conf_parse *conf,Conf_token *token,
                                       Httpd_policy_opts *policy, int *matches)
{
  Vstr_base *tmp = policy->policy_name;
    
  CONF_SC_PARSE_TOP_TOKEN_RET(conf, token, HTTPD_POLICY__FALSE);

  *matches = conf_token_cmp_val_eq(conf, token, tmp, 1, tmp->len);
  
  return (HTTPD_POLICY__TRUE);
}

extern inline int httpd_policy_path_eq(const Vstr_base *s1,
                                       const Vstr_base *s2,
                                       size_t *p2, size_t *l2)
{
  return (vstr_cmp_eq(s1, 1, s1->len, s2, *p2, *l2));
}

/* if the s1 is equal to the begining of s2 */
extern inline int httpd_policy_path_beg_eq(const Vstr_base *s1,
                                           const Vstr_base *s2,
                                           size_t *p2, size_t *l2)
{
  if (*l2 > s1->len)
    *l2 = s1->len;
  return (vstr_cmp_eq(s1, 1, s1->len, s2, *p2, *l2));
}

/* if the s1 is equal to the end of s2 */
extern inline int httpd_policy_path_end_eq(const Vstr_base *s1,
                                           const Vstr_base *s2,
                                           size_t *p2, size_t *l2)
{
  if (*l2 > s1->len)
  {
    *p2 += (*l2 - s1->len);
    *l2 = s1->len;
  }
  return (vstr_cmp_eq(s1, 1, s1->len, s2, *p2, *l2));
}

extern inline void httpd_policy_path_mod_name(const Vstr_base *s1,
                                              size_t *pos, size_t *len)
{
  size_t srch = vstr_srch_chr_rev(s1, *pos, *len, '/');
  size_t tmp  = 0;
  HTTPD_POLICY__ASSERT(srch); ++srch;

  tmp = (srch - *pos);
  *len -= tmp;
  *pos += tmp;
}

extern inline void httpd_policy_path_mod_extn(const Vstr_base *s1,
                                              size_t *pos, size_t *len)
{
  size_t tmp = *len;
  
  httpd_policy_path_mod_name(s1, pos, len);

  tmp = vstr_sc_poslast(*pos, *len);
  if (!(*len = vstr_srch_chr_rev(s1, *pos, *len, '.')))
    *pos = tmp + 1; /* at point just after basename */
  else
  {
    *pos = *len; /* include '.' */
    *len = vstr_sc_posdiff(*pos, tmp);
  }
}

extern inline void httpd_policy_path_mod_bnwe(const Vstr_base *s1,
                                              size_t *pos, size_t *len)
{
  size_t tmp = 0;

  httpd_policy_path_mod_name(s1, pos, len);

  if ((tmp = vstr_srch_chr_rev(s1, *pos, *len, '.')))
    *len = vstr_sc_posdiff(*pos, tmp) - 1; /* don't include '.' */
}

extern inline int httpd_policy_path_lim_eq(const Vstr_base *s1,
                                           size_t *pos, size_t *len,
                                           unsigned int lim,
                                           size_t vhost_prefix_len,
                                           Vstr_ref *ref)
{
  const Httpd_policy_path *srch = NULL;
  
  switch (lim)
  {
    default: HTTPD_POLICY__ASSERT(HTTPD_POLICY__FALSE);
    case HTTPD_POLICY_PATH_LIM_NONE:
      return (HTTPD_POLICY__TRUE);

    case HTTPD_POLICY_PATH_LIM_PATH_FULL:
    case HTTPD_POLICY_PATH_LIM_PATH_BEG:
    case HTTPD_POLICY_PATH_LIM_PATH_END:
    case HTTPD_POLICY_PATH_LIM_PATH_EQ:
      *len -= vhost_prefix_len;
      *pos += vhost_prefix_len;
      break;
    
    case HTTPD_POLICY_PATH_LIM_NAME_FULL:
    case HTTPD_POLICY_PATH_LIM_NAME_BEG:
    case HTTPD_POLICY_PATH_LIM_NAME_END:
    case HTTPD_POLICY_PATH_LIM_NAME_EQ:
      httpd_policy_path_mod_name(s1, pos, len);
      break;
    
    case HTTPD_POLICY_PATH_LIM_EXTN_FULL:
    case HTTPD_POLICY_PATH_LIM_EXTN_BEG:
    case HTTPD_POLICY_PATH_LIM_EXTN_END:
    case HTTPD_POLICY_PATH_LIM_EXTN_EQ:
      httpd_policy_path_mod_extn(s1, pos, len);
      break;

    case HTTPD_POLICY_PATH_LIM_BNWE_FULL:
    case HTTPD_POLICY_PATH_LIM_BNWE_BEG:
    case HTTPD_POLICY_PATH_LIM_BNWE_END:
    case HTTPD_POLICY_PATH_LIM_BNWE_EQ:
      httpd_policy_path_mod_bnwe(s1, pos, len);
      break;
  }

  if ((lim & HTTPD_POLICY__PATH_LIM_MASK) == HTTPD_POLICY__PATH_LIM_FULL)
    return (HTTPD_POLICY__TRUE);
  
  HTTPD_POLICY__ASSERT(ref);
  srch = ref->ptr;
  
  if (0) { }
  else if ((lim & HTTPD_POLICY__PATH_LIM_MASK) == HTTPD_POLICY__PATH_LIM_BEG)
  {
    if (!httpd_policy_path_beg_eq(srch->s1, s1, pos, len))
      goto path_no_match;
  }
  else if ((lim & HTTPD_POLICY__PATH_LIM_MASK) == HTTPD_POLICY__PATH_LIM_END)
  {
    if (!httpd_policy_path_end_eq(srch->s1, s1, pos, len))
      goto path_no_match;
  }
  else if ((lim & HTTPD_POLICY__PATH_LIM_MASK) == HTTPD_POLICY__PATH_LIM_EQ)
  {
    if (!httpd_policy_path_eq(srch->s1, s1, pos, len))
      goto path_no_match;
  }
  else
    HTTPD_POLICY__ASSERT(HTTPD_POLICY__FALSE);
    
  vstr_ref_del(ref); ref = NULL;
  return (HTTPD_POLICY__TRUE);
 path_no_match:
  vstr_ref_del(ref); ref = NULL;
  return (HTTPD_POLICY__FALSE);
}

extern inline int httpd_policy_path_req2lim(unsigned int type)
{
  unsigned int lim = HTTPD_POLICY_PATH_LIM_NONE;
  
  switch (type)
  {
    case HTTPD_POLICY_REQ_PATH_BEG: lim = HTTPD_POLICY_PATH_LIM_PATH_BEG; break;
    case HTTPD_POLICY_REQ_PATH_END: lim = HTTPD_POLICY_PATH_LIM_PATH_END; break;
    case HTTPD_POLICY_REQ_PATH_EQ:  lim = HTTPD_POLICY_PATH_LIM_PATH_EQ;  break;
      
    case HTTPD_POLICY_REQ_NAME_BEG: lim = HTTPD_POLICY_PATH_LIM_NAME_BEG; break;
    case HTTPD_POLICY_REQ_NAME_END: lim = HTTPD_POLICY_PATH_LIM_NAME_END; break;
    case HTTPD_POLICY_REQ_NAME_EQ:  lim = HTTPD_POLICY_PATH_LIM_NAME_EQ;  break;
      
    case HTTPD_POLICY_REQ_BNWE_BEG: lim = HTTPD_POLICY_PATH_LIM_BNWE_BEG; break;
    case HTTPD_POLICY_REQ_BNWE_END: lim = HTTPD_POLICY_PATH_LIM_BNWE_END; break;
    case HTTPD_POLICY_REQ_BNWE_EQ:  lim = HTTPD_POLICY_PATH_LIM_BNWE_EQ;  break;
      
    case HTTPD_POLICY_REQ_EXTN_BEG: lim = HTTPD_POLICY_PATH_LIM_EXTN_BEG; break;
    case HTTPD_POLICY_REQ_EXTN_END: lim = HTTPD_POLICY_PATH_LIM_EXTN_END; break;
    case HTTPD_POLICY_REQ_EXTN_EQ:  lim = HTTPD_POLICY_PATH_LIM_EXTN_EQ;  break;
      
    default:
      HTTPD_POLICY__ASSERT(HTTPD_POLICY__FALSE);
  }

  return (lim);
}

#endif

#endif
