#include "../tst-main.c"

static const char *rf = __FILE__;

static const char *fn = "make_check_tst_sc_tst_write.tmp";

#ifndef HAVE_POSIX_HOST
extern        int unlink(const char *);

# define O_RDONLY (-1)
# define O_WRONLY (-1)
# define O_EXCL (-1)
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
  unsigned int err = 0;
  
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
    TST_B_TST(ret, 5, (err != VSTR_TYPE_SC_WRITE_FILE_ERR_NONE));
    TST_B_TST(ret, 5, (err != VSTR_TYPE_SC_WRITE_FD_ERR_NONE));
  }

  read_file_s2(s2->len);
  VSTR_ADD_CSTR_BUF(s4, 0, buf);
  TST_B_TST(ret, 6, !VSTR_CMP_EQ(s4, 1, s4->len, s2, 1, s2->len));

  /* end -- get rid of file */
  unlink(fn);

  VSTR_ADD_CSTR_BUF(s1, 0, buf);  

  /* O_RD0NLY == 0, which means default flags ... as RDONLY isn't valid
   * (which is what we are testing for ... so hack it by including O_EXCL as
   * well, then it's non zero and hence won't use default */
  
  TST_B_TST(ret, 7, !!vstr_sc_write_file(s1, 1, s1->len, "/blah/.nothere",
                                         O_RDONLY | O_EXCL, 0666, 0, &err));

  TST_B_TST(ret, 8, (err != VSTR_TYPE_SC_WRITE_FILE_ERR_OPEN_ERRNO));

  TST_B_TST(ret, 9, !!vstr_sc_write_file(s1, 1, s1->len, "/dev/stdin",
                                         O_RDONLY | O_EXCL, 0666, 1, &err));
  
  TST_B_TST(ret, 10, (err != VSTR_TYPE_SC_WRITE_FILE_ERR_SEEK_ERRNO));
  
  TST_B_TST(ret, 11, !!vstr_sc_write_file(s1, 1, s1->len, rf,
                                          O_RDONLY | O_EXCL, 0666, 1, &err));
  
  TST_B_TST(ret, 12, (err != VSTR_TYPE_SC_WRITE_FILE_ERR_WRITE_ERRNO));
            

  TST_B_TST(ret, 13, !!vstr_sc_write_fd(s1, 1, s1->len, 1024, &err));
  
  TST_B_TST(ret, 14, (err != VSTR_TYPE_SC_WRITE_FD_ERR_WRITE_ERRNO));

  return (TST_B_RET(ret));
}
/* Crap for tst_coverage constants... None trivial to test.
 *
 * VSTR_TYPE_SC_WRITE_FD_ERR_MEM
 * VSTR_TYPE_SC_WRITE_FILE_ERR_CLOSE_ERRNO
 * VSTR_TYPE_SC_WRITE_FILE_ERR_MEM
 *
 */
