/* TEST: abcd */
#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  int ret = 0;
  size_t tlen = strlen("/* TEST: abcd */\n");
  
#ifndef HAVE_POSIX_HOST
  return (EXIT_FAILED_OK);
#endif
  
  while (s1->len < tlen)
  {
    size_t left = tlen - s1->len;
    TST_B_TST(ret, 1, 
              !vstr_sc_read_len_file(s1, s1->len, __FILE__,
                                     s1->len, left, NULL));
    if (ret) break;
  }

  TST_B_TST(ret, 2, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "/* TEST: abcd */\n"));

  vstr_del(s1, 1, s1->len);
  
  while (s1->len < tlen)
  {
    TST_B_TST(ret, 3, 
              !vstr_sc_read_iov_file(s1, s1->len, __FILE__,
                                     s1->len, 2, 4, NULL));
    if (ret) break;
  }

  if (s1->len > tlen)
    vstr_del(s1, tlen + 1, s1->len - tlen);
  
  TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s1, 1, s1->len, "/* TEST: abcd */\n"));

  /* again with s3 ... Ie. small _buf nodes */
  while (s3->len < tlen)
  {
    size_t left = tlen - s3->len;
    TST_B_TST(ret, 5,
              !vstr_sc_read_len_file(s3, s3->len, __FILE__,
                                     s3->len, left, NULL));
    if (ret) break;
  }

  TST_B_TST(ret, 6, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, "/* TEST: abcd */\n"));

  vstr_del(s3, 1, s3->len);
  
  while (s3->len < tlen)
  {
    TST_B_TST(ret, 7, 
              !vstr_sc_read_iov_file(s3, s3->len, __FILE__,
                                     s3->len, 2, 4, NULL));
    if (ret) break;
  }

  if (s3->len > tlen)
    vstr_del(s3, tlen + 1, s3->len - tlen);
  
  TST_B_TST(ret, 8, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, "/* TEST: abcd */\n"));
  
  /* again with s4 ... Ie. no iovs */
  while (s4->len < tlen)
  {
    size_t left = tlen - s4->len;
    TST_B_TST(ret, 9, 
              !vstr_sc_read_len_file(s4, s4->len, __FILE__,
                                     s4->len, left, NULL));
    if (ret) break;
  }

  TST_B_TST(ret, 10, !VSTR_CMP_CSTR_EQ(s4, 1, s4->len, "/* TEST: abcd */\n"));

  vstr_del(s4, 1, s4->len);
  
  while (s4->len < tlen)
  {
    TST_B_TST(ret, 11, 
              !vstr_sc_read_iov_file(s4, s4->len, __FILE__,
                                     s4->len, 2, 4, NULL));
    if (ret) break;
  }

  if (s4->len > tlen)
    vstr_del(s4, tlen + 1, s4->len - tlen);
  
  TST_B_TST(ret, 12, !VSTR_CMP_CSTR_EQ(s4, 1, s4->len, "/* TEST: abcd */\n"));

  return (TST_B_RET(ret));
}
