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
  
  str1 = vstr_make_base(NULL);
  if (!str1)
    exit (EXIT_FAILURE);

  vstr_add_fmt(str1, str1->len, "Hello %s, World is %d.\n", "vstr", 1);

  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("vstr_add_fmt:");
  
  ex_utils_cpy_write_all(str1, 1);

  vstr_del(str1, 1, str1->len);

  if (!vstr_cntl_conf(str1->conf,
                      VSTR_CNTL_CONF_SET_LOC_CSTR_NAME_NUMERIC, "custom"))
    errno = ENOMEM, DIE("vstr_cntl_conf(LOC_CSTR_NAME_NUMERIC):");
  if (!vstr_cntl_conf(str1->conf, VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_SEP, "_."))
    errno = ENOMEM, DIE("vstr_cntl_conf(LOC_CSTR_THOU_SEP):");
  if (!vstr_cntl_conf(str1->conf, VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_GRP, "\3"))
    errno = ENOMEM, DIE("vstr_cntl_conf(LOC_CSTR_THOU_GRP):");
  
  vstr_add_fmt(str1, str1->len, "10_.000_.000=%'20.16u.\n", (10 * 1000 * 1000));

  while (str1->len)
    ex_utils_write(str1, 1);
  
  vstr_free_base(str1);
  
  exit (EXIT_SUCCESS);
}
