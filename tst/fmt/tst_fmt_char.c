#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  
  sprintf(buf, "%d %d %u %u", SCHAR_MAX, SCHAR_MIN, 0, UCHAR_MAX);
  vstr_add_fmt(s1, 0, "%hhd %hhd %hhu %hhu",
               SCHAR_MAX, SCHAR_MIN, 0, UCHAR_MAX);

  TST_B_TST(ret, 1, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, buf));

  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s1, 0, "%c%3c%-3c%c", 'a', 'b', 'c', 'd');
  
  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "a  bc  d"));

  vstr_del(s1, 1, s1->len);
  vstr_add_fmt(s1, 0, "%lc%3lc%-3lc%C",
               (wint_t) L'a', (wint_t) L'b', (wint_t) L'c', (wint_t) L'd');

  TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "a  bc  d"));


  {
    int mfail_count = 0;
    /* test for memory failures... */
    
    do
    {
      tst_mfail_num(++mfail_count);
    } while (!vstr_add_fmt(s3, 0, "%c%3c%-3c%c" "%lc%3lc%-3lc%C",
                           'a', 'b', 'c', 'd',
                           (wint_t) L'a', (wint_t) L'b',
                           (wint_t) L'c', (wint_t) L'd'));
    
    TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len,
                                        "a  bc  d" "a  bc  d"));
    
    tst_mfail_num(0);
  }

  return (TST_B_RET(ret));
}
