#include "../tst-main.c"

static const char *rf = __FILE__;

static const char *fn = "make_check_tst_sc_tst_write.tmp";

#ifndef HAVE_POSIX_HOST
extern        int unlink(const char *);

# define O_WRONLY (-1)
# define O_CREAT (-1)
# define O_APPEND (-1)
#endif

static void read_file_s2(size_t tlen)
{
  vstr_del(s2, 1, s2->len);
  while (s2->len < tlen)
  {
    size_t left = tlen - s2->len;
    if (!vstr_sc_read_len_file(s2, s2->len, fn, s2->len, left, NULL))
      return;
  }
}

int tst(void)
{
  int ret = 0;
  size_t tmp = 0;
  
#ifndef HAVE_POSIX_HOST
  return (EXIT_FAILED_OK);
#endif
  
  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  VSTR_ADD_CSTR_BUF(s2, 0, buf);
  VSTR_ADD_CSTR_BUF(s3, 0, buf);
  VSTR_ADD_CSTR_BUF(s4, 0, buf);

  unlink(fn);
  tmp = s1->len;
  while (s1->len)
  {
    size_t off = tmp - s1->len;
    TST_B_TST(ret, 1,
              !vstr_sc_write_file(s1, 1, s1->len, fn,
                                  O_WRONLY | O_CREAT, 0600, off, NULL));
  }

  read_file_s2(s2->len);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  unlink(fn);
  tmp = s3->len;
  while (s3->len)
  {
    size_t off = tmp - s3->len;
    TST_B_TST(ret, 3, 
              !vstr_sc_write_file(s3, 1, s3->len, fn,
                                  O_WRONLY | O_CREAT, 0600, off, NULL));
  }

  read_file_s2(s2->len);
  VSTR_ADD_CSTR_BUF(s3, 0, buf);
  TST_B_TST(ret, 4, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));
  
  unlink(fn);
  tmp = s4->len;
  while (s4->len)
  {
    size_t off = tmp - s4->len;
    TST_B_TST(ret, 5, 
              !vstr_sc_write_file(s4, 1, s4->len, fn,
                                  O_WRONLY | O_CREAT, 0600, off, NULL));
  }

  read_file_s2(s2->len);
  VSTR_ADD_CSTR_BUF(s4, 0, buf);
  TST_B_TST(ret, 6, !VSTR_CMP_EQ(s4, 1, s4->len, s2, 1, s2->len));

  unlink(fn);
  tmp = s1->len;
  while (s1->len)
  {
    TST_B_TST(ret, 1,
              !vstr_sc_write_file(s1, 1, s1->len, fn,
                                  O_WRONLY | O_CREAT |O_APPEND, 0600, 0, NULL));
  }

  read_file_s2(s2->len);
  VSTR_ADD_CSTR_BUF(s1, 0, buf);  
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  unlink(fn);
  tmp = s3->len;
  while (s3->len)
  {
    TST_B_TST(ret, 3, 
              !vstr_sc_write_file(s3, 1, s3->len, fn,
                                  O_WRONLY | O_CREAT |O_APPEND, 0600, 0, NULL));
  }

  read_file_s2(s2->len);
  VSTR_ADD_CSTR_BUF(s3, 0, buf);
  TST_B_TST(ret, 4, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));
  
  unlink(fn);
  tmp = s4->len;
  while (s4->len)
  {
    TST_B_TST(ret, 5, 
              !vstr_sc_write_file(s4, 1, s4->len, fn,
                                  O_WRONLY | O_CREAT |O_APPEND, 0600, 0, NULL));
  }

  read_file_s2(s2->len);
  VSTR_ADD_CSTR_BUF(s4, 0, buf);
  TST_B_TST(ret, 6, !VSTR_CMP_EQ(s4, 1, s4->len, s2, 1, s2->len));

  /* end -- get rid of file */
  unlink(fn);
  
  return (TST_B_RET(ret));
}
