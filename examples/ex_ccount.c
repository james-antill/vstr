/* this program does a character count of it's input */

#include "ex_utils.h"
#include "ctype.h"

static int ex_ccount_process(Vstr_base *s1, Vstr_base *s2, int last)
{
  static size_t prev_len = 0;
  size_t len = 0;
  int ret = FALSE;

  while (s2->len)
  {
    char chrs[1];
    int did_all = FALSE;
    
    chrs[0] = vstr_export_chr(s2, 1);
    len = vstr_spn_chrs_fwd(s2, 1, s2->len, chrs, 1);

    ret = TRUE;

    did_all = (len == s2->len);
    vstr_del(s2, 1, len);
    
    if (did_all && !last)
    {
      prev_len += len;
      break;
    }

    if (isprint((unsigned char)chrs[0]))
      vstr_add_fmt(s1, s1->len, " '%c' [%#04x] * %zu\n", chrs[0], chrs[0],
                   prev_len + len);
    else
      vstr_add_fmt(s1, s1->len, " '?' [%#04x] * %zu\n", chrs[0],
                   prev_len + len);
    prev_len = 0;
  }

  return (ret);
}

static void ex_ccount_process_limit(Vstr_base *s1, Vstr_base *s2,
                                    unsigned int lim)
{
  while (s2->len > lim)
  { /* Finish processing read data (try writing if we need memory) */
    int proc_data = ex_ccount_process(s1, s2, !lim);
 
    if (!proc_data && (io_put(s1, STDOUT_FILENO) == IO_BLOCK))
      io_block(-1, STDOUT_FILENO);
  }
}

static void ex_ccount_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2, int fd)
{
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s2, fd);
                                                                                
    if (io_r_state == IO_EOF)
      break;

    ex_ccount_process(s1, s2, FALSE);
    
    io_w_state = io_put(s1, STDOUT_FILENO);
                                                                                
    io_limit(io_r_state, fd, io_w_state, STDOUT_FILENO, s1);
  }

  ex_ccount_process_limit(s1, s2, 0);
}
                                                                                
int main(int argc, char *argv[])
{
  Vstr_base *s2 = NULL;
  Vstr_base *s1 = ex_init(&s2); /* init the library, and create two strings */
  int count = 1; /* skip the program name */

  /* if no arguments are given just do stdin to stdout */
  if (count >= argc)
  {
    io_fd_set_o_nonblock(STDIN_FILENO);
    ex_ccount_read_fd_write_stdout(s1, s2, STDIN_FILENO);
  }

  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    int fd = io_open(argv[count]);
                                                                                
    ex_ccount_read_fd_write_stdout(s1, s2, fd);

    if (close(fd) == -1)
      warn("close(%s)", argv[count]);

    ++count;
  }

  /* output all remaining data */
  io_put_all(s1, STDOUT_FILENO);

  exit (ex_exit(s1, NULL));
}
