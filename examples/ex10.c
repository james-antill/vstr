/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <limits.h>
#include <errno.h>

#include "ex_utils.h"

const unsigned int FLAGS = (0 |
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
    errno = ENOMEM, DIE("vstr_dup_ptr:");

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
    errno = ENOMEM, DIE("vstr_dup_ptr:");

  ret = vstr_parse_uint(str2, 1, str2->len, FLAGS, len, err);

  vstr_free_base(str2);

  return (ret);
}

#define DO_TEST_SINTSTR(x, y, z) do { \
  size_t num_len = 0; \
  unsigned int err = 0; \
  vstr_add_fmt(str1, str1->len, y " = " z, \
               do_test_int(x, strlen(x), &num_len, &err)); \
  vstr_add_fmt(str1, str1->len, " (len/ret = %zu/%zu) (err = %u)\n", \
               strlen(x), num_len, err); \
} while (FALSE)

#define DO_TEST_UINTSTR(x, y, z) do { \
  size_t num_len = 0; \
  unsigned int err = 0; \
  vstr_add_fmt(str1, str1->len, y " = " z, \
               do_test_uint(x, strlen(x), &num_len, &err)); \
  vstr_add_fmt(str1, str1->len, " (len/ret = %zu/%zu) (err = %u)\n", \
               strlen(x), num_len, err); \
} while (FALSE)

#define DO_TEST_NUM(x, y, z) do { \
  size_t num_len = 0; \
  unsigned int err = 0; \
  vstr_del(str2, 1, str2->len); \
  vstr_add_fmt(str2, str2->len, y, x); \
  vstr_add_fmt(str1, str1->len, y " = " y, x, \
               vstr_parse_ ## z (str2, 1, str2->len, FLAGS, &num_len, &err)); \
  vstr_add_fmt(str1, str1->len, " (len/ret = %zu/%zu) (err = %u)\n", \
               str2->len, num_len, err); \
} while (FALSE)

int main(void)
{
  Vstr_base *str1 = NULL;
  Vstr_base *str2 = NULL;
  
  if (!vstr_init())
    exit (EXIT_FAILURE);
  
  str1 = vstr_make_base(NULL);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");

  str2 = vstr_make_base(NULL);
  if (!str2)
    errno = ENOMEM, DIE("vstr_make_base:");

  DO_TEST_SINTSTR("1234", "1234", "%d");
  DO_TEST_SINTSTR("-1234", "-1234", "%d");
  DO_TEST_SINTSTR("+1234", "1234", "%d");
  DO_TEST_UINTSTR("  0x1234", "0x1234", "%#x");
  DO_TEST_SINTSTR("1_234", "1234", "%d");
  DO_TEST_UINTSTR("0x1__23__4", "0x1234", "%#x");
  DO_TEST_UINTSTR("   0x00001__23__4", "0x1234", "%#x");
  DO_TEST_UINTSTR("012_34", "01234", "%#o");
  DO_TEST_UINTSTR("000012_34", "01234", "%#o");

  DO_TEST_NUM(INT_MAX, "%d", int);
  DO_TEST_NUM(INT_MIN, "%d", int);
  DO_TEST_NUM(UINT_MAX, "%u", uint);
  DO_TEST_NUM(UINT_MAX, "%#x", uint);
  DO_TEST_NUM(UINT_MAX, "%#o", uint);
  DO_TEST_NUM(UINTMAX_MAX, "%ju", uintmax);
  
  DO_TEST_UINTSTR("0xFFFFFFFFFFFF", "0xFFFFFFFF", "%#x");
  DO_TEST_SINTSTR("0xFFFFFFFF", "0x7FFFFFFF", "%#x");
  DO_TEST_SINTSTR("-0xFFFFFFFF", "-2147483648", "%d");
  DO_TEST_SINTSTR("0xFFFFFFFFFFFF", "0x7FFFFFFF", "%#x");
  DO_TEST_SINTSTR("-0xFFFFFFFFFFFF", "-2147483647", "%d");
  
  while (str1->len)
    ex_utils_write(str1, 1);
  
  vstr_free_base(str1);
  vstr_free_base(str2);

  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
