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
  unsigned int count = 0;

  if (!vstr_init())
      exit (EXIT_FAILURE);
  
  str1 = vstr_make_base(NULL);
  if (!str1)
    exit (EXIT_FAILURE);

  while (count < (10 * 1000 * 1000))
  {
  	vstr_add_buf(str1, str1->len, "vs\n", 2);
	++count;
	if (str1->len > 1000)
		vstr_del(str1, 1, str1->len);
  }

  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("vstr_add_fmt:");
  
  vstr_free_base(str1);
  
  exit (EXIT_SUCCESS);
}
