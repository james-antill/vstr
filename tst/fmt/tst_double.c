#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  sprintf(buf, "%e %e %f %f %g %g %a %a",
          -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX,
          -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX);
  vstr_add_fmt(s1, 0, "%e %e %f %f %g %g %a %a",
               -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX,
               -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX);
  
  return (!VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
}
