#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  long double ldz = 0.0;
  
#ifdef USE_RESTRICTED_HEADERS /* sucky host sprintf() implementions */
  return (EXIT_FAILED_OK);
#endif

  sprintf(buf,        "%LE %Le %LF %Lf %LG %Lg %LA %La %#La %#20.4La %#Lg %#.Lg %.20Lg %.Lf %#.Lf %-20.8Le",
          ldz, ldz, ldz, ldz, 
          ldz, ldz, ldz, ldz, 
          ldz, ldz, ldz, ldz, 
          ldz, ldz, ldz, ldz);
  vstr_add_fmt(s1, 0, "%LE %Le %LF %Lf %LG %Lg %LA %La %#La %#20.4La %#Lg %#.Lg %.20Lg %.Lf %#.Lf %-20.8Le",
          ldz, ldz, ldz, ldz, 
          ldz, ldz, ldz, ldz, 
          ldz, ldz, ldz, ldz, 
          ldz, ldz, ldz, ldz);

  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
  
#ifdef FMT_DBL_none
  if (ret) return (TST_B_RET(ret));
  return (EXIT_FAILED_OK);
#endif
  
  sprintf(buf, "%La %LA",
          -LDBL_MAX, LDBL_MAX);
  vstr_add_fmt(s1, 0, "%La %LA",
               -LDBL_MAX, LDBL_MAX);
  
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);

  sprintf(buf,        "%Le %LE %Lg %LG %La %LA",
          -LDBL_MAX, LDBL_MAX, /* %Lf doesn't work atm. */
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  vstr_add_fmt(s1, 0, "%Le %LE %Lg %LG %La %LA",
               -LDBL_MAX, LDBL_MAX, /* %Lf doesn't work atm. */
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);

#if defined (__GNUC__) && (__GNUC__ == 2)
  /* this test the sign */
  ldz = ((union __convert_long_double) {__convert_long_double_i: {0x00000000, 0, 0x007ffe, 0x0}}).__convert_long_double_d;
  sprintf(buf,        "%Le %LE %Lf %LF %Lg %LG %La %LA",
          ldz, ldz, ldz, ldz,
          ldz, ldz, ldz, ldz);
  vstr_add_fmt(s1, 0, "%Le %LE %Lf %LF %Lg %LG %La %LA",
          ldz, ldz, ldz, ldz,
          ldz, ldz, ldz, ldz);
  
  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);

  ldz = ((union __convert_long_double) {__convert_long_double_i: {0xffffffff, 0xffffffff, 0x107e137, 0x0}}).__convert_long_double_d;
  sprintf(buf,        "%Le %LE %Lf %LF %Lg %LG %La %LA",
          ldz, ldz, ldz, ldz,
          ldz, ldz, ldz, ldz);
  vstr_add_fmt(s1, 0, "%Le %LE %Lf %LF %Lg %LG %La %LA",
          ldz, ldz, ldz, ldz,
          ldz, ldz, ldz, ldz);
  
  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
#endif
  
  sprintf(buf,        "%Le %LE %Lf %LF %Lg %LG %La %LA",
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  vstr_add_fmt(s1, 0, "%Le %LE %Lf %LF %Lg %LG %La %LA",
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);

  TST_B_TST(ret, 5, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);

  sprintf(buf,        "%Le %LE %'Lf %'LF %'Lg %'LG %La %LA",
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  vstr_add_fmt(s2, 0, "%Le %LE %'Lf %'LF %'Lg %'LG %La %LA",
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  TST_B_TST(ret, 6, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf));

  return (TST_B_RET(ret));
}
