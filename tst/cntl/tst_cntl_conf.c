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

  TST_FLAG(  1, FLAG_ATOMIC_OPS, TRUE, FALSE);
  TST_FLAG(  2, FLAG_DEL_SPLIT, FALSE, TRUE);
  TST_FLAG(  3, FLAG_IOV_UPDATE, TRUE, FALSE);
  TST_CHR(   5, FMT_CHAR_ESC, 0, '$');
  
  TST_CSTR(  6, LOC_CSTR_DEC_POINT, ".", "->");
  TST_CSTR(  7, LOC_CSTR_NAME_NUMERIC, "C", "James");
  TST_CSTR2( 8, LOC_CSTR_THOU_GRP, "", "\1\2\3\255abcd", "\1\2\3\255");
  TST_CSTR(  9, LOC_CSTR_THOU_SEP, "", "<->");

  if (MFAIL_NUM_OK)
  {
    int succeeded = FALSE;
    unsigned long mfail_count = 0;
    
    while (!succeeded)
    {
      tst_mfail_num(++mfail_count);
      succeeded = vstr_cntl_conf(NULL,
                                 VSTR_CNTL_CONF_SET_LOC_CSTR_AUTO_NAME_NUMERIC,
                                 "C");
    }
    tst_mfail_num(0);
    ASSERT(mfail_count > 1);
  }
  
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
 * VSTR_CNTL_CONF_GET_NUM_REF
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
