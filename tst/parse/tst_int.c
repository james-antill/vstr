#include "../tst-main.c"

static const char *rf = __FILE__;

unsigned int FLAGS = (0 |
#if 0
                      VSTR_FLAG_PARSE_NUM_LOCAL |
                      VSTR_FLAG_PARSE_NO_BEG_ZERO |
#endif
                      VSTR_FLAG_PARSE_NUM_SEP | 
                      VSTR_FLAG_PARSE_NUM_OVERFLOW | 
                      VSTR_FLAG_PARSE_NUM_SPACE |
                      0);

static int do_test_int(const char *num_str, size_t num_len,
                       size_t *len, unsigned int *err)
{
  int ret = 0;
  Vstr_base *str2 = vstr_dup_ptr(NULL, num_str, num_len);
  if (!str2)
    die();

  ret = vstr_parse_int(str2, 1, str2->len, FLAGS, len, err);

  vstr_free_base(str2);

  return (ret);
}

static unsigned int do_test_uint(const char *num_str, size_t num_len,
                                 size_t *len, unsigned int *err)
{
  unsigned int ret = 0;
  Vstr_base *str2 = vstr_dup_ptr(NULL, num_str, num_len);
  if (!str2)
    die();
  
  ret = vstr_parse_uint(str2, 1, str2->len, FLAGS, len, err);

  vstr_free_base(str2);

  return (ret);
}

#define DO_TEST_SINTSTR(x, y, z) do { \
  size_t num_len = 0; \
  unsigned int err = 0; \
  vstr_del(s1, 1, s1->len); \
  vstr_del(s2, 1, s2->len); \
  VSTR_ADD_CSTR_BUF(s2, 0, y); \
  vstr_add_fmt(s1, 0, z, do_test_int(x, strlen(x), &num_len, &err)); \
  if (!VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len)) return (1); \
} while (FALSE)

#define DO_TEST_UINTSTR(x, y, z) do { \
  size_t num_len = 0; \
  unsigned int err = 0; \
  vstr_del(s1, 1, s1->len); \
  vstr_del(s2, 1, s2->len); \
  VSTR_ADD_CSTR_BUF(s2, 0, y); \
  vstr_add_fmt(s1, 0, z, do_test_uint(x, strlen(x), &num_len, &err)); \
  if (!VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len)) return (2); \
} while (FALSE)

#define DO_TEST_NUM(x, y, z) do { \
  size_t num_len = 0; \
  unsigned int err = 0; \
  vstr_del(s1, 1, s1->len); \
  vstr_del(s2, 1, s2->len); \
  vstr_add_fmt(s2, 0, y, x); \
  vstr_add_fmt(s1, 0, y, \
               vstr_parse_ ## z (s2, 1, s2->len, FLAGS, &num_len, &err)); \
  if (!VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len)) return (3); \
} while (FALSE)

int tst(void)
{
  DO_TEST_SINTSTR("1234", "1234", "%d");
  DO_TEST_SINTSTR("-1234", "-1234", "%d");
  DO_TEST_SINTSTR("+1234", "1234", "%d");
  DO_TEST_SINTSTR("1_234", "1234", "%d");

  DO_TEST_UINTSTR("  0x1234", "0x1234", "%#x");
  DO_TEST_UINTSTR("0x1__23__4", "0x1234", "%#x");
  DO_TEST_UINTSTR("   0x00001__23__4", "0x1234", "%#x");
  DO_TEST_UINTSTR("012_34", "01234", "%#o");
  DO_TEST_UINTSTR("000012_34", "01234", "%#o");

  DO_TEST_NUM(INT_MAX, "%d", int);
  DO_TEST_NUM(INT_MIN, "%d", int);
  DO_TEST_NUM(UINT_MAX, "%u", uint);
  DO_TEST_NUM(UINT_MAX, "%#x", uint);
  DO_TEST_NUM(UINT_MAX, "%#o", uint);
  DO_TEST_NUM(INTMAX_MIN, "%jd", intmax);
  DO_TEST_NUM(UINTMAX_MAX, "%ju", uintmax);
  
  DO_TEST_UINTSTR("0xFFFFFFFFFFFF", "0XFFFFFFFF", "%#X");
  
  DO_TEST_SINTSTR("0xffffffff", "0x7fffffff", "%#x");
  DO_TEST_SINTSTR("-0xFFFFFFFF", "-2147483648", "%d");
  DO_TEST_SINTSTR("0xFFFFFFFFFFFF", "0X7FFFFFFF", "%#X");
  DO_TEST_SINTSTR("-0xFFFFFFFFFFFF", "-2147483648", "%d");
  
  return (0);
}
