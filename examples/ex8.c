/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <errno.h>

#include "ex_utils.h"

int main(void)
{
  Vstr_base *str1 = NULL;
  
  if (!vstr_init())
    exit (EXIT_FAILURE);
  
  str1 = vstr_make_base(NULL);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");

  vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);
  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("vstr_add_fmt:");
  
  while (str1->len)
    ex_utils_write(str1, 1);
  
  vstr_free_base(str1);
  
  vstr_exit();
 
  exit (EXIT_SUCCESS);
}
