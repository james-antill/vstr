/* TEST: abcd */
#include "tst-main.c"

static const char *rf = __FILE__;

#ifndef HAVE_POSIX_HOST
#define pipe(x) 1
#define write(x, y, z) (z)
#endif

int tst(void)
{
  int ret = 0;
  size_t tlen = strlen("/* TEST: abcd */\n");
  unsigned int err = 0;
  int mfail_count = 0;
  int pfds[2];

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
    TST_B_TST(ret, 9,
              !vstr_sc_read_len_file(s4, s4->len, __FILE__,
                                     s4->len, 0, NULL));
    if (ret) break;
  }

  TST_B_TST(ret, 10, !VSTR_CMP_CSTR_EQ(s4, 1, tlen, "/* TEST: abcd */\n"));

  vstr_del(s4, 1, s4->len);

  while (s4->len < tlen)
  {
    TST_B_TST(ret, 11,
              !vstr_sc_read_iov_file(s4, s4->len, __FILE__,
                                     s4->len, 2, 4, &err));
    TST_B_TST(ret, 11, (err != VSTR_TYPE_SC_READ_FD_ERR_NONE));
    TST_B_TST(ret, 11, (err != VSTR_TYPE_SC_READ_FILE_ERR_NONE));

    if (ret) break;
  }

  if (s4->len > tlen)
    vstr_del(s4, tlen + 1, s4->len - tlen);

  TST_B_TST(ret, 12, !VSTR_CMP_CSTR_EQ(s4, 1, s4->len, "/* TEST: abcd */\n"));

  TST_B_TST(ret, 13, !!vstr_sc_read_len_fd(s1, s1->len, -1, 0, &err));
  TST_B_TST(ret, 14, (err != VSTR_TYPE_SC_READ_FD_ERR_FSTAT_ERRNO));
  TST_B_TST(ret, 14, (err != VSTR_TYPE_SC_READ_FILE_ERR_FSTAT_ERRNO));

  TST_B_TST(ret, 15, !!vstr_sc_read_iov_fd(s1, s1->len, -1, 1, 2, &err));
  TST_B_TST(ret, 16, (err != VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO));
  TST_B_TST(ret, 16, (err != VSTR_TYPE_SC_READ_FILE_ERR_READ_ERRNO));

  /* no cache */
  TST_B_TST(ret, 15, !!vstr_sc_read_iov_fd(s4, s4->len, -1, 1, 2, &err));
  TST_B_TST(ret, 16, (err != VSTR_TYPE_SC_READ_FD_ERR_READ_ERRNO));
  TST_B_TST(ret, 16, (err != VSTR_TYPE_SC_READ_FILE_ERR_READ_ERRNO));

  TST_B_TST(ret, 17, !!vstr_sc_read_len_file(s1, s1->len, "/dev/null",
                                             0, 20, &err));
  TST_B_TST(ret, 18, (err != VSTR_TYPE_SC_READ_FD_ERR_EOF));
  TST_B_TST(ret, 18, (err != VSTR_TYPE_SC_READ_FILE_ERR_EOF));

  TST_B_TST(ret, 19, vstr_sc_read_len_file(s1, s1->len, "/abcd_missing",
                                           0, 1, &err));
  TST_B_TST(ret, 20, (err != VSTR_TYPE_SC_READ_FILE_ERR_OPEN_ERRNO));

  TST_B_TST(ret, 19, vstr_sc_read_iov_file(s1, s1->len, "/abcd_missing",
                                           0, 4, 4, &err));
  TST_B_TST(ret, 20, (err != VSTR_TYPE_SC_READ_FILE_ERR_OPEN_ERRNO));

  TST_B_TST(ret, 19, vstr_sc_read_len_file(s1, s1->len, "/abcd_missing",
                                           0, 0, &err));
  TST_B_TST(ret, 20, (err != VSTR_TYPE_SC_READ_FILE_ERR_OPEN_ERRNO));

  TST_B_TST(ret, 21, vstr_sc_read_len_file(s1, s1->len, __FILE__,
                                           200 * 1000, 0, &err));
  TST_B_TST(ret, 22, (err   != VSTR_TYPE_SC_READ_FILE_ERR_FSTAT_ERRNO));
  TST_B_TST(ret, 22, (errno != ENOSPC));

  vstr_del(s1, 1, s1->len);
  vstr_del(s4, 1, s4->len);

  mfail_count = 0;
  do
  {
    vstr_free_spare_nodes(s1->conf, VSTR_TYPE_NODE_BUF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_sc_read_len_file(s1, s1->len, __FILE__, 0, 1, &err) &&
           (err == VSTR_TYPE_SC_READ_FILE_ERR_MEM));
  TST_B_TST(ret, 23, (mfail_count == 1) && MFAIL_NUM_OK);

  mfail_count = 0;
  do
  {
    vstr_free_spare_nodes(s1->conf, VSTR_TYPE_NODE_BUF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_sc_read_iov_file(s1, s1->len, __FILE__, 0, 1, 1, &err) &&
           (err == VSTR_TYPE_SC_READ_FILE_ERR_MEM));
  TST_B_TST(ret, 23, (mfail_count == 1) && MFAIL_NUM_OK);

  mfail_count = 0;
  do
  {
    vstr_free_spare_nodes(s4->conf, VSTR_TYPE_NODE_BUF, 1000);
    vstr_free_spare_nodes(s4->conf, VSTR_TYPE_NODE_REF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_sc_read_len_file(s4, s4->len, __FILE__, 0, 1, &err) &&
           (err == VSTR_TYPE_SC_READ_FILE_ERR_MEM));
  TST_B_TST(ret, 24, (mfail_count == 1) && MFAIL_NUM_OK);

  mfail_count = 0;
  do
  {
    vstr_free_spare_nodes(s4->conf, VSTR_TYPE_NODE_BUF, 1000);
    vstr_free_spare_nodes(s4->conf, VSTR_TYPE_NODE_REF, 1000);
    tst_mfail_num(++mfail_count);
  } while (!vstr_sc_read_iov_file(s4, s4->len, __FILE__, 0, 1, 1, &err) &&
           (err == VSTR_TYPE_SC_READ_FILE_ERR_MEM));
  TST_B_TST(ret, 24, (mfail_count == 1) && MFAIL_NUM_OK);

  tst_mfail_num(0);

  ASSERT(VSTR_TYPE_SC_READ_FILE_ERR_MEM == VSTR_TYPE_SC_READ_FD_ERR_MEM);


  if (pipe(pfds) == -1)
    return (EXIT_FAILURE);

  if (write(pfds[1], "ab", 2) != 2)
    return (EXIT_FAILURE);
  TST_B_TST(ret, 25, !vstr_sc_read_len_fd(s1, s1->len, pfds[0], 1, NULL));

  TST_B_TST(ret, 27, !VSTR_CMP_CSTR_EQ(s1, s1->len, 1, "a"));

  TST_B_TST(ret, 25, !vstr_sc_read_len_fd(s1, s1->len, pfds[0], 32, NULL));

  TST_B_TST(ret, 27, !VSTR_CMP_CSTR_EQ(s1, s1->len - 1, 2, "ab"));

  if (write(pfds[1], "Z", 1) != 1)
    return (EXIT_FAILURE);
  TST_B_TST(ret, 26, !vstr_sc_read_iov_fd(s1, s1->len, pfds[0], 1, 2, NULL));

  TST_B_TST(ret, 27, !VSTR_CMP_CSTR_EQ(s1, s1->len - 2, 3, "abZ"));

#ifdef __linux__
  /* hangs on FreeBSD ... */
  TST_B_TST(ret, 28, vstr_sc_read_len_file(s1, s1->len, "/dev/stdin",
                                           1, 1, &err));
  TST_B_TST(ret, 29, (err != VSTR_TYPE_SC_READ_FILE_ERR_SEEK_ERRNO));
  TST_B_TST(ret, 28, vstr_sc_read_iov_file(s1, s1->len, "/dev/stdin",
                                           1, 4, 4, &err));
  TST_B_TST(ret, 29, (err != VSTR_TYPE_SC_READ_FILE_ERR_SEEK_ERRNO));
#endif

  TST_B_TST(ret, 30, !vstr_sc_read_iov_fd(s1, s1->len, -1, 0, 0, NULL));

  return (TST_B_RET(ret));
}
/* Crap for tst_coverage constants... None trivial to test.
 *
 * VSTR_TYPE_SC_READ_FD_ERR_TOO_LARGE
 * VSTR_TYPE_SC_READ_FILE_ERR_CLOSE_ERRNO
 * VSTR_TYPE_SC_READ_FILE_ERR_TOO_LARGE
 *
 */
