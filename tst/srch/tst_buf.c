#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  unsigned int lens_fwd[4];
  int ret = 0;
  
#ifdef USE_RESTRICTED_HEADERS /* %n doesn't work in dietlibc */
  return (EXIT_FAILED_OK);
#endif

  /* FIXME: do with iovec and without */
  
  sprintf(buf, "%d%nabcd %d%nxyz %u%n!& %u%nabcd",
          INT_MAX,  lens_fwd + 0,
          INT_MIN,  lens_fwd + 1,
          0,        lens_fwd + 2,
          UINT_MAX, lens_fwd + 3);
  VSTR_ADD_CSTR_BUF(s3, s3->len, buf);

  ++lens_fwd[0]; /* convert to position of char after %n */
  ++lens_fwd[1];
  ++lens_fwd[2];
  ++lens_fwd[3];
  
  TST_B_TST(ret, 1,
            VSTR_SRCH_CSTR_BUF_FWD(s3, 1, s3->len, "abcd") != lens_fwd[0]);
  TST_B_TST(ret, 2,
            VSTR_SRCH_CSTR_BUF_REV(s3, 1, s3->len, "abcd") != lens_fwd[3]);
  TST_B_TST(ret, 3,
            VSTR_SRCH_CSTR_BUF_FWD(s3, 1, s3->len, "xyz ") != lens_fwd[1]);
  TST_B_TST(ret, 4,
            VSTR_SRCH_CSTR_BUF_REV(s3, 1, s3->len, "xyz ") != lens_fwd[1]);
  TST_B_TST(ret, 5,
            VSTR_SRCH_CSTR_BUF_FWD(s3, 1, s3->len, "!& ")  != lens_fwd[2]);
  TST_B_TST(ret, 6,
            VSTR_SRCH_CSTR_BUF_REV(s3, 1, s3->len, "!& ")  != lens_fwd[2]);
  TST_B_TST(ret, 7,
            VSTR_SRCH_CSTR_BUF_FWD(s3, lens_fwd[0], s3->len - (lens_fwd[0] - 1),
                                   "abcd ") != lens_fwd[0]);
  TST_B_TST(ret, 8,
            VSTR_SRCH_CSTR_BUF_REV(s3, lens_fwd[0], s3->len - (lens_fwd[0] - 1),
                                   "abcd ") != lens_fwd[0]);
  
  return (TST_B_RET(ret));
}
