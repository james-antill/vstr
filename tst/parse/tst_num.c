#include "../tst-main.c"

static const char *rf = __FILE__;

unsigned int FLAGS = (0 |
                      VSTR_FLAG_PARSE_NUM_SEP | 
                      VSTR_FLAG_PARSE_NUM_OVERFLOW | 
                      VSTR_FLAG_PARSE_NUM_SPACE |
                      0);

#define TST_MAKE_TST_FUNC(func_T, real_T) \
static real_T do_test_ ## func_T (const char *num_str, size_t num_len, \
                                  size_t *len, unsigned int *err) \
{ \
  real_T ret = 0; \
  Vstr_base *str2 = vstr_dup_ptr(NULL, num_str, num_len); \
  if (!str2) \
    die(); \
  \
  ret = vstr_parse_ ## func_T (str2, 1, str2->len, FLAGS, len, err); \
  \
  vstr_free_base(str2); \
  \
  return (ret); \
}

TST_MAKE_TST_FUNC(short, short)
TST_MAKE_TST_FUNC(ushort, unsigned short)
TST_MAKE_TST_FUNC(int, int)
TST_MAKE_TST_FUNC(uint, unsigned int)
TST_MAKE_TST_FUNC(intmax, intmax_t)
TST_MAKE_TST_FUNC(uintmax, uintmax_t)


#define DO_TEST_BEG() \
  size_t num_len = 0; \
  unsigned int err = 0; \
  vstr_del(s1, 1, s1->len); \
  vstr_del(s2, 1, s2->len)

#define DO_TEST_END() \
  if (!VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len)) return (1)

#define DO_TEST_NUMSTR(x, y, z, T) do { \
  DO_TEST_BEG(); \
  VSTR_ADD_CSTR_BUF(s2, 0, y); \
  vstr_add_fmt(s1, 0, z, do_test_ ## T (x, strlen(x), &num_len, &err)); \
  DO_TEST_END(); \
} while (FALSE)

#define DO_TEST_NUM(x, y, z) do { \
  DO_TEST_BEG(); \
  vstr_add_fmt(s2, 0, y, x); \
  vstr_add_fmt(s1, 0, y, \
               vstr_parse_ ## z (s2, 1, s2->len, FLAGS, &num_len, &err)); \
  DO_TEST_END(); \
} while (FALSE)

int tst(void)
{
  DO_TEST_NUMSTR("1234", "1234", "%hd", short);
  DO_TEST_NUMSTR("-1234", "-1234", "%hd", short);
  DO_TEST_NUMSTR("+1234", "1234", "%hd", short);
  DO_TEST_NUMSTR("1_234", "1234", "%hd", short);

  DO_TEST_NUMSTR("  0x1234", "0x1234", "%#hx", ushort);
  DO_TEST_NUMSTR("0x1__23__4", "0x1234", "%#hx", ushort);
  DO_TEST_NUMSTR("   0x00001__23__4", "0x1234", "%#hx", ushort);
  DO_TEST_NUMSTR("012_34", "01234", "%#ho", ushort);
  DO_TEST_NUMSTR("000012_34", "01234", "%#ho", ushort);

  DO_TEST_NUMSTR("1234", "1234", "%d", int);
  DO_TEST_NUMSTR("-1234", "-1234", "%d", int);
  DO_TEST_NUMSTR("+1234", "1234", "%d", int);
  DO_TEST_NUMSTR("1_234", "1234", "%d", int);

  DO_TEST_NUMSTR("  0x1234", "0x1234", "%#x", uint);
  DO_TEST_NUMSTR("0x1__23__4", "0x1234", "%#x", uint);
  DO_TEST_NUMSTR("   0x00001__23__4", "0x1234", "%#x", uint);
  DO_TEST_NUMSTR("012_34", "01234", "%#o", uint);
  DO_TEST_NUMSTR("000012_34", "01234", "%#o", uint);

  DO_TEST_NUM(SHRT_MAX, "%hd", short);
  DO_TEST_NUM(SHRT_MIN, "%hd", short);
  DO_TEST_NUM(USHRT_MAX, "%hu", ushort);
  DO_TEST_NUM(USHRT_MAX, "%#hx", ushort);
  DO_TEST_NUM(USHRT_MAX, "%#ho", ushort);

  DO_TEST_NUM(INT_MAX, "%d", int);
  DO_TEST_NUM(INT_MIN, "%d", int);
  DO_TEST_NUM(UINT_MAX, "%u", uint);
  DO_TEST_NUM(UINT_MAX, "%#x", uint);
  DO_TEST_NUM(UINT_MAX, "%#o", uint);

  DO_TEST_NUM(LONG_MAX, "%ld", long);
  DO_TEST_NUM(LONG_MIN, "%ld", long);
  DO_TEST_NUM(ULONG_MAX, "%lu", ulong);
  DO_TEST_NUM(ULONG_MAX, "%#lx", ulong);
  DO_TEST_NUM(ULONG_MAX, "%#lo", ulong);
  
  sprintf(buf, "%jd%ju", INTMAX_MAX, UINTMAX_MAX);
  if (strcmp(buf, "jdju")) /* skip if sucky host sprintf() implemention */  
  {
    DO_TEST_NUM(INTMAX_MIN, "%jd", intmax);
    DO_TEST_NUM(UINTMAX_MAX, "%ju", uintmax);
    
    DO_TEST_NUMSTR("0xFfFFFFFFFFfFFFFFFFFF",  "0XFFFFFFFFFFFFFFFF", "%#jX",
                   uintmax);
    DO_TEST_NUMSTR("0xFFFFFFFFffffffff",      "0XFFFFFFFFFFFFFFFF", "%#jX",
                   uintmax);
    DO_TEST_NUMSTR("0xffffffffFFFFFFFF",      "0x7fffffffffffffff", "%#jx",
                   intmax);
    DO_TEST_NUMSTR("0xFFFFFFFFFFFFFFFF",      "0X7FFFFFFFFFFFFFFF", "%#jX",
                   intmax);
    DO_TEST_NUMSTR("-0xFFFFFFFFFfFFFFFFFFF",  "-9223372036854775808", "%jd",
                   intmax);
    DO_TEST_NUMSTR("-0xFFFFFFFFFFFFFFFF",     "-9223372036854775808", "%jd",
                   intmax);
  }
  
  DO_TEST_NUMSTR("0xFFfFFF", "0XFFFF", "%#hX", ushort);
  DO_TEST_NUMSTR("0xFFff",   "0XFFFF", "%#hX", ushort);
  DO_TEST_NUMSTR("0xffFF" ,  "0x7fff", "%#hx", short);
  DO_TEST_NUMSTR("0xFFFF",   "0X7FFF", "%#hX", short);
  DO_TEST_NUMSTR("-0xFFfFF", "-32768", "%hd", short);
  DO_TEST_NUMSTR("-0xFfFF",  "-32768", "%hd", short);
  
  DO_TEST_NUMSTR("0xFFfFFFfFFFFF",  "0XFFFFFFFF", "%#X", uint);
  DO_TEST_NUMSTR("0xFFFFffff",      "0XFFFFFFFF", "%#X", uint);
  DO_TEST_NUMSTR("0xffffFFFF",      "0x7fffffff", "%#x", int);
  DO_TEST_NUMSTR("0xFFFFFFFF",      "0X7FFFFFFF", "%#X", int);
  DO_TEST_NUMSTR("-0xFFFFFfFFFFF",  "-2147483648", "%d", int);
  DO_TEST_NUMSTR("-0xFFFFFFFF",     "-2147483648", "%d", int);
  
  return (0);
}

/* tst_coverage
 *
 * VSTR_FLAG_PARSE_NUM_DEF
 *
 */
