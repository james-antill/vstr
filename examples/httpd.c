/*
 *  Copyright (C) 2004, 2005  James Antill
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  email: james@and.org
 */
/* main HTTPD APIs, only really implements server portions */
#define HTTPD_HAVE_GLOBAL_OPTS 1
#include "httpd.h"
#include "httpd_policy.h"

#define EX_UTILS_NO_USE_INIT  1
#define EX_UTILS_NO_USE_EXIT  1
#define EX_UTILS_NO_USE_LIMIT 1
#define EX_UTILS_NO_USE_BLOCK 1
#define EX_UTILS_NO_USE_PUT   1
#define EX_UTILS_USE_NONBLOCKING_OPEN 1
#define EX_UTILS_RET_FAIL     1
#include "ex_utils.h"

#include "date.h"

#include "vlg.h"

#ifndef POSIX_FADV_SEQUENTIAL
# define posix_fadvise64(x1, x2, x3, x4) (errno = ENOSYS, -1)
#endif

#define HTTP_CONF_SAFE_PRINT_REQ TRUE

#ifdef VSTR_AUTOCONF_NDEBUG
# define HTTP_CONF_MMAP_LIMIT_MIN (16 * 1024) /* a couple of pages */
#else
# define HTTP_CONF_MMAP_LIMIT_MIN 8 /* debug... */
#endif
#define HTTP_CONF_MMAP_LIMIT_MAX (50 * 1024 * 1024)

#define HTTP_CONF_FS_READ_CALL_LIMIT 4

#define CLEN VSTR__AT_COMPILE_STRLEN

/* is the cstr a prefix of the vstr */
#define VPREFIX(vstr, p, l, cstr)                                       \
    (((l) >= CLEN(cstr)) &&                                             \
     vstr_cmp_buf_eq(vstr, p, CLEN(cstr), cstr, CLEN(cstr)))
/* is the cstr a suffix of the vstr */
#define VSUFFIX(vstr, p, l, cstr)                                       \
    (((l) >= CLEN(cstr)) &&                                             \
     vstr_cmp_eod_buf_eq(vstr, p, l, cstr, CLEN(cstr)))

/* is the cstr a prefix of the vstr, no case */
#define VIPREFIX(vstr, p, l, cstr)                                      \
    (((l) >= CLEN(cstr)) &&                                             \
     vstr_cmp_case_buf_eq(vstr, p, CLEN(cstr), cstr, CLEN(cstr)))

/* for simplicity */
#define VEQ(vstr, p, l, cstr)  vstr_cmp_cstr_eq(vstr, p, l, cstr)
#define VIEQ(vstr, p, l, cstr) vstr_cmp_case_cstr_eq(vstr, p, l, cstr)

#ifndef SWAP_TYPE
#define SWAP_TYPE(x, y, type) do {              \
      type internal_local_tmp = (x);            \
      (x) = (y);                                \
      (y) = internal_local_tmp;                 \
    } while (FALSE)
#endif

#define HTTP__HDR_SET(req, h, p, l) do {               \
      (req)-> http_hdrs -> hdr_ ## h ->pos = (p);          \
      (req)-> http_hdrs -> hdr_ ## h ->len = (l);          \
    } while (FALSE)
#define HTTP__HDR_MULTI_SET(req, h, p, l) do {         \
      (req)-> http_hdrs -> multi -> hdr_ ## h ->pos = (p); \
      (req)-> http_hdrs -> multi -> hdr_ ## h ->len = (l); \
    } while (FALSE)

#define HTTP__CONTENT_INIT_HDR(x) do {          \
      req-> x ## _vs1 = NULL;                   \
      req-> x ## _pos = 0;                      \
      req-> x ## _len = 0;                      \
    } while (FALSE)

#define HTTP__CONTENT_PARAMS(req, x)                            \
    (req)-> x ## _vs1, (req)-> x ## _pos, (req)-> x ## _len


HTTPD_CONF_MAIN_DECL_OPTS(httpd_opts, HTTPD_CONF_SERV_DEF_PORT);

static Vlg *vlg = NULL;


void httpd_init(Vlg *passed_vlg)
{
  ASSERT(passed_vlg && !vlg);
  vlg = passed_vlg;
}

void httpd_exit(void)
{
  ASSERT(vlg);
  vlg = NULL;
}

static void http__clear_hdrs(struct Httpd_req_data *req)
{
  Vstr_base *tmp = req->http_hdrs->multi->combiner_store;

  ASSERT(tmp);
  
  HTTP__HDR_SET(req, ua,                  0, 0);
  HTTP__HDR_SET(req, referer,             0, 0);

  HTTP__HDR_SET(req, expect,              0, 0);
  HTTP__HDR_SET(req, host,                0, 0);
  HTTP__HDR_SET(req, if_match,            0, 0);
  HTTP__HDR_SET(req, if_modified_since,   0, 0);
  HTTP__HDR_SET(req, if_none_match,       0, 0);
  HTTP__HDR_SET(req, if_range,            0, 0);
  HTTP__HDR_SET(req, if_unmodified_since, 0, 0);

  vstr_del(tmp, 1, tmp->len);
  HTTP__HDR_MULTI_SET(req, accept,          0, 0);
  HTTP__HDR_MULTI_SET(req, accept_charset,  0, 0);
  HTTP__HDR_MULTI_SET(req, accept_encoding, 0, 0);
  HTTP__HDR_MULTI_SET(req, accept_language, 0, 0);
  HTTP__HDR_MULTI_SET(req, connection,      0, 0);
  HTTP__HDR_MULTI_SET(req, range,           0, 0);
}

static void http__clear_xtra(struct Httpd_req_data *req)
{
  if (req->xtra_content)
    vstr_del(req->xtra_content, 1, req->xtra_content->len);

  HTTP__CONTENT_INIT_HDR(content_type);
  HTTP__CONTENT_INIT_HDR(content_location);
  HTTP__CONTENT_INIT_HDR(content_md5);
  HTTP__CONTENT_INIT_HDR(cache_control);
  HTTP__CONTENT_INIT_HDR(expires);
  HTTP__CONTENT_INIT_HDR(p3p);
  HTTP__CONTENT_INIT_HDR(ext_vary_a);
  HTTP__CONTENT_INIT_HDR(ext_vary_ac);
  HTTP__CONTENT_INIT_HDR(ext_vary_al);
}

Httpd_req_data *http_req_make(struct Con *con)
{
  static Httpd_req_data real_req[1];
  Httpd_req_data *req = real_req;

  ASSERT(!req->using_req);

  if (!req->done_once)
  {
    Vstr_conf *conf = NULL;

    if (con)
      conf = con->evnt->io_w->conf;
      
    if (!(req->fname = vstr_make_base(conf)) ||
        !(req->http_hdrs->multi->combiner_store = vstr_make_base(conf)) ||
        !(req->sects = vstr_sects_make(8)))
      return (NULL);
    
    req->f_mmap       = NULL;
    req->xtra_content = NULL;
  }

  httpd_policy_change_req(req, con ? con->policy : httpd_opts->def_policy);
  
  http__clear_hdrs(req);
  
  http__clear_xtra(req);
  
  req->http_hdrs->multi->comb = con ? con->evnt->io_r : NULL;

  vstr_del(req->fname, 1, req->fname->len);

  req->now = evnt_sc_time();
  
  req->len = 0;

  req->path_pos = 0;
  req->path_len = 0;

  req->error_code = 0;
  req->error_line = "";
  req->error_msg  = "";
  req->error_len  = 0;

  req->sects->num = 0;
  /* f_stat */
  if (con)
    req->orig_io_w_len = con->evnt->io_w->len;

  /* NOTE: These should probably be allocated at init time, depending on the
   * option flags given */
  ASSERT(!req->f_mmap || !req->f_mmap->len);
  if (req->f_mmap)
    vstr_del(req->f_mmap, 1, req->f_mmap->len);
  ASSERT(!req->xtra_content || !req->xtra_content->len);
  if (req->xtra_content)
    vstr_del(req->xtra_content, 1, req->xtra_content->len);
  
  req->vhost_prefix_len = 0;
  
  req->sects->malloc_bad = FALSE;

  req->content_encoding_gzip     = FALSE;
  req->content_encoding_bzip2    = FALSE;
  req->content_encoding_identity = TRUE;

  req->output_keep_alive_hdr = FALSE;

  req->user_return_error_code = FALSE;

  req->vary_star = con ? con->vary_star : TRUE;
  req->vary_a    = FALSE;
  req->vary_ac   = FALSE;
  req->vary_ae   = FALSE;
  req->vary_al   = FALSE;
  req->vary_rf   = FALSE;
  req->vary_ua   = FALSE;

  req->direct_uri         = FALSE;
  req->direct_filename    = FALSE;
  req->skip_document_root = FALSE;
  
  req->ver_0_9    = FALSE;
  req->ver_1_1    = FALSE;
  req->use_mmap   = FALSE;
  req->head_op    = FALSE;

  req->done_once  = TRUE;
  req->using_req  = TRUE;

  req->malloc_bad = FALSE;

  return (req);
}

void http_req_free(Httpd_req_data *req)
{
  if (!req) /* for if/when move to malloc/free */
    return;

  ASSERT(req->done_once && req->using_req);

  /* we do vstr deletes here to return the nodes back to the pool */
  vstr_del(req->fname, 1, req->fname->len);
  ASSERT(!req->http_hdrs->multi->combiner_store->len);
  if (req->f_mmap)
    vstr_del(req->f_mmap, 1, req->f_mmap->len);

  http__clear_xtra(req);
  
  req->http_hdrs->multi->comb = NULL;

  req->using_req = FALSE;
}

void http_req_exit(void)
{
  Httpd_req_data *req = http_req_make(NULL);
  struct Http_hdrs__multi *tmp = NULL;
  
  if (!req)
    return;

  tmp = req->http_hdrs->multi;
  
  vstr_free_base(req->fname);          req->fname          = NULL;
  vstr_free_base(tmp->combiner_store); tmp->combiner_store = NULL;
  vstr_free_base(req->f_mmap);         req->f_mmap         = NULL;
  vstr_free_base(req->xtra_content);   req->xtra_content   = NULL;
  vstr_sects_free(req->sects);         req->sects          = NULL;
  
  req->done_once = FALSE;
  req->using_req = FALSE;
}


/* HTTP crack -- Implied linear whitespace between tokens, note that it
 * is *LWS == *([CRLF] 1*(SP | HT)) */
static void http__skip_lws(Vstr_base *s1, size_t *pos, size_t *len)
{
  size_t lws__len = 0;
  
  ASSERT(s1 && pos && len);
  
  while (TRUE)
  {
    if (VPREFIX(s1, *pos, *len, HTTP_EOL))
    {
      *len -= CLEN(HTTP_EOL); *pos += CLEN(HTTP_EOL);
    }
    else if (lws__len)
      break;
    
    if (!(lws__len = vstr_spn_cstr_chrs_fwd(s1, *pos, *len, HTTP_LWS)))
      break;
    *len -= lws__len;
    *pos += lws__len;
  }
}

/* prints out headers in human friedly way for log files */
#define PCUR (pos + (base->len - orig_len))
static int http_app_vstr_escape(Vstr_base *base, size_t pos,
                                Vstr_base *sf, size_t sf_pos, size_t sf_len)
{
  unsigned int sf_flags = VSTR_TYPE_ADD_BUF_PTR;
  Vstr_iter iter[1];
  size_t orig_len = base->len;
  int saved_malloc_bad = FALSE;
  size_t norm_chr = 0;

  if (!sf_len) /* hack around !sf_pos */
    return (TRUE);
  
  if (!vstr_iter_fwd_beg(sf, sf_pos, sf_len, iter))
    return (FALSE);

  saved_malloc_bad = base->conf->malloc_bad;
  base->conf->malloc_bad = FALSE;
  while (sf_len)
  { /* assumes ASCII */
    char scan = vstr_iter_fwd_chr(iter, NULL);

    if ((scan >= ' ') && (scan <= '~') && (scan != '"') && (scan != '\\'))
      ++norm_chr;
    else
    {
      vstr_add_vstr(base, PCUR, sf, sf_pos, norm_chr, sf_flags);
      sf_pos += norm_chr;
      norm_chr = 0;
      
      if (0) {}
      else if (scan == '"')  vstr_add_cstr_buf(base, PCUR, "\\\"");
      else if (scan == '\\') vstr_add_cstr_buf(base, PCUR, "\\");
      else if (scan == '\t') vstr_add_cstr_buf(base, PCUR, "\\t");
      else if (scan == '\v') vstr_add_cstr_buf(base, PCUR, "\\v");
      else if (scan == '\r') vstr_add_cstr_buf(base, PCUR, "\\r");
      else if (scan == '\n') vstr_add_cstr_buf(base, PCUR, "\\n");
      else if (scan == '\b') vstr_add_cstr_buf(base, PCUR, "\\b");
      else
        vstr_add_sysfmt(base, PCUR, "\\x%02hhx", scan);
      ++sf_pos;
    }
    
    --sf_len;
  }

  vstr_add_vstr(base, PCUR, sf, sf_pos, norm_chr, sf_flags);
  
  if (base->conf->malloc_bad)
    return (FALSE);
  
  base->conf->malloc_bad = saved_malloc_bad;
  return (TRUE);
}
#undef PCUR

static int http__fmt__add_vstr_add_vstr(Vstr_base *base, size_t pos,
                                        Vstr_fmt_spec *spec)
{
  Vstr_base *sf = VSTR_FMT_CB_ARG_PTR(spec, 0);
  size_t sf_pos = VSTR_FMT_CB_ARG_VAL(spec, size_t, 1);
  size_t sf_len = VSTR_FMT_CB_ARG_VAL(spec, size_t, 2);
  
  return (http_app_vstr_escape(base, pos, sf, sf_pos, sf_len));
}
int http_fmt_add_vstr_add_vstr(Vstr_conf *conf, const char *name)
{
  return (vstr_fmt_add(conf, name, http__fmt__add_vstr_add_vstr,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_SIZE_T,
                       VSTR_TYPE_FMT_SIZE_T,
                       VSTR_TYPE_FMT_END));
}
static int http__fmt__add_vstr_add_sect_vstr(Vstr_base *base, size_t pos,
                                             Vstr_fmt_spec *spec)
{
  Vstr_base *sf     = VSTR_FMT_CB_ARG_PTR(spec, 0);
  Vstr_sects *sects = VSTR_FMT_CB_ARG_PTR(spec, 1);
  unsigned int num  = VSTR_FMT_CB_ARG_VAL(spec, unsigned int, 2);
  size_t sf_pos     = VSTR_SECTS_NUM(sects, num)->pos;
  size_t sf_len     = VSTR_SECTS_NUM(sects, num)->len;
  
  return (http_app_vstr_escape(base, pos, sf, sf_pos, sf_len));
}
int http_fmt_add_vstr_add_sect_vstr(Vstr_conf *conf, const char *name)
{
  return (vstr_fmt_add(conf, name, http__fmt__add_vstr_add_sect_vstr,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_PTR_VOID,
                       VSTR_TYPE_FMT_UINT,
                       VSTR_TYPE_FMT_END));
}

static void http__app_hdr_hdr(Vstr_base *out, const char *hdr)
{
  vstr_add_cstr_buf(out, out->len, hdr);
  vstr_add_cstr_buf(out, out->len, ": ");  
}
static void http__app_hdr_eol(Vstr_base *out)
{
  vstr_add_cstr_buf(out, out->len, HTTP_EOL);
}

static void http_app_hdr_cstr(Vstr_base *out, const char *hdr, const char *data)
{
  http__app_hdr_hdr(out, hdr);
  vstr_add_cstr_buf(out, out->len, data);
  http__app_hdr_eol(out);
}

static void http_app_hdr_vstr(Vstr_base *out, const char *hdr,
                              const Vstr_base *s1, size_t vpos, size_t vlen,
                              unsigned int type)
{
  http__app_hdr_hdr(out, hdr);
  vstr_add_vstr(out, out->len, s1, vpos, vlen, type);
  http__app_hdr_eol(out);
}

static void http_app_hdr_vstr_def(Vstr_base *out, const char *hdr,
                                  const Vstr_base *s1, size_t vpos, size_t vlen)
{
  http__app_hdr_hdr(out, hdr);
  vstr_add_vstr(out, out->len, s1, vpos, vlen, VSTR_TYPE_ADD_DEF);
  http__app_hdr_eol(out);
}

static void http_app_hdr_conf_vstr(Vstr_base *out, const char *hdr,
                                   const Vstr_base *s1)
{
  http__app_hdr_hdr(out, hdr);
  vstr_add_vstr(out, out->len, s1, 1, s1->len, VSTR_TYPE_ADD_DEF);
  http__app_hdr_eol(out);
}

static void http_app_hdr_fmt(Vstr_base *out, const char *hdr,
                             const char *fmt, ...)
   VSTR__COMPILE_ATTR_FMT(3, 4);
static void http_app_hdr_fmt(Vstr_base *out, const char *hdr,
                             const char *fmt, ...)
{
  va_list ap;

  http__app_hdr_hdr(out, hdr);
  
  va_start(ap, fmt);
  vstr_add_vfmt(out, out->len, fmt, ap);
  va_end(ap);

  http__app_hdr_eol(out);
}

static void http_app_hdr_uintmax(Vstr_base *out, const char *hdr,
                                 VSTR_AUTOCONF_uintmax_t data)
{
  http__app_hdr_hdr(out, hdr);
  vstr_add_fmt(out, out->len, "%ju", data);
  http__app_hdr_eol(out);
}

void http_app_def_hdrs(struct Con *con, struct Httpd_req_data *req,
                       unsigned int http_ret_code,
                       const char *http_ret_line, time_t mtime,
                       const char *custom_content_type,
                       int use_range,
                       VSTR_AUTOCONF_uintmax_t content_length)
{
  Vstr_base *out = con->evnt->io_w;

  vstr_add_fmt(out, out->len, "%s %03u %s" HTTP_EOL,
               "HTTP/1.1", http_ret_code, http_ret_line);
  http_app_hdr_cstr(out, "Date", date_rfc1123(req->now));
  http_app_hdr_conf_vstr(out, "Server", req->policy->server_name);
  
  if (difftime(req->now, mtime) < 0) /* if mtime in future, chop it #14.29 */
    mtime = req->now;

  if (mtime)
    http_app_hdr_cstr(out, "Last-Modified", date_rfc1123(mtime));

  switch (con->keep_alive)
  {
    case HTTP_NON_KEEP_ALIVE:
      http_app_hdr_cstr(out, "Connection", "close");
      break;
      
    case HTTP_1_0_KEEP_ALIVE:
      http_app_hdr_cstr(out, "Connection", "Keep-Alive");
      /* FALLTHROUGH */
    case HTTP_1_1_KEEP_ALIVE: /* max=xxx ? */
      if (req->output_keep_alive_hdr)
        http_app_hdr_fmt(out, "Keep-Alive",
                         "%s=%u", "timeout", httpd_opts->s->idle_timeout);
      
      ASSERT_NO_SWITCH_DEF();
  }

  switch (http_ret_code)
  {
    case 200: /* OK */
      /* case 206: */ /* OK - partial -- needed? */
    case 301: case 302: case 303: case 307: /* Redirects */
    case 304: /* Not modified */
    case 406: /* Not accept - a */
      /* case 410: */ /* Gone - like 404 or 301 ? */
    case 412: /* Not accept - precondition */
    case 413: /* Too large */
    case 416: /* Bad range */
    case 417: /* Not accept - exceptation */
      if (use_range)
        http_app_hdr_cstr(out, "Accept-Ranges", "bytes");
  }
  
  if (req->vary_star)
    http_app_hdr_cstr(out, "Vary", "*");
  else if (req->vary_a || req->vary_ac || req->vary_ae || req->vary_al ||
           req->vary_rf || req->vary_ua)
  {
    const char *varies[6];
    unsigned int num = 0;
    
    if (req->vary_ua) varies[num++] = "User-Agent";
    if (req->vary_rf) varies[num++] = "Referer";
    if (req->vary_al) varies[num++] = "Accept-Language";
    if (req->vary_ae) varies[num++] = "Accept-Encoding";
    if (req->vary_ac) varies[num++] = "Accept-Charset";
    if (req->vary_a)  varies[num++] = "Accept";

    ASSERT(num && (num <= 5));
    
    http__app_hdr_hdr(out, "Vary");
    while (num > 1)
    {
      vstr_add_cstr_buf(out, out->len, varies[--num]);
      vstr_add_cstr_buf(out, out->len, ",");
    }
    
    vstr_add_cstr_buf(out, out->len, varies[0]);
    http__app_hdr_eol(out);
  }
  
  if (custom_content_type) /* possible we don't send one */
    http_app_hdr_cstr(out, "Content-Type", custom_content_type);
  else if (req->content_type_vs1 && req->content_type_len)
    http_app_hdr_vstr_def(out, "Content-Type",
                          HTTP__CONTENT_PARAMS(req, content_type));
  
  if (req->content_encoding_bzip2)
      http_app_hdr_cstr(out, "Content-Encoding", "bzip2");
  else if (req->content_encoding_gzip)
  {
    if (req->content_encoding_xgzip)
      http_app_hdr_cstr(out, "Content-Encoding", "x-gzip");
    else
      http_app_hdr_cstr(out, "Content-Encoding", "gzip");
  }
  
  http_app_hdr_uintmax(out, "Content-Length", content_length);
}
static void http_app_end_hdrs(Vstr_base *out)
{
  http__app_hdr_eol(out);
}

static void http_vlg_def(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_sect_node *h_h  = req->http_hdrs->hdr_host;
  Vstr_sect_node *h_ua = req->http_hdrs->hdr_ua;
  Vstr_sect_node *h_r  = req->http_hdrs->hdr_referer;

  vlg_info(vlg, (" host[\"$<http-esc.vstr:%p%zu%zu>\"]"
                 " UA[\"$<http-esc.vstr:%p%zu%zu>\"]"
                 " ref[\"$<http-esc.vstr:%p%zu%zu>\"]"),
           data, h_h->pos, h_h->len,    
           data, h_ua->pos, h_ua->len,
           data, h_r->pos, h_r->len);

  if (req->ver_0_9)
    vlg_info(vlg, " ver[\"HTTP/0.9]\"");
  else
    vlg_info(vlg, " ver[\"$<vstr.sect:%p%p%u>\"]", data, req->sects, 3);

  vlg_info(vlg, ": $<http-esc.vstr:%p%zu%zu>\n",
           data, req->path_pos, req->path_len);
}

void httpd_fin_fd_close(struct Con *con)
{
  con->f->len = 0;
  if (con->f->fd != -1)
    close(con->f->fd);
  con->f->fd = -1;
}

static int http_fin_req(struct Con *con, Httpd_req_data *req)
{
  Vstr_base *out = con->evnt->io_w;
  
  ASSERT(!out->conf->malloc_bad);
  http__clear_hdrs(req);

  /* Note: Not resetting con->parsed_method_ver_1_0,
   * if it's non-0.9 now it should continue to be */
  
  if (!con->keep_alive) /* all input is here */
  {
    evnt_wait_cntl_del(con->evnt, POLLIN);
    req->len = con->evnt->io_r->len; /* delete it all */
  }
  
  vstr_del(con->evnt->io_r, 1, req->len);
  
  http_req_free(req);

  evnt_put_pkt(con->evnt);
  
  evnt_fd_set_cork(con->evnt, TRUE);
  return (httpd_serv_send(con));
}

static int http_con_cleanup(struct Con *con, struct Httpd_req_data *req)
{
  con->evnt->io_r->conf->malloc_bad = FALSE;
  con->evnt->io_w->conf->malloc_bad = FALSE;
  
  http__clear_hdrs(req);
  http_req_free(req);
    
  return (FALSE);
}

static int http_fin_err_req(struct Con *con, Httpd_req_data *req)
{
  Vstr_base *out = con->evnt->io_w;

  req->content_encoding_gzip  = FALSE;
  req->content_encoding_bzip2 = FALSE;
  
  if ((req->error_code == 400) || (req->error_code == 405) ||
      (req->error_code == 413) ||
      (req->error_code == 500) || (req->error_code == 501))
    con->keep_alive = HTTP_NON_KEEP_ALIVE;
  
  ASSERT(!con->f->len);
  
  vlg_info(vlg, "ERREQ from[$<sa:%p>] err[%03u %s]",
           con->evnt->sa, req->error_code, req->error_line);
  if (req->sects->num >= 2)
    http_vlg_def(con, req);
  else
    vlg_info(vlg, "%s", "\n");

  if (req->malloc_bad)
  { /* delete all input to give more room */
    ASSERT(req->error_code == 500);
    vstr_del(con->evnt->io_r, 1, con->evnt->io_r->len);
  }
  
  if (!req->ver_0_9)
  { /* use_range is dealt with inside */
    http_app_def_hdrs(con, req, req->error_code, req->error_line,
                      httpd_opts->beg_time, "text/html",
                      req->policy->use_range, req->error_len);
    if (req->error_code == 416)
      http_app_hdr_fmt(out, "Content-Range", "%s */%ju", "bytes",
                          (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);

    if ((req->error_code == 405) || (req->error_code == 501))
      http_app_hdr_cstr(out, "Allow", "GET, HEAD, OPTIONS, TRACE");
    if ((req->error_code == 301) || (req->error_code == 302) ||
        (req->error_code == 303) || (req->error_code == 307))
    { /* make sure we haven't screwed up and allowed response splitting */
      Vstr_base *tmp = req->fname;
      
      ASSERT(!vstr_srch_cstr_chrs_fwd(tmp, 1, tmp->len, HTTP_EOL));
      http_app_hdr_vstr(out, "Location",
                           tmp, 1, tmp->len, VSTR_TYPE_ADD_ALL_BUF);
    }
    
    if (req->user_return_error_code)
    {
      if (req->cache_control_vs1)
        http_app_hdr_vstr_def(out, "Cache-Control",
                              HTTP__CONTENT_PARAMS(req, cache_control));
      if (req->expires_vs1)
        http_app_hdr_vstr_def(out, "Expires",
                              HTTP__CONTENT_PARAMS(req, expires));
      if (req->p3p_vs1)
        http_app_hdr_vstr_def(out, "P3P",
                              HTTP__CONTENT_PARAMS(req, p3p));
    }
    http_app_end_hdrs(out);
  }

  if (!req->head_op)
  {
    Vstr_base *loc = req->fname;

    switch (req->error_code)
    {
      case 301: vstr_add_fmt(out, out->len, CONF_MSG_FMT_301,
                             CONF_MSG__FMT_301_BEG,
                             loc, (size_t)1, loc->len, VSTR_TYPE_ADD_ALL_BUF,
                             CONF_MSG__FMT_30x_END); break;
      case 302: vstr_add_fmt(out, out->len, CONF_MSG_FMT_302,
                             CONF_MSG__FMT_302_BEG,
                             loc, (size_t)1, loc->len, VSTR_TYPE_ADD_ALL_BUF,
                             CONF_MSG__FMT_30x_END); break;
      case 303: vstr_add_fmt(out, out->len, CONF_MSG_FMT_303,
                             CONF_MSG__FMT_303_BEG,
                             loc, (size_t)1, loc->len, VSTR_TYPE_ADD_ALL_BUF,
                             CONF_MSG__FMT_30x_END); break;
      case 307: vstr_add_fmt(out, out->len, CONF_MSG_FMT_307,
                             CONF_MSG__FMT_307_BEG,
                             loc, (size_t)1, loc->len, VSTR_TYPE_ADD_ALL_BUF,
                             CONF_MSG__FMT_30x_END); break;
      default: vstr_add_ptr(out, out->len, req->error_msg, req->error_len);
    }
  }
  
  vlg_dbg2(vlg, "ERROR REPLY:\n$<vstr:%p%zu%zu>\n", out, (size_t)1, out->len);

  if (out->conf->malloc_bad)
    return (http_con_cleanup(con, req));
  
  return (http_fin_req(con, req));
}

static int http_fin_errmem_req(struct Con *con, struct Httpd_req_data *req)
{ /* try sending a 500 as the last msg */
  Vstr_base *out = con->evnt->io_w;
  
  /* remove anything we can to free space */
  vstr_sc_reduce(out, 1, out->len, out->len - req->orig_io_w_len);
  
  con->evnt->io_r->conf->malloc_bad = FALSE;
  con->evnt->io_w->conf->malloc_bad = FALSE;
  req->malloc_bad = TRUE;
  
  HTTPD_ERR_RET(req, 500, http_fin_err_req(con, req));
}

static int http_fin_err_close_req(struct Con *con, struct Httpd_req_data *req)
{
  httpd_fin_fd_close(con);
  return (http_fin_err_req(con, req));
}

int httpd_sc_add_default_hostname(Httpd_policy_opts *opts,
                                  Vstr_base *lfn, size_t pos)
{
  Vstr_base *d_h = opts->default_hostname;
  
  return (vstr_add_vstr(lfn, pos, d_h, 1, d_h->len, VSTR_TYPE_ADD_DEF));
}

/* turn //foo//bar// into /foo/bar/ */
int httpd_canon_path(Vstr_base *s1)
{
  size_t tmp = 0;
  
  while ((tmp = vstr_srch_cstr_buf_fwd(s1, 1, s1->len, "//")))
  {
    if (!vstr_del(s1, tmp, 1))
      return (FALSE);
  }

  return (TRUE);
}

int httpd_canon_dir_path(Vstr_base *s1)
{
  if (!httpd_canon_path(s1))
    return (FALSE);

  if (s1->len && (vstr_export_chr(s1, s1->len) != '/'))
    return (vstr_add_cstr_ptr(s1, s1->len, "/"));

  return (TRUE);
}

void httpd_req_absolute_uri(struct Con *con, Httpd_req_data *req,
                            Vstr_base *lfn, size_t pos, size_t len)
{
  struct Httpd_policy_opts *opts = req->policy;
  Vstr_base *data = con->evnt->io_r;
  Vstr_sect_node *h_h = req->http_hdrs->hdr_host;
  size_t apos = pos - 1;
  size_t alen = lfn->len;
  int has_schema   = TRUE;
  int has_abs_path = TRUE;
  int has_data     = TRUE;
  unsigned int prev_num = 0;
  
  if (!VPREFIX(lfn, pos, len, "http://"))
  {
    has_schema = FALSE;
    if (!VPREFIX(lfn, pos, len, "/")) /* relative pathname */
    {
      while (VPREFIX(lfn, pos, len, "../"))
      {
        ++prev_num;
        vstr_del(lfn, pos, strlen("../")); len -= strlen("../");
      }
      if (prev_num)
        alen = lfn->len;
      else
        has_data = !!lfn->len;
      
      has_abs_path = FALSE;
    }
  }

  if (!has_schema)
  {
    vstr_add_cstr_buf(lfn, apos, "http://");
    apos += lfn->len - alen;
    alen = lfn->len;
    if (!h_h->len)
      httpd_sc_add_default_hostname(opts, lfn, apos);
    else
      vstr_add_vstr(lfn, apos,
                    data, h_h->pos, h_h->len, VSTR_TYPE_ADD_ALL_BUF);
    apos += lfn->len - alen;
  }
    
  if (!has_abs_path)
  {
    size_t path_len = req->path_len;

    if (has_data || prev_num)
    {
      path_len -= vstr_cspn_cstr_chrs_rev(data, req->path_pos, path_len, "/");
      
      while (path_len && prev_num--)
      {
        path_len -= vstr_spn_cstr_chrs_rev(data,  req->path_pos, path_len, "/");
        path_len -= vstr_cspn_cstr_chrs_rev(data, req->path_pos, path_len, "/");
      }
      if (!path_len) path_len = 1; /* make sure there is a / at the end */
    }

    vstr_add_vstr(lfn, apos, data, req->path_pos, path_len, VSTR_TYPE_ADD_DEF);
  }
}

/* doing http://www.example.com/foo/bar where bar is a dir is bad
   because all relative links will be relative to foo, not bar.
   Also note that location must be "http://www.example.com/foo/bar/" or maybe
   "http:/foo/bar/"
*/
int http_req_chk_dir(struct Con *con, Httpd_req_data *req)
{
  Vstr_base *lfn = req->fname;
  
  ASSERT(lfn->len && (vstr_export_chr(lfn, 1) != '/') &&
         (vstr_export_chr(lfn, lfn->len) != '/'));
  
  assert(!VSUFFIX(lfn, 1, lfn->len, "/..") &&
         !VEQ(lfn, 1, lfn->len, ".."));
  
  vstr_del(lfn, 1, lfn->len);
  httpd_req_absolute_uri(con, req, lfn, 1, 0);
  
  /* we got:       http://foo/bar/
   * and so tried: http://foo/bar/index.html
   *
   * but foo/bar/index.html is a directory (fun), so redirect to:
   *               http://foo/bar/index.html/
   */
  if (VSUFFIX(lfn, 1, lfn->len, "/"))
  {
    Vstr_base *tmp = req->policy->dir_filename;
    vstr_add_vstr(lfn, lfn->len, tmp, 1, tmp->len, VSTR_TYPE_ADD_DEF);
  }
  
  vstr_add_cstr_buf(lfn, lfn->len, "/");
  
  HTTPD_ERR_301(req);
  
  return (http_fin_err_req(con, req));
}

static void http_req_split_method(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *s1 = con->evnt->io_r;
  size_t pos = 1;
  size_t len = req->len;
  size_t el = 0;
  size_t skip_len = 0;
  unsigned int orig_num = req->sects->num;
  
  el = vstr_srch_cstr_buf_fwd(s1, pos, len, HTTP_EOL);
  ASSERT(el >= pos);
  len = el - pos; /* only parse the first line */

  /* split request */
  if (!(el = vstr_srch_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
    return;
  vstr_sects_add(req->sects, pos, el - pos);
  len -= (el - pos); pos += (el - pos);

  /* just skip whitespace on method call... */
  if ((skip_len = vstr_spn_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
  { len -= skip_len; pos += skip_len; }

  if (!len)
  {
    req->sects->num = orig_num;
    return;
  }
  
  if (!(el = vstr_srch_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
  {
    vstr_sects_add(req->sects, pos, len);
    len = 0;
  }
  else
  {
    vstr_sects_add(req->sects, pos, el - pos);
    len -= (el - pos); pos += (el - pos);
    
    /* just skip whitespace on method call... */
    if ((skip_len = vstr_spn_cstr_chrs_fwd(s1, pos, len, HTTP_LWS)))
    { len -= skip_len; pos += skip_len; }
  }

  if (len)
    vstr_sects_add(req->sects, pos, len);
  else
    req->ver_0_9 = TRUE;
}

static void http_req_split_hdrs(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *s1 = con->evnt->io_r;
  size_t pos = 1;
  size_t len = req->len;
  size_t el = 0;
  size_t hpos = 0;

  ASSERT(req->sects->num >= 3);  
    
  /* skip first line */
  el = (VSTR_SECTS_NUM(req->sects, req->sects->num)->pos +
        VSTR_SECTS_NUM(req->sects, req->sects->num)->len);
  
  assert(VEQ(s1, el, CLEN(HTTP_EOL), HTTP_EOL));
  len -= (el - pos) + CLEN(HTTP_EOL); pos += (el - pos) + CLEN(HTTP_EOL);
  
  if (VPREFIX(s1, pos, len, HTTP_EOL))
    return; /* end of headers */

  ASSERT(vstr_srch_cstr_buf_fwd(s1, pos, len, HTTP_END_OF_REQUEST));
  /* split headers */
  hpos = pos;
  while ((el = vstr_srch_cstr_buf_fwd(s1, pos, len, HTTP_EOL)) != pos)
  {
    char chr = 0;
    
    len -= (el - pos) + CLEN(HTTP_EOL); pos += (el - pos) + CLEN(HTTP_EOL);

    chr = vstr_export_chr(s1, pos);
    if (chr == ' ' || chr == '\t') /* header continues to next line */
      continue;

    vstr_sects_add(req->sects, hpos, el - hpos);
    
    hpos = pos;
  }
}

#if 0
/* see rfc2616 3.3.1 -- full date parser */
static time_t http_parse_date(Vstr_base *s1, size_t pos, size_t len, time_t now)
{
  struct tm *tm = gmtime(&now);
  static const char http__date_days_shrt[4][7] =
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char http__date_days_full[10][7] =
    {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
     "Saturday"};
  static const char http__date_months[4][12] =
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
     "Sep", "Oct", "Nov", "Dec"};
  unsigned int scan = 0;

  if (!tm) return (-1);

  switch (len)
  {
    case 4 + 1 + 9 + 1 + 8 + 1 + 3: /* rfc1123 format - should be most common */
    {
      scan = 0;
      while (scan < 7)
      {
        if (VPREFIX(s1, pos, len, http__date_days_shrt[scan]))
          break;
        ++scan;
      }
      len -= 3; pos += 3;
      
      if (!VPREFIX(s1, pos, len, ", "))
        return (-1);
      len -= CLEN(", "); pos += CLEN(", ");

      tm->tm_mday = http__date_parse_2d(s1, pos, len, 1, 31);
      
      if (!VPREFIX(s1, pos, len, " "))
        return (-1);
      len -= CLEN(" "); pos += CLEN(" ");

      scan = 0;
      while (scan < 12)
      {
        if (VPREFIX(s1, pos, len, http__date_months[scan]))
          break;
        ++scan;
      }
      len -= 3; pos += 3;
      
      tm->tm_mon = scan;
      
      if (!VPREFIX(s1, pos, len, " "))
        return (-1);
      len -= CLEN(" "); pos += CLEN(" ");

      tm->tm_year = http__date_parse_4d(s1, pos, len);

      if (!VPREFIX(s1, pos, len, " "))
        return (-1);
      len -= CLEN(" "); pos += CLEN(" ");

      tm->tm_hour = http__date_parse2d(s1, pos, len, 0, 23);
      if (!VPREFIX(s1, pos, len, ":"))
        return (-1);
      len -= CLEN(":"); pos += CLEN(":");
      tm->tm_min  = http__date_parse2d(s1, pos, len, 0, 59);
      if (!VPREFIX(s1, pos, len, ":"))
        return (-1);
      len -= CLEN(":"); pos += CLEN(":");
      tm->tm_sec  = http__date_parse2d(s1, pos, len, 0, 61);
      
      if (!VPREFIX(s1, pos, len, " GMT"))
        return (-1);
    }
    return (mktime(tm));

    case  7 + 1 + 7 + 1 + 8 + 1 + 3:
    case  8 + 1 + 7 + 1 + 8 + 1 + 3:
    case  9 + 1 + 7 + 1 + 8 + 1 + 3:
    case 10 + 1 + 7 + 1 + 8 + 1 + 3: /* rfc850 format */
    {
      size_t match_len = 0;
      
      scan = 0;
      while (scan < 7)
      {
        match_len = CLEN(http__date_days_full[scan]);
        if (VPREFIX(s1, pos, len, http__date_days_full[scan]))
          break;
        ++scan;
      }
      len -= match_len; pos += match_len;

      return (-1);
    }
    return (mktime(tm));

    case  3 + 1 + 6 + 1 + 8 + 1 + 4: /* asctime format */
    {
      scan = 0;
      while (scan < 7)
      {
        if (VPREFIX(s1, pos, len, http__date_days_shrt[scan]))
          break;
        ++scan;
      }
      len -= 3; pos += 3;
      
      return (-1);
    }
    return (mktime(tm));
  }
  
  return (-1);  
}
#endif

/* gets here if the GET/HEAD response is ok, we test for caching etc. using the
 * if-* headers */
/* FALSE = 412 Precondition Failed */
static int http_response_ok(struct Con *con, struct Httpd_req_data *req,
                            unsigned int *http_ret_code,
                            const char ** http_ret_line)
{
  Vstr_base *hdrs = con->evnt->io_r;
  time_t mtime = req->f_stat->st_mtime;
  Vstr_sect_node *h_im   = req->http_hdrs->hdr_if_match;
  Vstr_sect_node *h_ims  = req->http_hdrs->hdr_if_modified_since;
  Vstr_sect_node *h_inm  = req->http_hdrs->hdr_if_none_match;
  Vstr_sect_node *h_ir   = req->http_hdrs->hdr_if_range;
  Vstr_sect_node *h_iums = req->http_hdrs->hdr_if_unmodified_since;
  Vstr_sect_node *h_r    = req->http_hdrs->multi->hdr_range;
  int h_ir_tst      = FALSE;
  int h_iums_tst    = FALSE;
  int req_if_range  = FALSE;
  int cached_output = FALSE;
  const char *date = NULL;
  
  if (difftime(req->now, mtime) < 0) /* if mtime in future, chop it #14.29 */
    mtime = req->now;

  if (req->ver_1_1 && h_iums->pos)
    h_iums_tst = TRUE;

  if (req->ver_1_1 && h_ir->pos && h_r->pos)
    h_ir_tst = TRUE;
  
  /* assumes time doesn't go backwards ... From rfc2616:
   *
   Note: When handling an If-Modified-Since header field, some
   servers will use an exact date comparison function, rather than a
   less-than function, for deciding whether to send a 304 (Not
   Modified) response. To get best results when sending an If-
   Modified-Since header field for cache validation, clients are
   advised to use the exact date string received in a previous Last-
   Modified header field whenever possible.
  */
  date = date_rfc1123(mtime);
  if (h_ims->pos && VEQ(hdrs, h_ims->pos,  h_ims->len,  date))
    cached_output = TRUE;
  if (h_iums_tst && VEQ(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst   && VEQ(hdrs, h_ir->pos,   h_ir->len,   date))
    req_if_range = TRUE;
  
  date = date_rfc850(mtime);
  if (h_ims->pos && VEQ(hdrs, h_ims->pos,  h_ims->len,  date))
    cached_output = TRUE;
  if (h_iums_tst && VEQ(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst   && VEQ(hdrs, h_ir->pos,   h_ir->len,   date))
    req_if_range = TRUE;
  
  date = date_asctime(mtime);
  if (h_ims->pos && VEQ(hdrs, h_ims->pos,  h_ims->len,  date))
    cached_output = TRUE;
  if (h_iums_tst && VEQ(hdrs, h_iums->pos, h_iums->len, date))
    return (FALSE);
  if (h_ir_tst   && VEQ(hdrs, h_ir->pos,   h_ir->len,   date))
    req_if_range = TRUE;

  if (h_ir_tst && !req_if_range)
    h_r->pos = 0;
  
  if (req->ver_1_1)
  {
    if (h_inm->pos &&  VEQ(hdrs, h_inm->pos, h_inm->len, "*"))
      cached_output = TRUE;

    if (h_im->pos  && !VEQ(hdrs, h_inm->pos, h_inm->len, "*"))
      return (FALSE);
  }
  
  if (cached_output)
  {
    req->head_op = TRUE;
    *http_ret_code = 304;
    *http_ret_line = "Not Modified";
  }
  else if (h_r->pos)
  {
    *http_ret_code = 206;
    *http_ret_line = "Partial content";
  }

  return (TRUE);
}

static void http__multi_hdr_fixup(Vstr_sect_node *hdr_ignore,
                                  Vstr_sect_node *hdr, size_t pos, size_t len)
{
  if (hdr == hdr_ignore)
    return;
  
  if (hdr->pos <= pos)
    return;

  hdr->pos += len;
}

static int http__multi_hdr_cp(Vstr_base *comb,
                              Vstr_base *data, Vstr_sect_node *hdr)
{
  size_t pos = comb->len + 1;

  if (!hdr->len)
    return (TRUE);
  
  if (!vstr_add_vstr(comb, comb->len,
                     data, hdr->pos, hdr->len, VSTR_TYPE_ADD_BUF_PTR))
    return (FALSE);

  hdr->pos = pos;
  
  return (TRUE);
}

static int http__app_multi_hdr(Vstr_base *data, struct Http_hdrs *hdrs,
                               Vstr_sect_node *hdr, size_t pos, size_t len)
{
  Vstr_base *comb = hdrs->multi->comb;
  
  ASSERT(comb);
  
  ASSERT((hdr == hdrs->multi->hdr_accept) ||
         (hdr == hdrs->multi->hdr_accept_charset) ||
         (hdr == hdrs->multi->hdr_accept_encoding) ||
         (hdr == hdrs->multi->hdr_accept_language) ||
         (hdr == hdrs->multi->hdr_connection) ||
         (hdr == hdrs->multi->hdr_range));

  ASSERT((comb == data) || (comb == hdrs->multi->combiner_store));
  
  if ((data == comb) && !hdr->pos)
  { /* Do the fast thing... */
    hdr->pos = pos;
    hdr->len = len;
    return (TRUE);
  }

  if (data == comb)
  { /* OK, so we have a crap request and need to JOIN multiple headers... */
    comb = hdrs->multi->comb = hdrs->multi->combiner_store;
  
    if (!http__multi_hdr_cp(comb, data, hdrs->multi->hdr_accept) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_accept_charset) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_accept_encoding) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_accept_language) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_connection) ||
        !http__multi_hdr_cp(comb, data, hdrs->multi->hdr_range) ||
        FALSE)
      return (FALSE);
  }
  
  if (!hdr->pos)
  {
    hdr->pos = comb->len + 1;
    hdr->len = len;
    return (vstr_add_vstr(comb, comb->len,
                          data, pos, len, VSTR_TYPE_ADD_BUF_PTR));
  }

  /* reverses the order, but that doesn't matter */
  if (!vstr_add_cstr_ptr(comb, hdr->pos - 1, ","))
    return (FALSE);
  if (!vstr_add_vstr(comb, hdr->pos - 1,
                     data, pos, len, VSTR_TYPE_ADD_BUF_PTR))
    return (FALSE);
  hdr->len += ++len;

  /* now need to "move" any hdrs after this one */
  pos = hdr->pos - 1;
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_accept,          pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_accept_charset,  pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_accept_encoding, pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_accept_language, pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_connection,      pos, len);
  http__multi_hdr_fixup(hdr, hdrs->multi->hdr_range,           pos, len);
    
  return (TRUE);
}

/* viprefix, with local knowledge */
static int http__hdr_eq(Vstr_base *data, size_t pos, size_t len,
                        const char *hdr, size_t *hdr_val_pos)
{
  if (!VIPREFIX(data, pos, len, hdr))
    return (FALSE);
  len -= CLEN(hdr); pos += CLEN(hdr);

  HTTP_SKIP_LWS(data, pos, len);
  if (!len)
    return (FALSE);
  
  if (vstr_export_chr(data, pos) != ':')
    return (FALSE);
  --len; ++pos;
  
  *hdr_val_pos = pos;
  
  return (TRUE);
}

/* remove LWS from front and end... what a craptastic std. */
static void http__hdr_fixup(Vstr_base *data, size_t *pos, size_t *len,
                            size_t hdr_val_pos)
{
  size_t tmp = 0;
  
  *len -= hdr_val_pos - *pos; *pos += hdr_val_pos - *pos;
  HTTP_SKIP_LWS(data, *pos, *len);

  /* hand coding for a HTTP_SKIP_LWS() going backwards... */
  while ((tmp = vstr_spn_cstr_chrs_rev(data, *pos, *len, HTTP_LWS)))
  {
    *len -= tmp;
  
    if (VSUFFIX(data, *pos, *len, HTTP_EOL))
      *len -= CLEN(HTTP_EOL);
  }  
}

#define HDR__EQ(x) http__hdr_eq(data, pos, len, x, &hdr_val_pos)
#define HDR__SET(h) do {                                                \
      http__hdr_fixup(data, &pos, &len, hdr_val_pos);                   \
      http_hdrs-> hdr_ ## h ->pos = pos;                                \
      http_hdrs-> hdr_ ## h ->len = len;                                \
    } while (FALSE)
#define HDR__MULTI_SET(h) do {                                          \
      http__hdr_fixup(data, &pos, &len, hdr_val_pos);                   \
      if (!http__app_multi_hdr(data, http_hdrs,                         \
                               http_hdrs->multi-> hdr_ ## h, pos, len)) \
        HTTPD_ERR_RET(req, 500, FALSE);                                 \
    } while (FALSE)

static int http__parse_hdrs(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  struct Http_hdrs *http_hdrs = req->http_hdrs;
  unsigned int num = 3; /* skip "method URI version" */
  
  while (++num <= req->sects->num)
  {
    size_t pos = VSTR_SECTS_NUM(req->sects, num)->pos;
    size_t len = VSTR_SECTS_NUM(req->sects, num)->len;
    size_t hdr_val_pos = 0;

    if (0) { /* nothing */ }
    /* single headers, isn't allowed ... we do last one wins */
    /* nothing headers ... use for logging only */
    else if (HDR__EQ("User-Agent"))          HDR__SET(ua);
    else if (HDR__EQ("Referer"))             HDR__SET(referer);

    else if (HDR__EQ("Expect"))              HDR__SET(expect);
    else if (HDR__EQ("Host"))                HDR__SET(host);
    else if (HDR__EQ("If-Match"))            HDR__SET(if_match);
    else if (HDR__EQ("If-Modified-Since"))   HDR__SET(if_modified_since);
    else if (HDR__EQ("If-None-Match"))       HDR__SET(if_none_match);
    else if (HDR__EQ("If-Range"))            HDR__SET(if_range);
    else if (HDR__EQ("If-Unmodified-Since")) HDR__SET(if_unmodified_since);

    /* allow continuations over multiple headers... *sigh* */
    else if (HDR__EQ("Accept"))              HDR__MULTI_SET(accept);
    else if (HDR__EQ("Accept-Charset"))      HDR__MULTI_SET(accept_charset);
    else if (HDR__EQ("Accept-Encoding"))     HDR__MULTI_SET(accept_encoding);
    else if (HDR__EQ("Accept-Language"))     HDR__MULTI_SET(accept_language);
    else if (HDR__EQ("Connection"))          HDR__MULTI_SET(connection);
    else if (HDR__EQ("Range"))               HDR__MULTI_SET(range);

    /* in theory 0 bytes is ok ... who cares */
    else if (HDR__EQ("Content-Length"))      HTTPD_ERR_RET(req, 413, FALSE);

    /* in theory identity is ok ... who cares */
    else if (HDR__EQ("Transfer-Encoding"))   HTTPD_ERR_RET(req, 413, FALSE);
  }

  return (TRUE);
}
#undef HDR__EQ
#undef HDR__SET
#undef HDR__MUTLI_SET

/* might have been able to do it with string matches, but getting...
 * "HTTP/1.1" = OK
 * "HTTP/1.10" = OK
 * "HTTP/1.10000000000000" = BAD
 * ...seemed not as easy. It also seems like you have to accept...
 * "HTTP / 01 . 01" as "HTTP/1.1"
 */
static int http_req_parse_version(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  size_t op_pos = VSTR_SECTS_NUM(req->sects, 3)->pos;
  size_t op_len = VSTR_SECTS_NUM(req->sects, 3)->len;
  unsigned int major = 0;
  unsigned int minor = 0;
  size_t num_len = 0;
  unsigned int num_flags = 10 | (VSTR_FLAG_PARSE_NUM_NO_BEG_PM |
                                 VSTR_FLAG_PARSE_NUM_OVERFLOW);
  
  if (!VPREFIX(data, op_pos, op_len, "HTTP"))
    HTTPD_ERR_RET(req, 400, FALSE);

  op_len -= CLEN("HTTP"); op_pos += CLEN("HTTP");
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  if (!VPREFIX(data, op_pos, op_len, "/"))
    HTTPD_ERR_RET(req, 400, FALSE);
  op_len -= CLEN("/"); op_pos += CLEN("/");
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  major = vstr_parse_uint(data, op_pos, op_len, num_flags, &num_len, NULL);
  op_len -= num_len; op_pos += num_len;
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  if (!num_len || !VPREFIX(data, op_pos, op_len, "."))
    HTTPD_ERR_RET(req, 400, FALSE);

  op_len -= CLEN("."); op_pos += CLEN(".");
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  minor = vstr_parse_uint(data, op_pos, op_len, num_flags, &num_len, NULL);
  op_len -= num_len; op_pos += num_len;
  HTTP_SKIP_LWS(data, op_pos, op_len);
  
  if (!num_len || op_len)
    HTTPD_ERR_RET(req, 400, FALSE);
  
  if (0) { } /* not allowing HTTP/0.9 here */
  else if ((major == 1) && (minor >= 1))
    req->ver_1_1 = TRUE;
  else if ((major == 1) && (minor == 0))
  { /* do nothing */ }
  else
    HTTPD_ERR_RET(req, 505, FALSE);
        
  return (TRUE);
}

#define HDR__CON_1_0_FIXUP(name, h)                                     \
    else if (VIEQ(data, pos, tmp, name))                                \
      do {                                                              \
        req -> http_hdrs -> hdr_ ## h ->pos = 0;                        \
        req -> http_hdrs -> hdr_ ## h ->len = 0;                        \
      } while (FALSE)
#define HDR__CON_1_0_MULTI_FIXUP(name, h)                               \
    else if (VIEQ(data, pos, tmp, name))                                \
      do {                                                              \
        req -> http_hdrs -> multi -> hdr_ ## h ->pos = 0;               \
        req -> http_hdrs -> multi -> hdr_ ## h ->len = 0;               \
      } while (FALSE)
static void http__parse_connection(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *data = req->http_hdrs->multi->comb;
  size_t pos = 0;
  size_t len = 0;

  pos = req->http_hdrs->multi->hdr_connection->pos;
  len = req->http_hdrs->multi->hdr_connection->len;

  if (req->ver_1_1)
    con->keep_alive = HTTP_1_1_KEEP_ALIVE;

  if (!len)
    return;

  while (len)
  {
    size_t tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len,
                                         HTTP_EOL HTTP_LWS ",");

    if (req->ver_1_1)
    { /* this is all we have to do for HTTP/1.1 ... proxies understnad it */
      if (VIEQ(data, pos, tmp, "close"))
        con->keep_alive = HTTP_NON_KEEP_ALIVE;
    }
    else if (VIEQ(data, pos, tmp, "keep-alive"))
    {
      if (req->policy->use_keep_alive_1_0)
        con->keep_alive = HTTP_1_0_KEEP_ALIVE;
    }
    /* now fixup connection headers for HTTP/1.0 proxies */
    HDR__CON_1_0_FIXUP("User-Agent",          ua);
    HDR__CON_1_0_FIXUP("Referer",             referer);
    HDR__CON_1_0_FIXUP("Expect",              expect);
    HDR__CON_1_0_FIXUP("Host",                host);
    HDR__CON_1_0_FIXUP("If-Match",            if_match);
    HDR__CON_1_0_FIXUP("If-Modified-Since",   if_modified_since);
    HDR__CON_1_0_FIXUP("If-None-Match",       if_none_match);
    HDR__CON_1_0_FIXUP("If-Range",            if_range);
    HDR__CON_1_0_FIXUP("If-Unmodified-Since", if_unmodified_since);
    
    HDR__CON_1_0_MULTI_FIXUP("Accept",          accept);
    HDR__CON_1_0_MULTI_FIXUP("Accept-Charset",  accept_charset);
    HDR__CON_1_0_MULTI_FIXUP("Accept-Encoding", accept_encoding);
    HDR__CON_1_0_MULTI_FIXUP("Accept-Language", accept_language);
    HDR__CON_1_0_MULTI_FIXUP("Range",           range);

    /* skip to end, or after next ',' */
    tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len, ",");    
    len -= tmp; pos += tmp;
    if (!len)
      break;
    
    assert(VPREFIX(data, pos, len, ","));
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
  }
}
#undef HDR__CON_1_0_FIXUP
#undef HDR__CON_1_0_MULTI_FIXUP
                                  
/* parse >= 1.0 things like, version and headers */
static int http__parse_1_x(struct Con *con, struct Httpd_req_data *req)
{
  ASSERT(!req->ver_0_9);
  
  if (!http_req_parse_version(con, req))
    return (FALSE);
  
  if (!http__parse_hdrs(con, req))
    return (FALSE);

  if (!req->policy->use_keep_alive)
    return (TRUE);

  http__parse_connection(con, req);
  
  return (TRUE);
}

/* because we only parse for a combined CRLF, and some proxies/clients parse for
 * either ... make sure we don't have enbedded ones to remove possability
 * of request splitting */
static int http__chk_single_crlf(Vstr_base *data, size_t pos, size_t len)
{
  if (vstr_srch_chr_fwd(data, pos, len, '\r') ||
      vstr_srch_chr_fwd(data, pos, len, '\n'))
    return (TRUE);

  return (FALSE);
}

/* convert a http://abcd/foo into /foo with host=abcd ...
 * also do sanity checking on the URI and host for valid characters */
static int http_parse_host(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  size_t op_pos = req->path_pos;
  size_t op_len = req->path_len;
  
  /* check for absolute URIs */
  if (VIPREFIX(data, op_pos, op_len, "http://"))
  { /* ok, be forward compatible */
    size_t tmp = CLEN("http://");
    
    op_len -= tmp; op_pos += tmp;
    tmp = vstr_srch_chr_fwd(data, op_pos, op_len, '/');
    if (!tmp)
    {
      HTTP__HDR_SET(req, host, op_pos, op_len);
      op_len = 1;
      --op_pos;
    }
    else
    { /* found end of host ... */
      size_t host_len = tmp - op_pos;
      
      HTTP__HDR_SET(req, host, op_pos, host_len);
      op_len -= host_len; op_pos += host_len;
    }
    assert(VPREFIX(data, op_pos, op_len, "/"));
  }

  /* HTTP/1.1 requires a host -- allow blank hostnames */
  if (req->ver_1_1 && !req->http_hdrs->hdr_host->pos)
    return (FALSE);
  
  if (req->http_hdrs->hdr_host->len)
  { /* check host looks valid ... header must exist, but can be empty */
    size_t pos = req->http_hdrs->hdr_host->pos;
    size_t len = req->http_hdrs->hdr_host->len;
    size_t tmp = 0;

    /* leaving out most checks for ".." or invalid chars in hostnames etc.
       as the default filename checks should catch them
     */

    /*  Check for Host header with extra / ...
     * Ie. only allow a single directory name.
     *  We could just leave this (it's not a security check, /../ is checked
     * for at filepath time), but I feel like being anal and this way there
     * aren't multiple urls to a single path. */
    if (vstr_srch_chr_fwd(data, pos, len, '/'))
      return (FALSE);

    if (http__chk_single_crlf(data, pos, len))
      return (FALSE);

    if ((tmp = vstr_srch_chr_fwd(data, pos, len, ':')))
    { /* NOTE: not sure if we have to 400 if the port doesn't match
       * or if it's an "invalid" port number (Ie. == 0 || > 65535) */
      len -= tmp - pos; pos = tmp;

      /* if it's port 80, pretend it's not there */
      if (VEQ(data, pos, len, ":80") || VEQ(data, pos, len, ":"))
        req->http_hdrs->hdr_host->len -= len;
      else
      {
        len -= 1; pos += 1; /* skip the ':' */
        if (vstr_spn_cstr_chrs_fwd(data, pos, len, "0123456789") != len)
          return (FALSE);
      }
    }
  }

  if (http__chk_single_crlf(data, op_pos, op_len))
    return (FALSE);
  
  /* uri#fragment ... craptastic clients pass this and assume it is ignored */
  if (req->policy->remove_url_frag)
    op_len = vstr_cspn_cstr_chrs_fwd(data, op_pos, op_len, "#");
  /* uri?foo ... This is "ok" to pass, however if you move dynamic
   * resources to static ones you need to do this */
  if (req->policy->remove_url_query)
    op_len = vstr_cspn_cstr_chrs_fwd(data, op_pos, op_len, "?");
  
  req->path_pos = op_pos;
  req->path_len = op_len;
  
  return (TRUE);
}

static void http__parse_skip_blanks(Vstr_base *data,
                                    size_t *passed_pos, size_t *passed_len)
{
  size_t pos = *passed_pos;
  size_t len = *passed_len;
  
  HTTP_SKIP_LWS(data, pos, len);
  while (VPREFIX(data, pos, len, ",")) /* http crack */
  {
    len -= CLEN(","); pos += CLEN(",");
    HTTP_SKIP_LWS(data, pos, len);
  }

  *passed_pos = pos;
  *passed_len = len;
}

/* Allow...
   bytes=NUM-NUM
   bytes=-NUM
   bytes=NUM-
   ...and due to LWS etc. http crapola parsing, even...
   bytes = , , NUM - NUM , ,
   ...not allowing multiple ranges at once though, as multipart/byteranges
   is too much crack, I think this is stds. compliant.
 */
static int http_parse_range(struct Httpd_req_data *req,
                            VSTR_AUTOCONF_uintmax_t *range_beg,
                            VSTR_AUTOCONF_uintmax_t *range_end)
{
  Vstr_base *data     = req->http_hdrs->multi->comb;
  Vstr_sect_node *h_r = req->http_hdrs->multi->hdr_range;
  size_t pos = h_r->pos;
  size_t len = h_r->len;
  VSTR_AUTOCONF_uintmax_t fsize = req->f_stat->st_size;
  unsigned int num_flags = 10 | (VSTR_FLAG_PARSE_NUM_NO_BEG_PM |
                                 VSTR_FLAG_PARSE_NUM_OVERFLOW);
  size_t num_len = 0;
  
  *range_beg = 0;
  *range_end = 0;
  
  if (!VPREFIX(data, pos, len, "bytes"))
    return (0);
  len -= CLEN("bytes"); pos += CLEN("bytes");

  HTTP_SKIP_LWS(data, pos, len);
  
  if (!VPREFIX(data, pos, len, "="))
    return (0);
  len -= CLEN("="); pos += CLEN("=");

  http__parse_skip_blanks(data, &pos, &len);
  
  if (VPREFIX(data, pos, len, "-"))
  { /* num bytes at end */
    VSTR_AUTOCONF_uintmax_t tmp = 0;

    len -= CLEN("-"); pos += CLEN("-");
    HTTP_SKIP_LWS(data, pos, len);

    tmp = vstr_parse_uintmax(data, pos, len, num_flags, &num_len, NULL);
    len -= num_len; pos += num_len;
    if (!num_len)
      return (0);

    if (!tmp)
      return (416);
    
    if (tmp >= fsize)
      return (0);
    
    *range_beg = fsize - tmp;
    *range_end = fsize - 1;
  }
  else
  { /* offset - [end] */
    *range_beg = vstr_parse_uintmax(data, pos, len, num_flags, &num_len, NULL);
    len -= num_len; pos += num_len;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "-"))
      return (0);
    len -= CLEN("-"); pos += CLEN("-");
    HTTP_SKIP_LWS(data, pos, len);

    if (!len || VPREFIX(data, pos, len, ","))
      *range_end = fsize - 1;
    else
    {
      *range_end = vstr_parse_uintmax(data, pos, len, num_flags, &num_len,NULL);
      len -= num_len; pos += num_len;
      if (!num_len)
        return (0);
      
      if (*range_end >= fsize)
        *range_end = fsize - 1;
    }
    
    if ((*range_beg >= fsize) || (*range_beg > *range_end))
      return (416);
    
    if ((*range_beg == 0) && 
        (*range_end == (fsize - 1)))
      return (0);
  }

  http__parse_skip_blanks(data, &pos, &len);
  
  if (len) /* after all that, ignore if there is more than one range */
    return (0);

  return (200);
}

#define HTTP__PARSE_CHK_RET_OK() do {                   \
      HTTP_SKIP_LWS(data, pos, len);                    \
                                                        \
      if (!len ||                                       \
          VPREFIX(data, pos, len, ",") ||               \
          (allow_more && VPREFIX(data, pos, len, ";"))) \
      {                                                 \
        *passed_pos = pos;                              \
        *passed_len = len;                              \
                                                        \
        return (TRUE);                                  \
      }                                                 \
    } while (FALSE)

/* What is the quality parameter, value between 0 and 1000 inclusive.
 * returns TRUE on success, FALSE on failure. */
static int http_parse_quality(Vstr_base *data,
                              size_t *passed_pos, size_t *passed_len,
                              int allow_more, unsigned int *val)
{
  size_t pos = *passed_pos;
  size_t len = *passed_len;
  
  ASSERT(val);
  
  *val = 1000;
  
  HTTP_SKIP_LWS(data, pos, len);

  *passed_pos = pos;
  *passed_len = len;
  
  if (!len || VPREFIX(data, pos, len, ","))
    return (TRUE);
  else if (VPREFIX(data, pos, len, ";"))
  {
    int lead_zero = FALSE;
    unsigned int num_len = 0;
    unsigned int parse_flags = VSTR_FLAG02(PARSE_NUM, NO_BEG_PM, NO_NEGATIVE);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "q"))
      return (!!allow_more);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
    
    if (!VPREFIX(data, pos, len, "="))
      return (!!allow_more);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);

    /* if it's 0[.00?0?] TRUE, 0[.\d\d?\d?] or 1[.00?0?] is FALSE */
    if (!(lead_zero = VPREFIX(data, pos, len, "0")) &&
        !VPREFIX(data, pos, len, "1"))
      return (FALSE);
    *val = (!lead_zero) * 1000;
    
    len -= 1; pos += 1;
    
    HTTP__PARSE_CHK_RET_OK();

    if (!VPREFIX(data, pos, len, "."))
      return (FALSE);
    
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);

    *val += vstr_parse_uint(data, pos, len, 10 | parse_flags, &num_len, NULL);
    if (!num_len || (num_len > 3) || (*val > 1000))
      return (FALSE);
    if (num_len < 3) *val *= 10;
    if (num_len < 2) *val *= 10;
    ASSERT(*val <= 1000);
    
    len -= num_len; pos += num_len;
    
    HTTP__PARSE_CHK_RET_OK();
  }
  
  return (FALSE);
}
#undef HTTP__PARSE_CHK_RET_OK

static int http_parse_accept_encoding(struct Httpd_req_data *req)
{
  Vstr_base *data = req->http_hdrs->multi->comb;
  size_t pos = 0;
  size_t len = 0;
  unsigned int gzip_val     = 1001;
  unsigned int bzip2_val    = 1001;
  unsigned int identity_val = 1001;
  unsigned int star_val     = 1001;
  
  pos = req->http_hdrs->multi->hdr_accept_encoding->pos;
  len = req->http_hdrs->multi->hdr_accept_encoding->len;

  if (!req->policy->use_err_406 && !req->allow_accept_encoding)
    return (FALSE);
  req->vary_ae = TRUE;
  
  if (!len)
    return (FALSE);
  
  req->content_encoding_xgzip = FALSE;
  
  while (len)
  {
    size_t tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len,
                                         HTTP_EOL HTTP_LWS ";,");
    
    if (0) { }
    else if (VEQ(data, pos, tmp, "identity"))
    {
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, FALSE, &identity_val))
        return (FALSE);
    }
    else if (req->allow_accept_encoding && VEQ(data, pos, tmp, "gzip"))
    {
      len -= tmp; pos += tmp;
      req->content_encoding_xgzip = FALSE;
      if (!http_parse_quality(data, &pos, &len, FALSE, &gzip_val))
        return (FALSE);
    }
    else if (req->allow_accept_encoding && VEQ(data, pos, tmp, "bzip2"))
    {
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, FALSE, &bzip2_val))
        return (FALSE);
    }
    else if (req->allow_accept_encoding && VEQ(data, pos, tmp, "x-gzip"))
    {
      len -= tmp; pos += tmp;
      req->content_encoding_xgzip = TRUE;
      if (!http_parse_quality(data, &pos, &len, FALSE, &gzip_val))
        return (FALSE);
      gzip_val = 1000; /* ignore quality on x-gzip - just parse for errors */
    }
    else if (VEQ(data, pos, tmp, "*"))
    { /* "*;q=0,gzip" means TRUE ... and "*;q=1.0,gzip;q=0" means FALSE */
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, FALSE, &star_val))
        return (FALSE);
    }
    else
    {
      len -= tmp; pos += tmp;
    }
    
    /* skip to end, or after next ',' */
    tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len, ",");    
    len -= tmp; pos += tmp;
    if (!len)
      break;
    assert(VPREFIX(data, pos, len, ","));
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
  }

  if (!req->allow_accept_encoding)
  {
    gzip_val  = 0;
    bzip2_val = 0;
  }
  
  if (gzip_val     == 1001) gzip_val     = star_val;
  if (bzip2_val    == 1001) bzip2_val    = star_val;
  if (identity_val == 1001) identity_val = star_val;

  if (gzip_val     == 1001) gzip_val     = 0;
  if (bzip2_val    == 1001) bzip2_val    = 0;
  if (identity_val == 1001) identity_val = 1;

  if (!identity_val)
    req->content_encoding_identity = FALSE;
  
  if ((identity_val > gzip_val) && (identity_val > bzip2_val))
    return (FALSE);
  
  req->content_encoding_gzip  = !!gzip_val;
  req->content_encoding_bzip2 = !!bzip2_val;
  
  return (req->content_encoding_gzip || req->content_encoding_bzip2);
}

/* try to use gzip content-encoding on entity */
static int http__try_encoded_content(struct Con *con, Httpd_req_data *req,
                                     const char *zip_ext)
{
  const char *fname_cstr = NULL;
  int fd = -1;
  int ret = FALSE;
  
  vstr_add_cstr_ptr(req->fname, req->fname->len, zip_ext);

  fname_cstr = vstr_export_cstr_ptr(req->fname, 1, req->fname->len);
  if (!fname_cstr)
    vlg_warn(vlg, "Failed to export cstr for '%s'\n", zip_ext);
  else if ((fd = io_open_nonblock(fname_cstr)) == -1)
    vstr_sc_reduce(req->fname, 1, req->fname->len, strlen(zip_ext));
  else
  {
    struct stat64 f_stat[1];
    
    if (fstat64(fd, f_stat) == -1)
      vlg_warn(vlg, "fstat: %m\n");
    else if ((req->policy->use_public_only && !(f_stat->st_mode & S_IROTH)) ||
             (S_ISDIR(f_stat->st_mode)) || (!S_ISREG(f_stat->st_mode)) ||
             (req->f_stat->st_mtime >  f_stat->st_mtime) ||
             (req->f_stat->st_size  <= f_stat->st_size))
    { /* ignore the zip else */ }
    else
    {
      /* swap, close the old fd (later) and use the new */
      SWAP_TYPE(con->f->fd, fd, int);
      
      ASSERT(con->f->len == (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);
      /* _only_ copy the new size over, mtime etc. is from the original file */
      con->f->len = req->f_stat->st_size = f_stat->st_size;
      ret = TRUE;
    }
    close(fd);
  }

  return (ret);
}  

static void httpd_serv_conf_file(struct Con *con, struct Httpd_req_data *req,
                                 VSTR_AUTOCONF_uintmax_t f_off,
                                 VSTR_AUTOCONF_uintmax_t f_len)
{
  ASSERT(!req->head_op);
  ASSERT(!req->use_mmap);
  
  if (!con->use_sendfile)
    return;
  
  con->f->off = f_off;
  con->f->len = f_len;
}

static void httpd_serv_call_mmap(struct Con *con, struct Httpd_req_data *req,
                                 VSTR_AUTOCONF_uintmax_t off,
                                 VSTR_AUTOCONF_uintmax_t f_len)
{
  static long pagesz = 0;
  Vstr_base *data = con->evnt->io_r;
  VSTR_AUTOCONF_uintmax_t mmoff = off;
  VSTR_AUTOCONF_uintmax_t mmlen = f_len;
  
  ASSERT(!req->f_mmap || !req->f_mmap->len);
  ASSERT(!req->use_mmap);
  ASSERT(!req->head_op);

  if (con->use_sendfile)
    return;

  if (!pagesz)
    pagesz = sysconf(_SC_PAGESIZE);
  if (pagesz == -1)
    req->policy->use_mmap = FALSE;

  if (!req->policy->use_mmap ||
      (f_len < HTTP_CONF_MMAP_LIMIT_MIN) || (f_len > HTTP_CONF_MMAP_LIMIT_MAX))
    return;
  
  /* mmap offset needs to be aligned - so tweak offset before and after */
  mmoff /= pagesz;
  mmoff *= pagesz;
  ASSERT(mmoff <= off);
  mmlen += off - mmoff;

  if (!req->f_mmap && !(req->f_mmap = vstr_make_base(data->conf)))
    VLG_WARN_RET_VOID((vlg, /* fall back to read */
                       "failed to allocate mmap Vstr.\n"));
  ASSERT(!req->f_mmap->len);
  
  if (!vstr_sc_mmap_fd(req->f_mmap, 0, con->f->fd, mmoff, mmlen, NULL))
    VLG_WARN_RET_VOID((vlg, /* fall back to read */
                       "mmap($<http-esc.vstr:%p%zu%zu>,"
                            "(%ju,%ju)->(%ju,%ju)): %m\n",
                       req->fname, 1, req->fname->len,
                       off, f_len, mmoff, mmlen));

  req->use_mmap = TRUE;  
  vstr_del(req->f_mmap, 1, off - mmoff);
    
  ASSERT(req->f_mmap->len == f_len);
  
  /* possible range request successful, alter response length */
  con->f->len = f_len;
}

static void httpd_serv_call_seek(struct Con *con, struct Httpd_req_data *req,
                                 VSTR_AUTOCONF_uintmax_t f_off,
                                 VSTR_AUTOCONF_uintmax_t f_len,
                                 unsigned int *http_ret_code,
                                 const char ** http_ret_line)
{
  ASSERT(!req->head_op);
  
  if (req->use_mmap || con->use_sendfile)
    return;

  if (f_off && f_len && (lseek64(con->f->fd, f_off, SEEK_SET) == -1))
  { /* this should be impossible for normal files AFAIK */
    vlg_warn(vlg, "lseek($<http-esc.vstr:%p%zu%zu>,off=%ju): %m\n",
             req->fname, 1, req->fname->len, f_off);
    /* opts->use_range - turn off? */
    req->http_hdrs->multi->hdr_range->pos = 0;
    *http_ret_code = 200;
    *http_ret_line = "OK - Range Failed";
    return;
  }
  
  con->f->len = f_len;

  return;
}

static void httpd_serv_call_file_init(struct Con *con, Httpd_req_data *req,
                                      VSTR_AUTOCONF_uintmax_t f_off,
                                      VSTR_AUTOCONF_uintmax_t f_len,
                                      unsigned int *http_ret_code,
                                      const char ** http_ret_line)
{
  if (req->head_op)
    con->f->len = f_len;
  else
  {
    httpd_serv_conf_file(con, req, f_off, f_len);
    httpd_serv_call_mmap(con, req, f_off, f_len);
    httpd_serv_call_seek(con, req, f_off, f_len, http_ret_code, http_ret_line);
  }
}

static int http_req_1_x(struct Con *con, struct Httpd_req_data *req,
                        unsigned int *http_ret_code,
                        const char **http_ret_line)
{
  Vstr_base *out = con->evnt->io_w;
  VSTR_AUTOCONF_uintmax_t range_beg = 0;
  VSTR_AUTOCONF_uintmax_t range_end = 0;
  VSTR_AUTOCONF_uintmax_t f_len     = 0;
  Vstr_sect_node *h_r = req->http_hdrs->multi->hdr_range;
  
  if (req->ver_1_1 && req->http_hdrs->hdr_expect->len)
    /* I'm pretty sure we can ignore 100-continue, as no request will
     * have a body */
    HTTPD_ERR_RET(req, 417, FALSE);
          
  /* Might normally add "!req->head_op && ..." but
   * http://www.w3.org/TR/chips/#gl6 says that's bad */
  if (http_parse_accept_encoding(req))
  {
    if ( req->content_encoding_bzip2 &&
        !http__try_encoded_content(con, req, ".bz2"))
      req->content_encoding_bzip2 = FALSE;
    if (!req->content_encoding_bzip2 && req->content_encoding_gzip &&
        !http__try_encoded_content(con, req, ".gz"))
      req->content_encoding_gzip = FALSE;
  }
  
  if (req->policy->use_err_406 &&
      !req->content_encoding_identity && !req->content_encoding_gzip)
    HTTPD_ERR_RET(req, 406, FALSE);
  
  if (h_r->pos)
  {
    int ret_code = 0;

    if (!(req->policy->use_range &&
	  (req->ver_1_1 || req->policy->use_range_1_0)))
      h_r->pos = 0;
    else if (!(ret_code = http_parse_range(req, &range_beg, &range_end)))
      h_r->pos = 0;
    ASSERT(!ret_code || (ret_code == 200) || (ret_code == 416));
    if (ret_code == 416)
    {
      if (!req->http_hdrs->hdr_if_range->pos)
        HTTPD_ERR_RET(req, 416, FALSE);
      h_r->pos = 0;
    }
  }

  if (!http_response_ok(con, req, http_ret_code, http_ret_line))
    HTTPD_ERR_RET(req, 412, FALSE);

  if (h_r->pos)
    f_len = (range_end - range_beg) + 1;
  else
  {
    range_beg = 0;
    f_len     = con->f->len;
  }
  ASSERT(con->f->len >= f_len);

  httpd_serv_call_file_init(con, req, range_beg, f_len,
                            http_ret_code, http_ret_line);

  http_app_def_hdrs(con, req, *http_ret_code, *http_ret_line,
                    req->f_stat->st_mtime, NULL, req->policy->use_range,
                    con->f->len);
  if (h_r->pos)
    http_app_hdr_fmt(out, "Content-Range",
                        "%s %ju-%ju/%ju", "bytes", range_beg, range_end,
                        (VSTR_AUTOCONF_uintmax_t)req->f_stat->st_size);
  if (req->content_location_vs1)
    http_app_hdr_vstr_def(out, "Content-Location",
                          HTTP__CONTENT_PARAMS(req, content_location));
  if (req->content_md5_vs1)
    http_app_hdr_vstr_def(out, "Content-MD5",
                          HTTP__CONTENT_PARAMS(req, content_md5));
  if (req->cache_control_vs1)
    http_app_hdr_vstr_def(out, "Cache-Control",
                          HTTP__CONTENT_PARAMS(req, cache_control));
  if (req->expires_vs1)
    http_app_hdr_vstr_def(out, "Expires", HTTP__CONTENT_PARAMS(req, expires));
  if (req->p3p_vs1)
    http_app_hdr_vstr_def(out, "P3P",
                          HTTP__CONTENT_PARAMS(req, p3p));
  /* TODO: ETag */
  
  http_app_end_hdrs(out);
  
  return (TRUE);
}

/* skip a quoted string, or fail on syntax error */
static int http__skip_quoted_string(Vstr_base *data, size_t *pos, size_t *len)
{
  size_t tmp = 0;

  assert(VPREFIX(data, *pos, *len, "\""));
  *len -= 1; *pos += 1;

  while (TRUE)
  {
    tmp = vstr_cspn_cstr_chrs_fwd(data, *pos, *len, "\"\\");
    *len -= tmp; *pos += tmp;
    if (!*len)
      return (FALSE);
    
    if (vstr_export_chr(data, *pos) == '"')
      goto found;
    assert(VPREFIX(data, *pos, *len, "\\"));
    if (*len <= 2) /* must be at least <\X"> */
      return (FALSE);
    *len -= 2; *pos += 2;
  }

 found:
  *len -= 1; *pos += 1;
  HTTP_SKIP_LWS(data, *pos, *len);
  return (TRUE);
}

/* skip, or fail on syntax error */
static int http__skip_parameters(Vstr_base *data, size_t *pos, size_t *len)
{
  while (*len && (vstr_export_chr(data, *pos) != ','))
  { /* skip parameters */
    size_t tmp = 0;
    
    if (vstr_export_chr(data, *pos) != ';')
      return (FALSE); /* syntax error */
    
    *len -= 1; *pos += 1;
    HTTP_SKIP_LWS(data, *pos, *len);
    tmp = vstr_cspn_cstr_chrs_fwd(data, *pos, *len, ";,=");
    *len -= tmp; *pos += tmp;
    if (!*len)
      break;
    
    switch (vstr_export_chr(data, *pos))
    {
      case ';': break;
      case ',': break;
        
      case '=': /* skip parameter value */
        *len -= 1; *pos += 1;
        HTTP_SKIP_LWS(data, *pos, *len);
        if (!*len)
          return (FALSE); /* syntax error */
        if (vstr_export_chr(data, *pos) == '"')
        {
          if (!http__skip_quoted_string(data, pos, len))
            return (FALSE); /* syntax error */
        }
        else
        {
          tmp = vstr_cspn_cstr_chrs_fwd(data, *pos, *len, ";,");
          *len -= tmp; *pos += tmp;
        }
        break;
    }
  }

  return (TRUE);
}

/* returns quality of the passed content-type in the "Accept:" header,
 * if it isn't there or we get a syntax error we return 1001 for not avilable */
unsigned int http_parse_accept(Httpd_req_data *req,
                               const Vstr_base *ct_vs1,
                               size_t ct_pos, size_t ct_len)
{
  Vstr_base *data = req->http_hdrs->multi->comb;
  size_t pos = 0;
  size_t len = 0;
  unsigned int quality = 1001;
  int done_sub_type = FALSE;
  size_t ct_sub_len = 0;
  
  pos = req->http_hdrs->multi->hdr_accept->pos;
  len = req->http_hdrs->multi->hdr_accept->len;
  
  if (!len) /* no accept == accept all */
    return (1000);

  ASSERT(ct_vs1);
    
  if (!(ct_sub_len = vstr_srch_chr_fwd(ct_vs1, ct_pos, ct_len, '/')))
  { /* it's too weird, blank it */
    if (ct_vs1 == req->content_type_vs1)
      req->content_type_vs1 = NULL;
    return (1);
  }
  ct_sub_len = vstr_sc_posdiff(ct_pos, ct_sub_len);
  
  while (len)
  {
    size_t tmp = vstr_cspn_cstr_chrs_fwd(data, pos, len,
                                         HTTP_EOL HTTP_LWS ";,");
    
    if (0) { }
    else if (vstr_cmp_eq(data, pos, tmp, ct_vs1, ct_pos, ct_len))
    { /* full match */
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, TRUE, &quality))
        return (1);
      return (quality);
    }
    else if ((tmp == (ct_sub_len + 1)) &&
             vstr_cmp_eq(data, pos, ct_sub_len, ct_vs1, ct_pos, ct_sub_len) &&
             (vstr_export_chr(data, vstr_sc_poslast(pos, tmp)) == '*'))
    { /* sub match */
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, TRUE, &quality))
        return (1);
      done_sub_type = TRUE;
    }
    else if (!done_sub_type && VEQ(data, pos, tmp, "*/*"))
    { /* "*;q=0,gzip" means TRUE ... and "*;q=1,gzip;q=0" means FALSE */
      len -= tmp; pos += tmp;
      if (!http_parse_quality(data, &pos, &len, TRUE, &quality))
        return (1);
    }
    else
    {
      len -= tmp; pos += tmp;
      HTTP_SKIP_LWS(data, pos, len);
    }

    if (!http__skip_parameters(data, &pos, &len))
      return (1);
    if (!len)
      break;
    
    assert(VPREFIX(data, pos, len, ","));
    len -= 1; pos += 1;
    HTTP_SKIP_LWS(data, pos, len);
  }

  ASSERT(quality <= 1001);
  if (quality == 1001)
    return (0);
  
  return (quality);
}

/* Lookup content type for filename, If this lookup "fails" it still returns
 * the default content-type. So we just have to determine if we want to use
 * it or not. Can also return "content-types" like /404/ which returns a 404
 * error for the request */
int http_req_content_type(Httpd_req_data *req)
{
  const Vstr_base *vs1 = NULL;
  size_t     pos = 0;
  size_t     len = 0;

  if (req->content_type_vs1) /* manually set */
    return (TRUE);

  mime_types_match(req->policy->mime_types,
                   req->fname, 1, req->fname->len, &vs1, &pos, &len);
  if (!len)
  {
    req->parse_accept = FALSE;
    return (TRUE);
  }
  
  if ((vstr_export_chr(vs1, pos) == '/') && (len > 2) &&
      (vstr_export_chr(vs1, vstr_sc_poslast(pos, len)) == '/'))
  {
    size_t num_len = 1;

    len -= 2;
    ++pos;
    req->user_return_error_code = TRUE;
    switch (vstr_parse_uint(vs1, pos, len, 0, &num_len, NULL))
    {
      case 400: if (num_len == len) HTTPD_ERR_RET(req, 400, FALSE);
      case 403: if (num_len == len) HTTPD_ERR_RET(req, 403, FALSE);
      case 404: if (num_len == len) HTTPD_ERR_RET(req, 404, FALSE);
      case 410: if (num_len == len) HTTPD_ERR_RET(req, 410, FALSE);
      case 500: if (num_len == len) HTTPD_ERR_RET(req, 500, FALSE);
      case 503: if (num_len == len) HTTPD_ERR_RET(req, 503, FALSE);
        
      default: /* just ignore any other content */
        req->user_return_error_code = FALSE;
        return (TRUE);
    }
  }

  req->content_type_vs1 = vs1;
  req->content_type_pos = pos;
  req->content_type_len = len;

  return (TRUE);
}

static int http__policy_req(struct Con *con, Httpd_req_data *req)
{
  if (!httpd_policy_request(con, req,
                            httpd_opts->conf, httpd_opts->match_request))
  {
    Vstr_base *s1 = httpd_opts->conf->tmp;
    if (s1->len)
      vlg_info(vlg, "CONF-ERR from[$<sa:%p>]: backtrace: $<vstr:%p%zu%zu>\n",
               con->evnt->sa, s1, (size_t)1, s1->len);
    return (TRUE);
  }
  
  if (con->evnt->flag_q_closed)
  {
    Vstr_base *s1 = req->policy->policy_name;
    vlg_info(vlg, "BLOCKED from[$<sa:%p>]: HTTPD policy $<vstr:%p%zu%zu>\n",
             con->evnt->sa, s1, (size_t)1, s1->len);
    return (FALSE);
  }

  return (TRUE);
}

static int http__conf_req(struct Con *con, Httpd_req_data *req)
{
  Conf_parse *conf = NULL;
  
  if (!(conf = conf_parse_make(NULL)))
    return (FALSE);
  
  if (!httpd_conf_req_parse_file(conf, con, req))
  {
    Vstr_base *s1 = conf->tmp;
    if (conf->tmp->len)
      vlg_info(vlg, "CONF-ERR from[$<sa:%p>]: backtrace: $<vstr:%p%zu%zu>\n",
               con->evnt->sa, s1, (size_t)1, s1->len);
    conf_parse_free(conf);
    return (TRUE);
  }
  conf_parse_free(conf);
  
  if (req->direct_uri)
  {
    vlg_info(vlg, "CONF-ERR from[$<sa:%p>]: Direct URI.\n", con->evnt->sa);
    HTTPD_ERR_RET(req, 500, TRUE);
  }
  
  return (TRUE);
}

int http_req_op_get(struct Con *con, Httpd_req_data *req)
{ /* GET or HEAD ops */
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *out  = con->evnt->io_w;
  Vstr_base *fname = req->fname;
  const char *fname_cstr = NULL;
  unsigned int http_ret_code = 200;
  const char * http_ret_line = "OK";
  int vary_a = FALSE;
  
  if (fname->conf->malloc_bad)
    goto malloc_err;

  assert(VPREFIX(fname, 1, fname->len, "/"));

  /* final act of vengance, before policy */
  if (vstr_srch_cstr_buf_fwd(data, req->path_pos, req->path_len, "//"))
  { /* in theory we can skip this if there is no policy */
    vstr_sub_vstr(fname, 1, fname->len, data, req->path_pos, req->path_len, 0);
    if (!httpd_canon_path(fname))
      goto malloc_err;
    httpd_req_absolute_uri(con, req, fname, 1, fname->len);
    HTTPD_ERR_301(req);
  
    return (http_fin_err_req(con, req));
  }
  
  if (!http__policy_req(con, req))
    return (FALSE);
  if (req->error_code)
    return (http_fin_err_req(con, req));
  
  if (!http__conf_req(con, req))
    return (FALSE);
  if (req->error_code)
    return (http_fin_err_req(con, req));
    
  if (!http_req_content_type(req))
    return (http_fin_err_req(con, req));

  vary_a = req->vary_a;
  if (req->parse_accept)
  {
    if (FALSE) /* does a 406 count, I don't think so ?? */
      vary_a = TRUE;
    if (!http_parse_accept(req, HTTP__CONTENT_PARAMS(req, content_type)))
      HTTPD_ERR_RET(req, 406, http_fin_err_req(con, req));
  }
  
  /* Add the document root now, this must be at least . so
   * "///foo" becomes ".///foo" ... this is done now
   * so nothing has to deal with document_root values */
  if (!req->skip_document_root)
  {
    Vstr_base *tmp = req->policy->document_root;
    vstr_add_vstr(fname, 0, tmp, 1, tmp->len, VSTR_TYPE_ADD_BUF_PTR);
  }
  
  fname_cstr = vstr_export_cstr_ptr(fname, 1, fname->len);
  
  if (fname->conf->malloc_bad)
    goto malloc_err;

  ASSERT(con->f->fd == -1);
  if ((con->f->fd = io_open_nonblock(fname_cstr)) == -1)
  {
    if (0) { }
    else if (!req->direct_filename && (errno == EISDIR)) /* don't allow */
      return (http_req_chk_dir(con, req));
    else if (errno == EISDIR) /* don't allow */
      HTTPD_ERR(req, 404);
    else if (errno == EACCES)
      HTTPD_ERR(req, 403);
    else if ((errno == ENOENT) ||
             (errno == ENODEV) ||
             (errno == ENXIO) ||
             (errno == ELOOP) ||
             (errno == ENOTDIR) ||
             (errno == ENAMETOOLONG) || /* 414 ? */
             FALSE)
      HTTPD_ERR(req, 404);
    else
      HTTPD_ERR(req, 500);
    
    return (http_fin_err_req(con, req));
  }
  if (fstat64(con->f->fd, req->f_stat) == -1)
    HTTPD_ERR_RET(req, 500, http_fin_err_close_req(con, req));
  if (req->policy->use_public_only && !(req->f_stat->st_mode & S_IROTH))
    HTTPD_ERR_RET(req, 403, http_fin_err_close_req(con, req));
  
  if (S_ISDIR(req->f_stat->st_mode))
  {
    if (req->direct_filename) /* don't allow */
      HTTPD_ERR_RET(req, 404, http_fin_err_close_req(con, req));
    httpd_fin_fd_close(con);
    return (http_req_chk_dir(con, req));
  }
  if (!S_ISREG(req->f_stat->st_mode))
    HTTPD_ERR_RET(req, 403, http_fin_err_close_req(con, req));
  
  con->f->len = req->f_stat->st_size;
  req->vary_a = vary_a;
  
  if (req->ver_0_9)
  {
    httpd_serv_call_file_init(con, req, 0, con->f->len,
                              &http_ret_code, &http_ret_line);
    http_ret_line = "OK - HTTP/0.9";
  }
  else if (!http_req_1_x(con, req, &http_ret_code, &http_ret_line))
    return (http_fin_err_close_req(con, req));
  
  if (out->conf->malloc_bad)
    goto malloc_close_err;
  
  vlg_dbg2(vlg, "REPLY:\n$<vstr:%p%zu%zu>\n", out, (size_t)1, out->len);
  
  if (req->use_mmap && !vstr_mov(con->evnt->io_w, con->evnt->io_w->len,
                                 req->f_mmap, 1, req->f_mmap->len))
    goto malloc_close_err;

  /* req->head_op is set for 304 returns */
  vlg_info(vlg, "REQ $<vstr.sect:%p%p%u> from[$<sa:%p>] ret[%03u %s]"
           " sz[${BKMG.ju:%ju}:%ju]", data, req->sects, 1, con->evnt->sa,
           http_ret_code, http_ret_line, con->f->len, con->f->len);
  http_vlg_def(con, req);
  
  if (req->head_op || req->use_mmap || !con->f->len)
    httpd_fin_fd_close(con);
  else if (req->policy->use_posix_fadvise)
    posix_fadvise64(con->f->fd, con->f->off, con->f->len,POSIX_FADV_SEQUENTIAL);
  
  return (http_fin_req(con, req));

 malloc_close_err:
  httpd_fin_fd_close(con);
 malloc_err:
  VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_get(): %m\n"));
}

int http_req_op_opts(struct Con *con, Httpd_req_data *req)
{
  Vstr_base *out = con->evnt->io_w;
  Vstr_base *fname = req->fname;
  VSTR_AUTOCONF_uintmax_t tmp = 0;

  if (fname->conf->malloc_bad)
    goto malloc_err;
  
  assert(VPREFIX(fname, 1, fname->len, "/") ||
         !req->policy->use_vhosts ||
         !req->policy->use_chk_host_err ||
	 !req->policy->use_host_err_400 ||
         VEQ(con->evnt->io_r, req->path_pos, req->path_len, "*"));
  
  /* apache doesn't test for 404's here ... which seems weird */
  
  http_app_def_hdrs(con, req, 200, "OK", 0, NULL, req->policy->use_range, 0);
  http_app_hdr_cstr(out, "Allow", "GET, HEAD, OPTIONS, TRACE");
  http_app_end_hdrs(out);
  if (out->conf->malloc_bad)
    goto malloc_err;
  
  vlg_info(vlg, "REQ %s from[$<sa:%p>] ret[%03u %s] sz[${BKMG.ju:%ju}:%ju]",
           "OPTIONS", con->evnt->sa, 200, "OK", tmp, tmp);
  http_vlg_def(con, req);
  
  return (http_fin_req(con, req));
  
 malloc_err:
  VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_opts(): %m\n"));
}

int http_req_op_trace(struct Con *con, Httpd_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *out  = con->evnt->io_w;
  VSTR_AUTOCONF_uintmax_t tmp = 0;
      
  http_app_def_hdrs(con, req, 200, "OK", req->now,
                    "message/http", FALSE, req->len);
  http_app_end_hdrs(out);
  vstr_add_vstr(out, out->len, data, 1, req->len, VSTR_TYPE_ADD_DEF);
  if (out->conf->malloc_bad)
    VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "op_trace(): %m\n"));

  tmp = req->len;
  vlg_info(vlg, "REQ %s from[$<sa:%p>] ret[%03u %s] sz[${BKMG.ju:%ju}:%ju]",
           "TRACE", con->evnt->sa, 200, "OK", tmp, tmp);
  http_vlg_def(con, req);
      
  return (http_fin_req(con, req));
}

/* characters that are valid in a URL _and_ in a filename ...
 * without encoding */
#define HTTPD__VALID_CSTR_CHRS_URL_FILENAME \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"            \
    "abcdefghijklmnopqrstuvwxyz"            \
    "0123456789"                            \
    ":./-_~"
int httpd_valid_url_filename(Vstr_base *s1, size_t pos, size_t len)
{
  const char *const cstr = HTTPD__VALID_CSTR_CHRS_URL_FILENAME;
  return (vstr_spn_cstr_chrs_fwd(s1, pos, len, cstr) == s1->len);
}

int httpd_init_default_hostname(Httpd_policy_opts *popts)
{
  Vstr_base *lfn = popts->default_hostname;
  
  if (!httpd_valid_url_filename(lfn, 1, lfn->len))
    vstr_del(lfn, 1, lfn->len);
  
  if (!lfn->len)
  {
    char buf[256];

    if (gethostname(buf, sizeof(buf)) == -1)
      err(EXIT_FAILURE, "gethostname");
    
    buf[255] = 0;
    vstr_add_cstr_buf(lfn, 0, buf);

    if (httpd_opts->s->tcp_port != 80)
      vstr_add_fmt(lfn, lfn->len, ":%hd", httpd_opts->s->tcp_port);
  }

  vstr_conv_lowercase(lfn, 1, lfn->len);

  return (!lfn->conf->malloc_bad);
}

static int serv_chk_vhost(Httpd_policy_opts *popts,
			  Vstr_base *lfn, size_t pos, size_t len)
{
  const char *vhost = NULL;
  struct stat64 v_stat[1];
  Vstr_base *def_hname = popts->default_hostname;
  int ret = -1;
  
  ASSERT(pos);

  if (!popts->use_chk_host_err)
    return (TRUE);
  
  if (vstr_cmp_eq(lfn, pos, len, def_hname, 1, def_hname->len))
    return (TRUE); /* don't do lots of work for nothing */

  vstr_add_cstr_ptr(lfn, pos - 1, "/");
  vstr_add_vstr(lfn, pos - 1, 
		popts->document_root, 1, popts->document_root->len,
                VSTR_TYPE_ADD_BUF_PTR);
  len += popts->document_root->len + 1;
  
  if (lfn->conf->malloc_bad || !(vhost = vstr_export_cstr_ptr(lfn, pos, len)))
    return (TRUE); /* dealt with as errmem_req() later */
  
  ret = stat64(vhost, v_stat);
  vstr_del(lfn, pos, popts->document_root->len + 1);

  if (ret == -1)
    return (FALSE);

  if (!S_ISDIR(v_stat->st_mode))
    return (FALSE);
  
  return (TRUE);
}

static int httpd_serv_add_vhost(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *fname = req->fname;
  Vstr_sect_node *h_h = req->http_hdrs->hdr_host;
  size_t h_h_pos = h_h->pos;
  size_t h_h_len = h_h->len;
  size_t orig_len = 0;
  
  if (h_h_len && req->policy->use_canonize_host)
  {
    size_t dots = 0;
    
    if (VIPREFIX(data, h_h_pos, h_h_len, "www."))
    { h_h_len -= CLEN("www."); h_h_pos += CLEN("www."); }
    
    dots = vstr_spn_cstr_chrs_rev(data, h_h_pos, h_h_len, ".");
    h_h_len -= dots;
  }

  orig_len = fname->len;
  if (!h_h_len)
    httpd_sc_add_default_hostname(req->policy, fname, 0);
  else if (vstr_add_vstr(fname, 0, data, /* add as buf's, for lowercase op */
                         h_h_pos, h_h_len, VSTR_TYPE_ADD_DEF))
  {
    vstr_conv_lowercase(fname, 1, h_h_len);
    
    if (!serv_chk_vhost(req->policy, fname, 1, h_h_len))
    {
      if (req->policy->use_host_err_400)
        HTTPD_ERR_RET(req, 400, FALSE); /* rfc2616 5.2 */
      else
      { /* what everything else does ... *sigh* */
        if (fname->conf->malloc_bad)
          return (TRUE);
      
        vstr_del(fname, 1, h_h_len);
        httpd_sc_add_default_hostname(req->policy, fname, 0);
      }  
    }
  }

  vstr_add_cstr_ptr(fname, 0, "/");
  req->vhost_prefix_len = (fname->len - orig_len);

  return (TRUE);
}

static int http_req_make_path(struct Con *con, struct Httpd_req_data *req)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_base *fname = req->fname;
  
  ASSERT(!fname->len);

  assert(VPREFIX(data, req->path_pos, req->path_len, "/") ||
         VEQ(data, req->path_pos, req->path_len, "*"));

  /* don't allow encoded slashes */
  if (vstr_srch_case_cstr_buf_fwd(data, req->path_pos, req->path_len, "%2f"))
    HTTPD_ERR_RET(req, 403, FALSE);
  
  vstr_add_vstr(fname, 0,
                data, req->path_pos, req->path_len, VSTR_TYPE_ADD_BUF_PTR);
  vstr_conv_decode_uri(fname, 1, fname->len);

  if (fname->conf->malloc_bad) /* dealt with as errmem_req() later */
    return (TRUE);
  
  if (req->policy->use_vhosts)
    if (!httpd_serv_add_vhost(con, req))
      return (FALSE);
  
  /* check posix path ... including hostname, for NIL and path escapes */
  if (vstr_srch_chr_fwd(fname, 1, fname->len, 0))
    HTTPD_ERR_RET(req, 403, FALSE);

  /* web servers don't have relative paths, so /./ and /../ aren't "special" */
  if (vstr_srch_cstr_buf_fwd(fname, 1, fname->len, "/../") ||
      VSUFFIX(req->fname, 1, req->fname->len, "/.."))
    HTTPD_ERR_RET(req, 400, FALSE);
  if (req->policy->chk_dot_dir &&
      (vstr_srch_cstr_buf_fwd(fname, 1, fname->len, "/./") ||
       VSUFFIX(req->fname, 1, req->fname->len, "/.")))
    HTTPD_ERR_RET(req, 400, FALSE);

  ASSERT(fname->len);
  assert(VPREFIX(fname, 1, fname->len, "/") ||
         VEQ(fname, 1, fname->len, "*") ||
         fname->conf->malloc_bad);

  if (fname->conf->malloc_bad)
    return (TRUE);
  
  if (vstr_export_chr(fname, fname->len) == '/')
  {
    Vstr_base *tmp = req->policy->dir_filename;
    vstr_add_vstr(fname, fname->len, tmp, 1, tmp->len, VSTR_TYPE_ADD_DEF);
  }
  
  return (TRUE);
}

static int http_parse_wait_io_r(struct Con *con)
{
  if (con->evnt->io_r_shutdown)
    return (FALSE);

  evnt_fd_set_cork(con->evnt, FALSE);
  return (TRUE);
}

static int serv__http_parse_no_req(struct Con *con, struct Httpd_req_data *req)
{
  if (req->policy->max_header_sz &&
      (con->evnt->io_r->len > req->policy->max_header_sz))
    HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

  http_req_free(req);
  
  return (http_parse_wait_io_r(con));
}

/* http spec says ignore leading LWS ... *sigh* */
static int http__parse_req_all(struct Con *con, struct Httpd_req_data *req,
                               const char *eol, int *ern)
{
  Vstr_base *data = con->evnt->io_r;
  
  ASSERT(eol && ern);
  
  *ern = FALSE;
  
  if (!(req->len = vstr_srch_cstr_buf_fwd(data, 1, data->len, eol)))
      goto no_req;
  
  if (req->len == 1)
  { /* should use vstr_del(data, 1, vstr_spn_cstr_buf_fwd(..., HTTP_EOL)); */
    while (VPREFIX(data, 1, data->len, HTTP_EOL))
      vstr_del(data, 1, CLEN(HTTP_EOL));
    
    if (!(req->len = vstr_srch_cstr_buf_fwd(data, 1, data->len, eol)))
      goto no_req;
    
    ASSERT(req->len > 1);
  }
  
  req->len += CLEN(eol) - 1; /* add rest of EOL */

  return (TRUE);

 no_req:
  *ern = serv__http_parse_no_req(con, req);
  return (FALSE);
}

static int http_parse_req(struct Con *con)
{
  Vstr_base *data = con->evnt->io_r;
  struct Httpd_req_data *req = NULL;
  int ern_req_all = 0;

  ASSERT(!con->f->len);

  if (!(req = http_req_make(con)))
    return (FALSE);

  if (con->parsed_method_ver_1_0) /* wait for all the headers */
  {
    if (!http__parse_req_all(con, req, HTTP_END_OF_REQUEST, &ern_req_all))
      return (ern_req_all);
  }
  else
  {
    if (!http__parse_req_all(con, req, HTTP_EOL,            &ern_req_all))
      return (ern_req_all);
  }

  con->keep_alive = HTTP_NON_KEEP_ALIVE;
  http_req_split_method(con, req);
  if (req->sects->malloc_bad)
    VLG_WARNNOMEM_RET(http_fin_errmem_req(con, req), (vlg, "split: %m\n"));
  else if (req->sects->num < 2)
    HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));
  else
  {
    size_t op_pos = 0;
    size_t op_len = 0;

    if (req->ver_0_9)
      vlg_dbg1(vlg, "Method(0.9):"
               " $<http-esc.vstr.sect:%p%p%u> $<http-esc.vstr.sect:%p%p%u>\n",
               con->evnt->io_r, req->sects, 1, con->evnt->io_r, req->sects, 2);
    else
    { /* need to get all headers */
      if (!con->parsed_method_ver_1_0)
      {
        con->parsed_method_ver_1_0 = TRUE;
        if (!http__parse_req_all(con, req, HTTP_END_OF_REQUEST, &ern_req_all))
          return (ern_req_all);
      }
      
      vlg_dbg1(vlg, "Method(1.x):"
               " $<http-esc.vstr.sect:%p%p%u> $<http-esc.vstr.sect:%p%p%u>"
               " $<http-esc.vstr.sect:%p%p%u>\n",
               data, req->sects, 1, data, req->sects, 2, data, req->sects, 3);
      http_req_split_hdrs(con, req);
    }
    evnt_got_pkt(con->evnt);

    if (HTTP_CONF_SAFE_PRINT_REQ)
      vlg_dbg3(vlg, "REQ:\n$<vstr.hexdump:%p%zu%zu>",
               data, (size_t)1, data->len);
    else
      vlg_dbg3(vlg, "REQ:\n$<vstr:%p%zu%zu>",
               data, (size_t)1, data->len);
    
    assert(((req->sects->num >= 3) && !req->ver_0_9) || (req->sects->num == 2));
    
    op_pos        = VSTR_SECTS_NUM(req->sects, 1)->pos;
    op_len        = VSTR_SECTS_NUM(req->sects, 1)->len;
    req->path_pos = VSTR_SECTS_NUM(req->sects, 2)->pos;
    req->path_len = VSTR_SECTS_NUM(req->sects, 2)->len;

    if (!req->ver_0_9 && !http__parse_1_x(con, req))
    {
      if (req->error_code == 500)
        return (http_fin_errmem_req(con, req));
      return (http_fin_err_req(con, req));
    }
    
    if (!http_parse_host(con, req))
      HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

    if (0) { }
    else if (VEQ(data, op_pos, op_len, "GET"))
    {
      if (!VPREFIX(data, req->path_pos, req->path_len, "/"))
        HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));
      
      if (!http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_get(con, req));
    }
    else if (req->ver_0_9) /* 400 or 501? - apache does 400 */
      HTTPD_ERR_RET(req, 501, http_fin_err_req(con, req));      
    else if (VEQ(data, op_pos, op_len, "HEAD"))
    {
      req->head_op = TRUE; /* not sure where this should go here */
      
      if (!VPREFIX(data, req->path_pos, req->path_len, "/"))
        HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));
      
      if (!http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_get(con, req));
    }
    else if (VEQ(data, op_pos, op_len, "OPTIONS"))
    {
      if (!VPREFIX(data, req->path_pos, req->path_len, "/") &&
          !VEQ(data, req->path_pos, req->path_len, "*"))
        HTTPD_ERR_RET(req, 400, http_fin_err_req(con, req));

      /* Speed hack: Don't even call make_path if it's "OPTIONS * ..."
       * and we don't need to check the Host header */
      if (req->policy->use_vhosts &&
          req->policy->use_chk_host_err && req->policy->use_host_err_400 &&
          !VEQ(data, req->path_pos, req->path_len, "*") &&
          !http_req_make_path(con, req))
        return (http_fin_err_req(con, req));

      return (http_req_op_opts(con, req));
    }
    else if (VEQ(data, op_pos, op_len, "TRACE"))
    {
      return (http_req_op_trace(con, req));
    }
    else if (VEQ(data, op_pos, op_len, "POST") ||
             VEQ(data, op_pos, op_len, "PUT") ||
             VEQ(data, op_pos, op_len, "DELETE") ||
             VEQ(data, op_pos, op_len, "CONNECT") ||
             FALSE) /* we know about these ... but don't allow them */
      HTTPD_ERR_RET(req, 405, http_fin_err_req(con, req));
    else
      HTTPD_ERR_RET(req, 501, http_fin_err_req(con, req));
  }
  ASSERT_NOT_REACHED();
}

static int httpd_serv_q_send(struct Con *con)
{
  vlg_dbg3(vlg, "Q $<sa:%p>\n", con->evnt->sa);
  if (!evnt_send_add(con->evnt, TRUE, HTTPD_CONF_MAX_WAIT_SEND))
  {
    if (errno != EPIPE)
      vlg_warn(vlg, "send q: %m\n");
    return (FALSE);
  }
      
  /* queued */
  return (TRUE);
}

static int httpd_serv_fin_send(struct Con *con)
{
  if (con->keep_alive)
  { /* need to try immediately, as we might have already got the next req */
    if (!con->evnt->io_r_shutdown)
      evnt_wait_cntl_add(con->evnt, POLLIN);
    return (http_parse_req(con));
  }

  return (evnt_shutdown_w(con->evnt));
}

/* try sending a bunch of times, bail out if we've done a few ... keep going
 * if we have too much data */
#define SERV__SEND(x) do {                    \
    if (!--num)                               \
      return (httpd_serv_q_send(con));        \
                                              \
    if (!evnt_send(con->evnt))                \
    {                                         \
      if (errno != EPIPE)                     \
        vlg_warn(vlg, "send(%s): %m\n", x);   \
      return (FALSE);                         \
    }                                         \
 } while (out->len >= EX_MAX_W_DATA_INCORE)

int httpd_serv_send(struct Con *con)
{
  Vstr_base *out = con->evnt->io_w;
  size_t len = 0;
  int ret = IO_OK;
  unsigned int num = HTTP_CONF_FS_READ_CALL_LIMIT;
  
  ASSERT(!out->conf->malloc_bad);
    
  if ((con->f->fd == -1) && con->f->len)
    return (FALSE);

  ASSERT((con->f->fd == -1) == !con->f->len);
  
  if (out->len)
    SERV__SEND("beg");

  if (!con->f->len)
  {
    while (out->len)
      SERV__SEND("end");
    return (httpd_serv_fin_send(con));
  }

  if (con->use_sendfile)
  {
    unsigned int ern = 0;
    
    while (con->f->len)
    {
      struct File_sect *f = con->f;

      if (!--num)
        return (httpd_serv_q_send(con));
      
      if (!evnt_sendfile(con->evnt, f->fd, &f->off, &f->len, &ern))
      {
        if (ern == VSTR_TYPE_SC_READ_FD_ERR_EOF)
          goto file_eof;
        
        if (errno == ENOSYS) /* also logs it */
          con->policy->use_sendfile = FALSE;
        if ((errno == EPIPE) || (errno == ECONNRESET))
          return (FALSE);
        else
          vlg_warn(vlg, "sendfile: %m\n");

        if (lseek64(f->fd, f->off, SEEK_SET) == -1)
          VLG_WARN_RET(FALSE, (vlg, "lseek(<sendfile>,off=%ju): %m\n", f->off));

        con->use_sendfile = FALSE;
        return (httpd_serv_send(con));
      }
    }
    
    goto file_eof;
  }
  
  ASSERT(con->f->len);
  len = out->len;
  while ((ret = io_get(out, con->f->fd)) != IO_FAIL)
  {
    size_t tmp = out->len - len;

    if (ret == IO_EOF)
      goto file_eof;
    
    if (tmp < con->f->len)
      con->f->len -= tmp;
    else
    { /* we might not be transfering to EOF, so reduce if needed */
      vstr_sc_reduce(out, 1, out->len, tmp - con->f->len);
      ASSERT((out->len - len) == con->f->len);
      con->f->len = 0;
      goto file_eof;
    }

    SERV__SEND("io_get");

    len = out->len;
  }

  vlg_warn(vlg, "read: %m\n");
  out->conf->malloc_bad = FALSE;

 file_eof:
  ASSERT(con->f->fd != -1);
  close(con->f->fd);
  con->f->fd = -1;

  return (httpd_serv_send(con)); /* restart with a read, or finish */
}

int httpd_serv_recv(struct Con *con)
{
  unsigned int ern = 0;
  int ret = 0;
  Vstr_base *data = con->evnt->io_r;

  ASSERT(!con->evnt->io_r_shutdown);
  
  if (!(ret = evnt_recv(con->evnt, &ern)))
  {
    vlg_dbg3(vlg, "RECV ERR from[$<sa:%p>]: %u\n", con->evnt->sa, ern);
    
    if (ern != VSTR_TYPE_SC_READ_FD_ERR_EOF)
      goto con_cleanup;
    if (!evnt_shutdown_r(con->evnt, TRUE))
      goto con_cleanup;
  }

  if (con->f->len) /* need to stop input, until we can get rid of it */
  {
    ASSERT(con->keep_alive || con->parsed_method_ver_1_0);
    
    if (con->policy->max_header_sz && (data->len > con->policy->max_header_sz))
      evnt_wait_cntl_del(con->evnt, POLLIN);

    return (TRUE);
  }
  
  if (http_parse_req(con))
    return (TRUE);
  
 con_cleanup:
  con->evnt->io_r->conf->malloc_bad = FALSE;
  con->evnt->io_w->conf->malloc_bad = FALSE;
    
  return (FALSE);
}

int httpd_con_init(struct Con *con, struct Acpt_listener *acpt_listener)
{
  con->f->fd                 = -1;
  con->f->len                =  0;
  con->vary_star             = FALSE;
  con->keep_alive            = HTTP_NON_KEEP_ALIVE;
  con->acpt_sa_ref           = vstr_ref_add(acpt_listener->ref);
  con->parsed_method_ver_1_0 = FALSE;
  
  httpd_policy_change_con(con, httpd_opts->def_policy);

  if (!httpd_policy_connection(con,
                               httpd_opts->conf, httpd_opts->match_connection))
  {
    Vstr_base *s1 = httpd_opts->conf->tmp;
    Vstr_base *s2 = con->policy->policy_name;
    
    if (s1->len)
      vlg_info(vlg, "CONF-ERR from[$<sa:%p>]: HTTPD policy $<vstr:%p%zu%zu>"
               " backtrace: $<vstr:%p%zu%zu>\n",
               con->evnt->sa, s1, (size_t)1, s1->len, s2, (size_t)1, s2->len);
    goto con_fail;
  }

  if (con->evnt->flag_q_closed)
  {
    Vstr_base *s1 = con->policy->policy_name;
    vlg_info(vlg, "BLOCKED from[$<sa:%p>]: HTTPD policy $<vstr:%p%zu%zu>\n",
             con->evnt->sa, s1, (size_t)1, s1->len);
  }
  
  return (TRUE);
 con_fail:
  vstr_ref_del(con->acpt_sa_ref);
  return (FALSE);  
}
