/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>

#include "ex_utils.h"

int main(void)
{
  Vstr_base *str1 = NULL;
  
  if (!vstr_init())
      exit (EXIT_FAILURE);
  
  str1 = VSTR_DUP_CSTR_BUF(NULL, "abcd xyz\n");
  if (!str1)
    exit (EXIT_FAILURE);

  VSTR_ADD_CSTR_PTR(str1, str1->len, "abcd xyz\n");
  
  vstr_conv_encode_uri(str1, 1, str1->len);

  VSTR_ADD_CSTR_BUF(str1, str1->len, "\n");
  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("first:");  

  while (str1->len)
    ex_utils_write(str1, 1);

  VSTR_ADD_CSTR_BUF(str1, str1->len, "abcd%20xyz%0A");
  VSTR_ADD_CSTR_PTR(str1, str1->len, "abcd%20xyz%0A");

  vstr_conv_decode_uri(str1, 1, str1->len);

  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("second:");  

  while (str1->len)
    ex_utils_write(str1, 1);

  vstr_free_base(str1);
  
  exit (EXIT_SUCCESS);
}
