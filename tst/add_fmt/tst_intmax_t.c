#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%jd %jd %ju %ju",
          INTMAX_MAX, INTMAX_MIN, (intmax_t)0, UINTMAX_MAX);
  vstr_add_fmt(s1, 0, "%jd %jd %ju %ju",
               INTMAX_MAX, INTMAX_MIN, (intmax_t)0, UINTMAX_MAX);
  ret |= !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf);
  
  sprintf(buf, "%'jd %'jd %'ju %'ju",
          INTMAX_MAX, INTMAX_MIN, (intmax_t)0, UINTMAX_MAX);
  vstr_add_fmt(s2, 0, "%'jd %'jd %'ju %'ju",
               INTMAX_MAX, INTMAX_MIN, (intmax_t)0, UINTMAX_MAX);
  ret |= !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf);

  return (ret);
}
