#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  unsigned int a_pos = 0, b_pos = 0, c_pos = 0, d_pos = 0;
  int ret = 0;
  VSTR_SECTS_DECL(sects8, 8);
  VSTR_SECTS_DECL(sects4, 4);
  VSTR_SECTS_DECL(sects2, 2);
  unsigned int split2_num = 0;
  unsigned int split4_num = 0;
  unsigned int split8_num = 0;
  
  VSTR_SECTS_DECL_INIT(sects8);
  VSTR_SECTS_DECL_INIT(sects4);
  VSTR_SECTS_DECL_INIT(sects2);
  
  VSTR_ADD_CSTR_BUF(s1, 0, ":a::b:!:c::d:");
  a_pos = 2;
  b_pos = 5;
  c_pos = 9;
  d_pos = 12;

  split2_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects2, sects2->sz, VSTR_FLAG_SPLIT_DEF);
  split4_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects4, sects4->sz, VSTR_FLAG_SPLIT_DEF);
  split8_num = vstr_split_cstr_chrs(s1, 1, s1->len, "!:",
                                    sects8, sects8->sz, VSTR_FLAG_SPLIT_DEF);
  
  VSTR_SECTS_DECL_INIT(sects8); /* can do many times without harm */
  VSTR_SECTS_DECL_INIT(sects4);
  VSTR_SECTS_DECL_INIT(sects2);

  /* limited to size - done for speed */
  TST_B_TST(ret, 1, ((sects8->num != 4) ||
                     (split8_num  != 4) ||
                     (VSTR_SECTS_NUM(sects8, 1)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects8, 1)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 2)->pos != b_pos) ||
                     (VSTR_SECTS_NUM(sects8, 2)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 3)->pos != c_pos) ||
                     (VSTR_SECTS_NUM(sects8, 3)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 4)->pos != d_pos) ||
                     (VSTR_SECTS_NUM(sects8, 4)->len != 1)));

  TST_B_TST(ret, 2, ((sects4->num != 4) ||
                     (split4_num  != 4) ||
                     (VSTR_SECTS_NUM(sects4, 1)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects4, 1)->len != 1) ||
                     (VSTR_SECTS_NUM(sects4, 2)->pos != b_pos) ||
                     (VSTR_SECTS_NUM(sects4, 2)->len != 1) ||
                     (VSTR_SECTS_NUM(sects4, 3)->pos != c_pos) ||
                     (VSTR_SECTS_NUM(sects4, 3)->len != 1) ||
                     (VSTR_SECTS_NUM(sects4, 4)->pos != d_pos) ||
                     (VSTR_SECTS_NUM(sects4, 4)->len != 1)));
  
  TST_B_TST(ret, 3, ((sects2->num != 2) ||
                     (split2_num  != 2) ||
                     (VSTR_SECTS_NUM(sects2, 1)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects2, 1)->len != 1) ||
                     (VSTR_SECTS_NUM(sects2, 2)->pos != b_pos) ||
                     (VSTR_SECTS_NUM(sects2, 2)->len != 1)));

  /* no limit - check return value */
  sects2->num = 0;
  split2_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects2, 0, VSTR_FLAG_SPLIT_DEF);
  
  TST_B_TST(ret, 4, ((sects2->num != 2) ||
                     (split2_num  != 4) ||
                     (VSTR_SECTS_NUM(sects2, 1)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects2, 1)->len != 1) ||
                     (VSTR_SECTS_NUM(sects2, 2)->pos != b_pos) ||
                     (VSTR_SECTS_NUM(sects2, 2)->len != 1)));

  /* with REMAIN flag -- also does second split into same section for sects8 */
  sects2->num = 0;
  split2_num = vstr_split_cstr_chrs(s1, 1, s1->len, "!:",
                                    sects2, sects2->sz, VSTR_FLAG_SPLIT_REMAIN);
  sects4->num = 0;
  split4_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects4, sects4->sz, VSTR_FLAG_SPLIT_REMAIN);
  split8_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects8, sects8->sz, VSTR_FLAG_SPLIT_REMAIN);
  
  TST_B_TST(ret, 5, ((sects8->num != 8) ||
                     (split8_num  != 4) ||
                     (VSTR_SECTS_NUM(sects8, 5)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects8, 5)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 6)->pos != b_pos) ||
                     (VSTR_SECTS_NUM(sects8, 6)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 7)->pos != c_pos) ||
                     (VSTR_SECTS_NUM(sects8, 7)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 8)->pos != d_pos) ||
                     (VSTR_SECTS_NUM(sects8, 8)->len != 1)));

  TST_B_TST(ret, 6, ((sects4->num != 4) ||
                     (split4_num  != 4) ||
                     (VSTR_SECTS_NUM(sects4, 1)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects4, 1)->len != 1) ||
                     (VSTR_SECTS_NUM(sects4, 2)->pos != b_pos) ||
                     (VSTR_SECTS_NUM(sects4, 2)->len != 1) ||
                     (VSTR_SECTS_NUM(sects4, 3)->pos != c_pos) ||
                     (VSTR_SECTS_NUM(sects4, 3)->len != 1) ||
                     (VSTR_SECTS_NUM(sects4, 4)->pos != (d_pos - 1)) ||
                     (VSTR_SECTS_NUM(sects4, 4)->len != 3)));
  
  TST_B_TST(ret, 7, ((sects2->num != 2) ||
                     (split2_num  != 2) ||
                     (VSTR_SECTS_NUM(sects2, 1)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects2, 1)->len != 1) ||
                     (VSTR_SECTS_NUM(sects2, 2)->pos != (b_pos - 1)) ||
                     (VSTR_SECTS_NUM(sects2, 2)->len != 10)));

  /* with REMAIN and beg null flags */
  sects2->num = 0;
  split2_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects2, sects2->sz,
                                    VSTR_FLAG_SPLIT_BEG_NULL |
                                    VSTR_FLAG_SPLIT_REMAIN);
  
  sects8->num = 0;
  split8_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects8, sects8->sz,
                                    VSTR_FLAG_SPLIT_BEG_NULL |
                                    VSTR_FLAG_SPLIT_REMAIN);
  
  TST_B_TST(ret, 8, ((sects8->num != 5) ||
                     (split8_num  != 5) ||
                     (VSTR_SECTS_NUM(sects2, 1)->pos != 1) ||
                     (VSTR_SECTS_NUM(sects2, 1)->len != 0) ||
                     (VSTR_SECTS_NUM(sects8, 2)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects8, 2)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 3)->pos != b_pos) ||
                     (VSTR_SECTS_NUM(sects8, 3)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 4)->pos != c_pos) ||
                     (VSTR_SECTS_NUM(sects8, 4)->len != 1) ||
                     (VSTR_SECTS_NUM(sects8, 5)->pos != d_pos) ||
                     (VSTR_SECTS_NUM(sects8, 5)->len != 1)));

  TST_B_TST(ret, 9, ((sects2->num != 2) ||
                     (split2_num  != 2) ||
                     (VSTR_SECTS_NUM(sects2, 1)->pos != 1) ||
                     (VSTR_SECTS_NUM(sects2, 1)->len != 0) ||
                     (VSTR_SECTS_NUM(sects2, 2)->pos != a_pos) ||
                     (VSTR_SECTS_NUM(sects2, 2)->len != 12)));

  /* with beg_null, and no remain ... check ret */
  sects2->num = 0;
  split2_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects2, 0,
                                    VSTR_FLAG_SPLIT_BEG_NULL);
  
  TST_B_TST(ret, 10, ((sects2->num != 2) ||
                      (split2_num  != 5) ||
                      (VSTR_SECTS_NUM(sects2, 1)->pos != 1) ||
                      (VSTR_SECTS_NUM(sects2, 1)->len != 0) ||
                      (VSTR_SECTS_NUM(sects2, 2)->pos != a_pos) ||
                      (VSTR_SECTS_NUM(sects2, 2)->len != 1)));

  /* with beg_null, no remain and no ret needed */
  sects2->num = 0;
  split2_num = VSTR_SPLIT_CSTR_CHRS(s1, 1, s1->len, "!:",
                                    sects2, 0,
                                    VSTR_FLAG_SPLIT_BEG_NULL |
                                    VSTR_FLAG_SPLIT_NO_RET);
  
  TST_B_TST(ret, 11, ((sects2->num != 2) ||
                      !split2_num ||
                      (VSTR_SECTS_NUM(sects2, 1)->pos != 1) ||
                      (VSTR_SECTS_NUM(sects2, 1)->len != 0) ||
                      (VSTR_SECTS_NUM(sects2, 2)->pos != a_pos) ||
                      (VSTR_SECTS_NUM(sects2, 2)->len != 1)));
  
  vstr_del(s1, 1, s1->len);
  vstr_add_cstr_buf(s1, s1->len, ":!:a");
  
  sects8->num = 0;
  split8_num = vstr_split_cstr_chrs(s1, 1, s1->len, ":!",
                                    sects8, sects8->sz,
                                    VSTR_FLAG_SPLIT_DEF);
  TST_B_TST(ret, 25, split8_num != 1);
  split8_num = vstr_split_cstr_chrs(s1, 1, s1->len, ":!",
                                    sects8, sects8->sz,
                                    VSTR_FLAG_SPLIT_BEG_NULL);
  TST_B_TST(ret, 26, split8_num != 4);
  sects8->num = 0;
  split8_num = vstr_split_cstr_chrs(s1, 1, s1->len - 1, "!:",
                                    sects8, 3,
                                    VSTR_FLAG_SPLIT_BEG_NULL);
  
  return (TST_B_RET(ret));
}
