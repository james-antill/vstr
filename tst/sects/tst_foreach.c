#include "../tst-main.c"

static const char *rf = __FILE__;

static unsigned int s_foreach(const Vstr_base *t1, size_t pos, size_t len,
                              void *data)
{
  int *vals = data;
  
  (void)t1;

  ++vals[1];
  
  TST_B_TST(vals[0], 4 + vals[1], vals[1] != pos);
  TST_B_TST(vals[0], 4 + vals[1], (vals[1] << 1) != len);

  return (0);
}

int tst(void)
{
  Vstr_sects *sects = vstr_sects_make(4);
  int vals[2] = {0, 0};

  TST_B_TST(vals[0], 1, (sects->sz != 4));
  TST_B_TST(vals[0], 2, (sects->num != 0));
  
  vstr_sects_add(sects, 1, 2);
  vstr_sects_add(sects, 2, 4);
  vstr_sects_add(sects, 3, 6);
  vstr_sects_add(sects, 4, 8);
  vstr_sects_add(sects, 5, 10);

  TST_B_TST(vals[0], 3, (sects->sz != 8));
  TST_B_TST(vals[0], 4, (sects->num != 5));

  vstr_sects_foreach(s1, sects, 0, s_foreach, vals);

  return (TST_B_RET(vals[0]));
}
