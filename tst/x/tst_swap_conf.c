
#include "../tst-main.c"

static const char *rf = __FILE__;

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
  TST_B_TST(ret, 19, (conf->buf_sz == s3->conf->buf_sz));
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

  vstr_free_conf(conf);
  
  return (EXIT_SUCCESS);
}
