#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  unsigned int lens_fwd[4];
  int ret = 0;
  
#ifdef USE_RESTRICTED_HEADERS /* %n doesn't work in dietlibc */
  return (EXIT_FAILED_OK);
#endif
  
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

  VSTR_ADD_CSTR_BUF(s4, 0, "abcd xyz !& ");
  /*                        123456789 12   */
  
  TST_B_TST(ret, 1,
            vstr_srch_vstr_fwd(s3, 1, s3->len, s4, 1, 4) != lens_fwd[0]);
  TST_B_TST(ret, 2,
            vstr_srch_vstr_rev(s3, 1, s3->len, s4, 1, 4) != lens_fwd[3]);
  TST_B_TST(ret, 3,
            vstr_srch_vstr_fwd(s3, 1, s3->len, s4, 6, 4) != lens_fwd[1]);
  TST_B_TST(ret, 4,
            vstr_srch_vstr_rev(s3, 1, s3->len, s4, 6, 4) != lens_fwd[1]);
  TST_B_TST(ret, 5,
            vstr_srch_vstr_fwd(s3, 1, s3->len, s4, 10, 3)  != lens_fwd[2]);
  TST_B_TST(ret, 6,
            vstr_srch_vstr_rev(s3, 1, s3->len, s4, 10, 3)  != lens_fwd[2]);
  TST_B_TST(ret, 7,
            vstr_srch_vstr_fwd(s3, lens_fwd[0], s3->len - (lens_fwd[0] - 1),
                               s4, 1, 5) != lens_fwd[0]);
  TST_B_TST(ret, 8,
            vstr_srch_vstr_rev(s3, lens_fwd[0], s3->len - (lens_fwd[0] - 1),
                               s4, 1, 5) != lens_fwd[0]);
  
  return (TST_B_RET(ret));
}
