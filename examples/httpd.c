
#include "httpd.h"
#include "httpd_conf_main.h"

#ifndef VSTR_AUTOCONF_NDEBUG
# define assert(x) do { if (x) {} else errx(EXIT_FAILURE, "assert(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
# define ASSERT(x) do { if (x) {} else errx(EXIT_FAILURE, "ASSERT(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
#else
# define ASSERT(x)
# define assert(x)
#endif
#define ASSERT_NOT_REACHED() assert(FALSE)

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

/* is the cstr a prefix of the vstr */
#define VPREFIX(vstr, p, l, cstr)                                       \
    (((l) >= strlen(cstr)) && vstr_cmp_bod_cstr_eq(vstr, p, l, cstr))

int httpd_sc_add_default_hostname(struct Httpd_opts *opts,
                                  Vstr_base *lfn, size_t pos)
{
  Vstr_base *d_h = opts->default_hostname;
  
  return (vstr_add_vstr(lfn, pos, d_h, 1, d_h->len, VSTR_TYPE_ADD_DEF));
}

void httpd_req_absolute_uri(struct Httpd_opts *opts,
                            struct Con *con, struct Httpd_req_data *req,
                            Vstr_base *lfn)
{
  Vstr_base *data = con->evnt->io_r;
  Vstr_sect_node *h_h = req->http_hdrs->hdr_host;
  size_t orig_len = lfn->len;
  size_t pos = lfn->len - orig_len;
  int has_schema   = TRUE;
  int has_abs_path = TRUE;
  
  if (!VPREFIX(lfn, 1, lfn->len, "http://"))
  {
    has_schema = FALSE;
    if (!VPREFIX(lfn, 1, lfn->len, "/")) /* relative pathname */
      has_abs_path = FALSE;
  }

  if (!has_schema)
  {
    vstr_add_cstr_buf(lfn, pos, "http://");
    pos = lfn->len - orig_len;
    if (!h_h->len)
      httpd_sc_add_default_hostname(opts, lfn, pos);
    else
      vstr_add_vstr(lfn, pos,
                    data, h_h->pos, h_h->len, VSTR_TYPE_ADD_ALL_BUF);
    pos = lfn->len - orig_len;
  }
    
  if (!has_abs_path)
  {
    size_t len = req->path_len;

    if (orig_len)
      len -= vstr_cspn_cstr_chrs_rev(data, req->path_pos, len, "/");
    
    vstr_add_vstr(lfn, pos, data, req->path_pos, len,
                  VSTR_TYPE_ADD_ALL_BUF);
    pos = lfn->len - orig_len;
  }
}

