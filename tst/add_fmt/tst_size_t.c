#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  ssize_t sn[4] = {INT_MAX,  INT_MIN};
  size_t  un[4] = {      0, UINT_MAX};

  sprintf(buf, "%zd%zu", sn[0], un[1]);
  if (!strcmp(buf, "zdzu"))
    return (EXIT_FAILED_OK); /* sucky host sprintf() implementions */
  
  sprintf(buf, "%zd %zd %zu %zu", sn[0], sn[1], un[0], un[1]);
  vstr_add_fmt(s1, 0, "%zd %zd %zu %zu", sn[0], sn[1], un[0], un[1]);

  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));
  
  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s1, 0, "%'zd", (size_t)1000);
  
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "1000"));

  sprintf(buf, "%'zd %'zd %'zu %'zu", sn[0], sn[1], un[0], un[1]);
  vstr_add_fmt(s2, 0, "%'zd %'zd %'zu %'zu", sn[0], sn[1], un[0], un[1]);
  
  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s2, 1, s2->len, buf));

  return (TST_B_RET(ret));
}
