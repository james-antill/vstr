#include "../tst-main.c"

static const char *rf = __FILE__;

static unsigned int lens_fwd[4];
static int ret = 0;

static void tst_srch_buf(Vstr_base *t1, unsigned int off)
{
  TST_B_TST(ret, off + 1,
            VSTR_SRCH_CASE_CSTR_BUF_FWD(t1, 1, t1->len, "aBcD") != lens_fwd[0]);
  TST_B_TST(ret, off + 2,
            VSTR_SRCH_CASE_CSTR_BUF_REV(t1, 1, t1->len, "AbcD") != lens_fwd[3]);
  TST_B_TST(ret, off + 3,
            VSTR_SRCH_CASE_CSTR_BUF_FWD(t1, 1, t1->len, "Xyz ") != lens_fwd[1]);
  TST_B_TST(ret, off + 4,
            VSTR_SRCH_CASE_CSTR_BUF_REV(t1, 1, t1->len, "xyZ ") != lens_fwd[1]);
  TST_B_TST(ret, off + 5,
            VSTR_SRCH_CASE_CSTR_BUF_FWD(t1, 1, t1->len, "!& ")  != lens_fwd[2]);
  TST_B_TST(ret, off + 6,
            VSTR_SRCH_CASE_CSTR_BUF_REV(t1, 1, t1->len, "!& ")  != lens_fwd[2]);
  TST_B_TST(ret, off + 7,
            VSTR_SRCH_CASE_CSTR_BUF_FWD(t1, lens_fwd[0],
                                        t1->len - (lens_fwd[0] - 1),
                                        "AbCd ") != lens_fwd[0]);
  TST_B_TST(ret, off + 8,
            VSTR_SRCH_CASE_CSTR_BUF_REV(t1, lens_fwd[0],
                                        t1->len - (lens_fwd[0] - 1),
                                        "abCD ") != lens_fwd[0]);
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
  VSTR_ADD_CSTR_BUF(s1, s1->len, buf); /* norm */
  VSTR_ADD_CSTR_BUF(s3, s3->len, buf); /* small */
  VSTR_ADD_CSTR_BUF(s4, s4->len, buf); /* no iovec */

  ++lens_fwd[0]; /* convert to position of char after %n */
  ++lens_fwd[1];
  ++lens_fwd[2];
  ++lens_fwd[3];

  tst_srch_buf(s1, 0);
  tst_srch_buf(s3, 8);

  /* make sure it's got a iovec cache */
  vstr_export_iovec_ptr_all(s1, NULL, NULL);
  vstr_export_iovec_ptr_all(s3, NULL, NULL);
  
  tst_srch_buf(s1, 0);
  tst_srch_buf(s3, 8);
  tst_srch_buf(s4, 16);
  
  return (TST_B_RET(ret));
}
