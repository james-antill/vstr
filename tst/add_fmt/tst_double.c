#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
#if 0
#ifdef USE_RESTRICTED_HEADERS /* sucky host sprintf() implementions */
  return (EXIT_FAILED_OK);
#endif

  sprintf(buf,        "%E %e %F %f %G %g %A %a %#a %#20.4a %#g %#.g %.20g %.f %#.f %-20.8e",
          0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0);
  vstr_add_fmt(s1, 0, "%E %e %F %f %G %g %A %a %#a %#20.4a %#g %#.g %.20g %.f %#.f %-20.8e",
               0.0, 0.0, 0.0, 0.0,
               0.0, 0.0, 0.0, 0.0,
               0.0, 0.0, 0.0, 0.0,
               0.0, 0.0, 0.0, 0.0);
  
  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
  
#ifdef FMT_DBL_none
  if (ret) return (TST_B_RET(ret));
  return (EXIT_FAILED_OK);
#endif
  
  sprintf(buf, "%a %A",
          -DBL_MAX, DBL_MAX);
  vstr_add_fmt(s1, 0, "%a %A",
               -DBL_MAX, DBL_MAX);
  
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);
#endif
  sprintf(buf,        "%E %e %F %f %G %g %A %a",
          -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX,
          -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX);
  vstr_add_fmt(s1, 0, "%E %e %F %f %G %g %A %a",
               -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX,
               -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX);
  
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  vstr_del(s1, 1, s1->len);

  sprintf(buf,        "%E %e %'F %'f %'G %'g %A %a",
          -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX,
          -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX);
  vstr_add_fmt(s2, 0, "%E %e %'F %'f %'G %'g %A %a",
               -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX,
               -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX);
  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf));

  return (TST_B_RET(ret));
}
