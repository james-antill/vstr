#include "../tst-main.c"

static const char *rf = __FILE__;

static unsigned int s_foreach(const Vstr_base *t1 __attribute__((unused)),
                              size_t pos, size_t len,
                              void *data)
{
  unsigned int *vals = data;

  ++vals[1];

  if (!len)
    return (VSTR_TYPE_SECTS_FOREACH_DEL);
  
  TST_B_TST(vals[0], 4 + vals[1], vals[1] != pos);
  TST_B_TST(vals[0], 4 + vals[1], (vals[1] << 1) != len);

  return (VSTR_TYPE_SECTS_FOREACH_DEF);
}

static unsigned int s_ret_foreach(const Vstr_base *t1 __attribute__((unused)),
                                  size_t pos __attribute__((unused)),
                                  size_t len __attribute__((unused)),
                                  void *data)
{
  unsigned int *vals = data;

  ++vals[1];

  TST_B_TST(vals[0], 28, (pos !=  5));
  TST_B_TST(vals[0], 29, (len != 10));
  
  return (VSTR_TYPE_SECTS_FOREACH_RET);
}

int tst(void)
{
  Vstr_sects *sects = vstr_sects_make(4);
  unsigned int vals[2] = {0, 0};

  TST_B_TST(vals[0], 1, (sects->sz != 4));
  TST_B_TST(vals[0], 2, (sects->num != 0));
  
  vstr_sects_add(sects, 1, 2);
  vstr_sects_add(sects, 2, 4);
  vstr_sects_add(sects, 3, 6);
  vstr_sects_add(sects, 4, 8);
  vstr_sects_add(sects, 5, 10);
  vstr_sects_add(sects, 6, 0);

  TST_B_TST(vals[0], 3, (sects->sz  != 8));
  TST_B_TST(vals[0], 4, (sects->num != 6));

  vstr_sects_foreach(s1, sects, VSTR_FLAG_SECTS_FOREACH_DEF, s_foreach, vals);
  TST_B_TST(vals[0], 12, (vals[1] != 5));

  vals[1] = 0;
  vstr_sects_foreach(s1, sects, VSTR_FLAG_SECTS_FOREACH_ALLOW_NULL,
                     s_foreach, vals);
  
  TST_B_TST(vals[0], 16, (sects->num != 5));
  TST_B_TST(vals[0], 17, (vals[1] != 6));
  
  vals[1] = 0;
  vstr_sects_foreach(s1, sects, VSTR_FLAG_SECTS_FOREACH_ALLOW_NULL,
                     s_foreach, vals);
  
  TST_B_TST(vals[0], 16, (sects->num != 5));
  TST_B_TST(vals[0], 17, (vals[1] != 5));
  
  vals[1] = 0;
  vstr_sects_foreach(s1, sects, VSTR_FLAG_SECTS_FOREACH_BACKWARD,
                     s_ret_foreach, vals);
  
  TST_B_TST(vals[0], 30, (vals[1] != 1));
  
  return (TST_B_RET(vals[0]));
}
