/* hello world with the Vstr string API */

#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>
#include <errno.h>

int main(void)
{
  Vstr_base *s1 = NULL;

  if (!vstr_init())
    exit (EXIT_FAILURE);

  s1 = vstr_dup_cstr_buf(NULL, "Hello World\n");
  if (!s1)
    exit (EXIT_FAILURE);

  while (s1->len) /* assumes POSIX */
    if (!vstr_sc_write_fd(s1, 1, s1->len, 1, NULL))
    {
      if ((errno != EAGAIN) && (errno != EINTR))
        exit (EXIT_FAILURE);
    }

  vstr_free_base(s1);

  vstr_exit();

  exit (EXIT_SUCCESS);
}
