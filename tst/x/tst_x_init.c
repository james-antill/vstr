
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

int main(void)
{
#ifdef VSTR_AUTOCONF_NDEBUG /* malloc fail doesn't work */
  exit (EXIT_SUCCESS);
#endif
  vstr_cntl_opt(666, 1);

  if (vstr_init()) /* should fail */
    exit (EXIT_FAILURE);
  
  exit (EXIT_SUCCESS);  
}
