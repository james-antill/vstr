#include "tst-main.c"

static const char *rf = __FILE__;

static unsigned int lens_fwd[4];
static unsigned int lens_rev[4];
static int ret = 0;

static void tst_chrs(Vstr_base *t1, unsigned int off)
{
  TST_B_TST(ret, off + 1,
            VSTR_SPN_CSTR_CHRS_FWD(t1, 1, t1->len, "0123456789") !=
            lens_fwd[0]);
  TST_B_TST(ret, off + 2,
            VSTR_SPN_CSTR_CHRS_REV(t1, 1, t1->len, "abcd") !=
            lens_rev[3]);
  TST_B_TST(ret, off + 3,
            vstr_spn_cstr_chrs_fwd(t1, 1, t1->len, "0123456789abcd -") !=
            lens_fwd[1]);
  TST_B_TST(ret, off + 4,
            vstr_spn_cstr_chrs_rev(t1, 1, t1->len, "0123456789abcd -") !=
            lens_rev[2] - 2);
  TST_B_TST(ret, off + 5,
            VSTR_SPN_CSTR_CHRS_FWD(t1, 1, t1->len, "0123456789abcd -xyz") !=
            lens_fwd[2]);
  TST_B_TST(ret, off + 6,
            VSTR_SPN_CSTR_CHRS_REV(t1, 1, t1->len, "0123456789abcd -!&") !=
            lens_rev[1] - 3);
  TST_B_TST(ret, off + 7,
            VSTR_SPN_CSTR_CHRS_FWD(t1, lens_fwd[0] + 1, t1->len - lens_fwd[0],
                                   "abcd ") != 5);
  TST_B_TST(ret, off + 8,
            VSTR_SPN_CSTR_CHRS_REV(t1, lens_fwd[0] + 1, t1->len - lens_fwd[0],
                                   "abcd")  != 4);  
}

int tst(void)
{
#ifdef USE_RESTRICTED_HEADERS /* %n doesn't work in dietlibc */
  return (EXIT_FAILED_OK);
#endif
  
  sprintf(buf, "%d%nabcd %d%nxyz %u%n!& %u%nabcd",
          INT_MAX,  lens_fwd + 0,
          INT_MIN,  lens_fwd + 1,
          0,        lens_fwd + 2,
          UINT_MAX, lens_fwd + 3);
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf);
  VSTR_ADD_CSTR_BUF(s3, s3->len, buf);
  VSTR_ADD_CSTR_BUF(s4, s4->len, buf);

  lens_rev[0] = s3->len - lens_fwd[0];
  lens_rev[1] = s3->len - lens_fwd[1];
  lens_rev[2] = s3->len - lens_fwd[2];
  lens_rev[3] = s3->len - lens_fwd[3];  

  tst_chrs(s1, 0);
  tst_chrs(s3, 8);

  /* make sure it's got a iovec cache */
  vstr_export_iovec_ptr_all(s1, NULL, NULL);
  vstr_export_iovec_ptr_all(s3, NULL, NULL);
  
  tst_chrs(s1, 0);
  tst_chrs(s3, 8);
  tst_chrs(s4, 16);

  return (TST_B_RET(ret));
}
