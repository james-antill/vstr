#include "tst-main.c"

static const char *rf = __FILE__;

#define TST_FLAG(num, name, old, new) \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_GET_ ## name, &tmp_f) && \
              tmp_f == (old))); \
  TST_B_TST(ret, (num), \
            !vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_ ## name, (new))); \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_GET_ ## name, &tmp_f) && \
              tmp_f == (new)))

#define TST_CHR(num, name, old, new) \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_GET_ ## name, &tmp_c) && \
              tmp_c == (old))); \
  TST_B_TST(ret, (num), \
            !vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_ ## name, (new))); \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, VSTR_CNTL_CONF_GET_ ## name, &tmp_c) && \
              tmp_c == (new)))

#define TST_CSTR(num, name, old, new) \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, \
                               VSTR_CNTL_CONF_GET_ ## name, &tmp_pc) && \
              !strcmp(tmp_pc, (old)))); \
  TST_B_TST(ret, (num), \
            !vstr_cntl_conf(NULL, \
                            VSTR_CNTL_CONF_SET_ ## name, (new))); \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, \
                               VSTR_CNTL_CONF_GET_ ## name, &tmp_pc) && \
              !strcmp(tmp_pc, (new))))

#define TST_CSTR2(num, name, old, new, tst_new) \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, \
                               VSTR_CNTL_CONF_GET_ ## name, &tmp_pc) && \
              !strcmp(tmp_pc, (old)))); \
  TST_B_TST(ret, (num), \
            !vstr_cntl_conf(NULL, \
                            VSTR_CNTL_CONF_SET_ ## name, (new))); \
  TST_B_TST(ret, (num), \
            !(!!vstr_cntl_conf(NULL, \
                               VSTR_CNTL_CONF_GET_ ## name, &tmp_pc) && \
              !strcmp(tmp_pc, (tst_new))))

int tst(void)
{
  int ret = 0;
  int tmp_f = FALSE;
  char tmp_c = 0;
  char *tmp_pc = NULL;
  Vstr_conf *tmp_cnf = NULL;
  int mfail_count = 0;
  
  TST_FLAG(  1, FLAG_ATOMIC_OPS, TRUE, FALSE);
  TST_FLAG(  2, FLAG_DEL_SPLIT, FALSE, TRUE);
  TST_FLAG(  3, FLAG_IOV_UPDATE, TRUE, FALSE);
  TST_CHR(   5, FMT_CHAR_ESC, 0, '$');

  TST_CSTR(  6, LOC_CSTR_DEC_POINT, ".", "->");
  TST_CSTR(  7, LOC_CSTR_NAME_NUMERIC, "C", "James");
  TST_CSTR2( 8, LOC_CSTR_THOU_GRP, "", "\1\2\3\255abcd", "\1\2\3\255");
  TST_CSTR(  9, LOC_CSTR_THOU_SEP, "", "<->");

  TST_B_TST(ret, 10, !vstr_cntl_conf(NULL, VSTR_CNTL_CONF_GET_NUM_REF, &tmp_f));
  TST_B_TST(ret, 11, (tmp_f != 2));
  TST_B_TST(ret, 12, !vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_GET_NUM_REF, &tmp_f));
  TST_B_TST(ret, 13, (tmp_f != 2));
  TST_B_TST(ret, 14, !vstr_cntl_base(s1, VSTR_CNTL_BASE_GET_CONF, &tmp_cnf));
  TST_B_TST(ret, 15, !vstr_cntl_conf(NULL, VSTR_CNTL_CONF_GET_NUM_REF, &tmp_f));
  TST_B_TST(ret, 16, (tmp_f != 3));
  TST_B_TST(ret, 17, !vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_GET_NUM_REF, &tmp_f));
  TST_B_TST(ret, 18, (tmp_f != 3));
  vstr_free_conf(tmp_cnf);
  TST_B_TST(ret, 15, !vstr_cntl_conf(NULL, VSTR_CNTL_CONF_GET_NUM_REF, &tmp_f));
  TST_B_TST(ret, 16, (tmp_f != 2));
  TST_B_TST(ret, 20, !vstr_cntl_base(s1, VSTR_CNTL_BASE_SET_CONF, NULL));

#define MFAIL_TST_CSTR(name, val)  \
 vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_ ## name, val)
  
  do
  {
    tst_mfail_num(++mfail_count);
  } while (!MFAIL_TST_CSTR(LOC_CSTR_DEC_POINT, "abcd") ||
           !MFAIL_TST_CSTR(LOC_CSTR_NAME_NUMERIC, "abcd") ||
           !MFAIL_TST_CSTR(LOC_CSTR_THOU_GRP, "abcd") ||
           !MFAIL_TST_CSTR(LOC_CSTR_THOU_SEP, "abcd"));

  tst_mfail_num(0);

  TST_CSTR(  26, LOC_CSTR_DEC_POINT, "abcd", "->");
  TST_CSTR(  27, LOC_CSTR_NAME_NUMERIC, "abcd", "James");
  TST_CSTR(  28, LOC_CSTR_THOU_GRP, "abcd", "\1\2\3\255");
  TST_CSTR(  29, LOC_CSTR_THOU_SEP, "abcd", "<->");

  mfail_count = 0;
  do
  {
    tst_mfail_num(++mfail_count);
  } while (!MFAIL_TST_CSTR(LOC_CSTR_AUTO_NAME_NUMERIC, "en_US"));

  mfail_count = 0;
  do
  {
    vstr_free_spare_nodes(s1->conf, VSTR_TYPE_NODE_PTR, 1000);
    tst_mfail_num(++mfail_count);
  } while (!MFAIL_TST_CSTR(NUM_SPARE_PTR, 4));

  tst_mfail_num(0);

  MFAIL_TST_CSTR(NUM_SPARE_PTR, 0);
  
  return (TST_B_RET(ret));
}
/* Crap for tst_coverage constants....
 *
 * VSTR_CNTL_CONF_GET_FLAG_ATOMIC_OPS
 * VSTR_CNTL_CONF_SET_FLAG_ATOMIC_OPS
 *
 * VSTR_CNTL_CONF_GET_FLAG_DEL_SPLIT
 * VSTR_CNTL_CONF_SET_FLAG_DEL_SPLIT
 *
 * VSTR_CNTL_CONF_GET_FLAG_IOV_UPDATE
 * VSTR_CNTL_CONF_SET_FLAG_IOV_UPDATE
 *
 * VSTR_CNTL_CONF_GET_FMT_CHAR_ESC
 * VSTR_CNTL_CONF_SET_FMT_CHAR_ESC
 *
 * VSTR_CNTL_CONF_GET_LOC_CSTR_DEC_POINT
 * VSTR_CNTL_CONF_SET_LOC_CSTR_DEC_POINT
 *
 * VSTR_CNTL_CONF_GET_LOC_CSTR_NAME_NUMERIC
 * VSTR_CNTL_CONF_SET_LOC_CSTR_NAME_NUMERIC
 *
 * VSTR_CNTL_CONF_GET_LOC_CSTR_THOU_GRP
 * VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_GRP
 *
 * VSTR_CNTL_CONF_GET_LOC_CSTR_THOU_SEP
 * VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_SEP
 *
 *
 *
 *
 * VSTR_CNTL_CONF_GET_NUM_BUF_SZ
 *
 * VSTR_CNTL_CONF_GET_NUM_SPARE_BUF
 * VSTR_CNTL_CONF_SET_NUM_SPARE_BUF
 *
 * VSTR_CNTL_CONF_GET_NUM_SPARE_NON
 * VSTR_CNTL_CONF_SET_NUM_SPARE_NON
 *
 * VSTR_CNTL_CONF_GET_NUM_SPARE_PTR
 * VSTR_CNTL_CONF_SET_NUM_SPARE_PTR
 *
 * VSTR_CNTL_CONF_GET_NUM_SPARE_REF
 * VSTR_CNTL_CONF_SET_NUM_SPARE_REF
 *
 *
 */
