#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t pos = 0;
  size_t len = 0;
  size_t dlen = 0;
  unsigned int buf_pos = 0;
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);

  vstr_add_fmt(s1, s1->len, "%d:%n%s,", strlen(buf), &buf_pos, buf);
  ++buf_pos;
  
  len = vstr_parse_netstr(s1, 1, s1->len, &pos, &dlen);
  TST_B_TST(ret, 1,
            (len != s1->len));
  TST_B_TST(ret, 2,
            (dlen != strlen(buf)));
  TST_B_TST(ret, 3,
            (pos != buf_pos));
  
  return (TST_B_RET(ret));
}
