#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  unsigned int lens_fwd[4];
  unsigned int lens_rev[4];
  int ret = 0;
  
  sprintf(buf, "%d%nabcd %d%nxyz %u%n!& %u%nabcd",
          INT_MAX,  lens_fwd + 0,
          INT_MIN,  lens_fwd + 1,
          0,        lens_fwd + 2,
          UINT_MAX, lens_fwd + 3);
  VSTR_ADD_CSTR_BUF(s3, s3->len, buf);

  lens_rev[0] = s3->len - lens_fwd[0];
  lens_rev[1] = s3->len - lens_fwd[1];
  lens_rev[2] = s3->len - lens_fwd[2];
  lens_rev[3] = s3->len - lens_fwd[3];
  
  TST_B_TST(ret, 1,
            VSTR_SPN_CSTR_CHRS_FWD(s3, 1, s3->len, "0123456789") !=
            lens_fwd[0]);
  TST_B_TST(ret, 2,
            VSTR_SPN_CSTR_CHRS_REV(s3, 1, s3->len, "abcd") !=
            lens_rev[3]);
  TST_B_TST(ret, 3,
            VSTR_SPN_CSTR_CHRS_FWD(s3, 1, s3->len, "0123456789abcd -") !=
            lens_fwd[1]);
  TST_B_TST(ret, 4,
            VSTR_SPN_CSTR_CHRS_REV(s3, 1, s3->len, "0123456789abcd -") !=
            lens_rev[2] - 2);
  TST_B_TST(ret, 5,
            VSTR_SPN_CSTR_CHRS_FWD(s3, 1, s3->len, "0123456789abcd -xyz") !=
            lens_fwd[2]);
  TST_B_TST(ret, 6,
            VSTR_SPN_CSTR_CHRS_REV(s3, 1, s3->len, "0123456789abcd -!&") !=
            lens_rev[1] - 3);
  TST_B_TST(ret, 7,
            VSTR_SPN_CSTR_CHRS_FWD(s3, lens_fwd[0] + 1, s3->len - lens_fwd[0],
                                   "abcd ") != 5);
  TST_B_TST(ret, 8,
            VSTR_SPN_CSTR_CHRS_REV(s3, lens_fwd[0] + 1, s3->len - lens_fwd[0],
                                   "abcd")  != 4);
  
  
  
  TST_B_TST(ret, 9,
            VSTR_CSPN_CSTR_CHRS_FWD(s3, 1, s3->len, "x") !=
            lens_fwd[1]);
  TST_B_TST(ret, 10,
            VSTR_CSPN_CSTR_CHRS_REV(s3, 1, s3->len, "x") !=
            lens_rev[1] - 1);
  TST_B_TST(ret, 11,
            VSTR_CSPN_CSTR_CHRS_FWD(s3, 1, s3->len, "!") !=
            lens_fwd[2]);
  TST_B_TST(ret, 12,
            VSTR_CSPN_CSTR_CHRS_REV(s3, 1, s3->len, "!x") !=
            lens_rev[2] - 1);
  TST_B_TST(ret, 13,
            VSTR_CSPN_CSTR_CHRS_FWD(s3, 1, s3->len, "0123456789") != 0);
  TST_B_TST(ret, 14,
            VSTR_CSPN_CSTR_CHRS_REV(s3, 1, s3->len, "0123456789") != 4);
  TST_B_TST(ret, 15,
            VSTR_CSPN_CSTR_CHRS_FWD(s3, 1, lens_fwd[3], "abcd") !=
            lens_fwd[0]);
  TST_B_TST(ret, 16,
            VSTR_CSPN_CSTR_CHRS_REV(s3, 1, lens_fwd[3], "abcd") !=
            (lens_fwd[3] - lens_fwd[0]) - 4);
  TST_B_TST(ret, 17,
            VSTR_CSPN_CSTR_CHRS_FWD(s3, lens_fwd[1] + 1, s3->len - lens_fwd[1],
                                    "0123456789") != 4);
  TST_B_TST(ret, 18,
            VSTR_CSPN_CSTR_CHRS_FWD(s3, lens_fwd[1] + 1, s3->len - lens_fwd[1],
                                    "0123456789") != 4);
  
  return (TST_B_RET(ret));
}
