
#include "tst-main.c"

static const char *rf = __FILE__;

static int tst_fmt_cb(struct Vstr_base *d1 __attribute__((unused)),
                      size_t d2 __attribute__((unused)),
                      struct Vstr_fmt_spec *d3 __attribute__((unused)))
{
  return (TRUE);
}

static void *tst_cache_cb(const struct Vstr_base *d1 __attribute__((unused)),
                          size_t d2 __attribute__((unused)),
                          size_t d3 __attribute__((unused)),
                          unsigned int d4 __attribute__((unused)),
                          void *d5 __attribute__((unused)))
{
  return (NULL);
}

int tst(void)
{
  Vstr_conf *conf = vstr_make_conf();
  Vstr_conf *orig = conf;
  int ret = 0;
  
  TST_B_TST(ret,  1,
            !vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, UINT_MAX));

  assert(VSTR_MAX_NODE_BUF < UINT_MAX);
  
  TST_B_TST(ret,  2, (conf->buf_sz != VSTR_MAX_NODE_BUF));
  TST_B_TST(ret,  3, (conf == s1->conf));
  TST_B_TST(ret,  4, (conf->buf_sz == s1->conf->buf_sz));
  TST_B_TST(ret,  5, (conf != orig));
  
  TST_B_TST(ret,  6,
            !vstr_swap_conf(s1, &conf));

  TST_B_TST(ret,  7, (conf->buf_sz == VSTR_MAX_NODE_BUF));
  TST_B_TST(ret,  8, (conf == s1->conf));
  TST_B_TST(ret,  9, (conf->buf_sz != s1->conf->buf_sz));

  TST_B_TST(ret, 10, (conf == orig));
  vstr_cntl_base(s1, VSTR_CNTL_BASE_GET_CONF, &orig); /* get another ref */
  TST_B_TST(ret, 10, (conf == orig));
  
  TST_B_TST(ret, 10, !vstr_swap_conf(s1, &orig)); /* nop */
  TST_B_TST(ret, 10, (conf == orig));
  vstr_free_conf(orig); /* release second ref */
  
  TST_B_TST(ret, 11,
            !vstr_swap_conf(s1, &conf));

  /* s3 */
  TST_B_TST(ret, 12, (conf == s1->conf));
  TST_B_TST(ret, 13, (conf->buf_sz != s1->conf->buf_sz));
  TST_B_TST(ret, 14, (conf == s3->conf));
  TST_B_TST(ret, 15, (conf->buf_sz == s3->conf->buf_sz));
  TST_B_TST(ret, 16, (conf != orig));
  
  TST_B_TST(ret, 17,
            !vstr_swap_conf(s3, &conf));
  
  TST_B_TST(ret, 18, (conf == s1->conf));
  TST_B_TST(ret, 19, (conf->buf_sz == s1->conf->buf_sz));
  TST_B_TST(ret, 20, (conf == s3->conf));
  TST_B_TST(ret, 21, (conf->buf_sz != s3->conf->buf_sz));
  TST_B_TST(ret, 22, (conf == orig));

  TST_B_TST(ret, 23,
            !vstr_swap_conf(s3, &conf));
  
  TST_B_TST(ret, 24, (conf == s1->conf));
  TST_B_TST(ret, 25, (conf->buf_sz == s1->conf->buf_sz));
  TST_B_TST(ret, 26, (conf == s3->conf));
  TST_B_TST(ret, 27, (conf->buf_sz != s3->conf->buf_sz));
  TST_B_TST(ret, 28, (conf != orig));

  vstr_fmt_add(s3->conf, "fubar", tst_fmt_cb, VSTR_TYPE_FMT_END);
  vstr_cache_add(s3->conf, "fubar", tst_cache_cb);
  TST_B_TST(ret, 29, s3->conf->cache_cbs_sz < conf->cache_cbs_sz);
  TST_B_TST(ret, 30, !vstr_swap_conf(s3, &conf));
  TST_B_TST(ret, 31, s3->conf->cache_cbs_sz != conf->cache_cbs_sz);

  vstr_free_conf(conf);
  
  return (TST_B_RET(ret));
}
