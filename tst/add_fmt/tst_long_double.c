#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;

#ifdef FMT_DBL_none
  return (EXIT_FAILED_OK);
#endif
  
  sprintf(buf, "%La %LA",
          -LDBL_MAX, LDBL_MAX);
  vstr_add_fmt(s1, 0, "%La %LA",
               -LDBL_MAX, LDBL_MAX);
  
  ret |= !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf);
  vstr_del(s1, 1, s1->len);

#ifdef FMT_DBL_glibc /* only has %a implemented atm. */
  if (ret) return (ret);
  return (EXIT_FAILED_OK);
#endif
  
  sprintf(buf,        "%Le %LE %Lf %LF %Lg %LG %La %LA",
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  vstr_add_fmt(s1, 0, "%Le %LE %Lf %LF %Lg %LG %La %LA",
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);

  ret |= 2 * !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf);
  vstr_del(s1, 1, s1->len);

  sprintf(buf,        "%Le %LE %'Lf %'LF %'Lg %'LG %La %LA",
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
          -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  vstr_add_fmt(s2, 0, "%Le %LE %'Lf %'LF %'Lg %'LG %La %LA",
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX,
               -LDBL_MAX, LDBL_MAX, -LDBL_MAX, LDBL_MAX);
  ret |= 4 * !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf);

  return (ret);
}
