/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>

#include "ex_utils.h"

const unsigned int FLAGS = (VSTR_FLAG_SPLIT_BEG_NULL | 
                            VSTR_FLAG_SPLIT_MID_NULL | 
                            VSTR_FLAG_SPLIT_END_NULL | 
                            VSTR_FLAG_SPLIT_NO_RET
                            );

#define ALLOC_LIM 8
#define PASS_LIM 8

static unsigned int foreach_func(const Vstr_base *base, size_t pos, size_t len,
                                 void *data)
{
  static unsigned int count = 0;
  Vstr_base *str1 = data;

  if (!pos)
    return (0);

  ++count;
  
  if (count == PASS_LIM) 
    vstr_add_fmt(str1, str1->len, "Rest %s = ::", "   ");
  else
    vstr_add_fmt(str1, str1->len, "Ent %4u = ::", count);
  
  if (!len)
    vstr_add_fmt(str1, str1->len, "%s", ".");
  else
    vstr_add_vstr(str1, str1->len, base, pos, len, 0);
  
  vstr_add_fmt(str1, str1->len, ":: len(%u)\n", len);

  return (0);
}

int main(void)
{
  Vstr_base *str1 = NULL;
  Vstr_base *str2 = NULL;
  const char *path = getenv("PATH");
  VSTR_SECTS_DECL(sects, ALLOC_LIM);
  
  VSTR_SECTS_DECL_INIT(sects);
    
  if (!vstr_init())
    exit (EXIT_FAILURE);
  
  str1 = vstr_make_base(NULL);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");

  str2 = vstr_dup_ptr(NULL, path, strlen(path));
  if (!str2)
    errno = ENOMEM, DIE("vstr_dup_ptr:");

  vstr_split_buf(str2, 1, str2->len, ":", 1, sects, PASS_LIM, FLAGS);

  vstr_sects_foreach(str2, sects, 0, foreach_func, str1);

  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("vstr_add_fmt:");
  
  while (str1->len)
    ex_utils_write(str1, 1);
  
  vstr_free_base(str1);
  vstr_free_base(str2);
  
  exit (EXIT_SUCCESS);
}
