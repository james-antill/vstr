#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  vstr_del(s1, 1, 0); /* empty */
  vstr_del(s1, 1, 1); /* empty with length */
  vstr_del(s1, 2, 2); /* empty with length at offset */
  
  return (0);
}
