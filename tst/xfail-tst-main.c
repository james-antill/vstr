#include "tst-main.c"

static void xfail_tst(void); /* fwd */

int tst(void)
{
#ifdef HAVE_POSIX_HOST
  int fd = -1;
                
  if ((fd = open("/dev/null", O_WRONLY)) == -1) return (EXIT_SUCCESS);
  if (dup2(fd, 1) == -1)                        return (EXIT_SUCCESS);
  if (dup2(fd, 2) == -1)                        return (EXIT_SUCCESS);
#endif
  
  xfail_tst();
  
  return (EXIT_SUCCESS);
}
