
#define EX_UTILS_NO_USE_OPEN 1
#include "ex_utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int ex_html_dir_list_process(Vstr_base *s1, Vstr_base *s2,
                                    int *parsed_header)
{
  size_t pos = 0;
  size_t len = 0;
  size_t ns1 = vstr_parse_netstr(s2, 1, s2->len, &pos, &len);
  int done = FALSE;
  
  if (!ns1)
  {
    if ((len     > EX_MAX_R_DATA_INCORE) || 
        (s2->len > EX_MAX_R_DATA_INCORE))
      errx(EXIT_FAILURE, "bad input");
  
    return (FALSE);
  }

  if (!*parsed_header)
  {
    size_t vpos = 0;
    size_t vlen = 0;
    size_t nst  = 0;
    
    if (!(nst = vstr_parse_netstr(s2, pos, len, &vpos, &vlen)))
      errx(EXIT_FAILURE, "bad input");
    pos += nst; len -= nst;
    if (!vstr_cmp_cstr_eq(s2, vpos, vlen, "version"))
      errx(EXIT_FAILURE, "bad input");
    if (!(nst = vstr_parse_netstr(s2, pos, len, &vpos, &vlen)))
      errx(EXIT_FAILURE, "bad input");
    pos += nst; len -= nst;
    if (!vstr_cmp_cstr_eq(s2, vpos, vlen, "1"))
      errx(EXIT_FAILURE, "Unsupported version");
    *parsed_header = TRUE;
    len = 0;
  }
  
  while (len)
  {
    size_t kpos = 0;
    size_t klen = 0;
    size_t vpos = 0;
    size_t vlen = 0;
    size_t nst  = 0;

    if (!(nst = vstr_parse_netstr(s2, pos, len, &kpos, &klen)))
      errx(EXIT_FAILURE, "bad input");
    pos += nst; len -= nst;
    
    if (!(nst = vstr_parse_netstr(s2, pos, len, &vpos, &vlen)))
      errx(EXIT_FAILURE, "bad input");    
    pos += nst; len -= nst;

    if (0) { }
    else if (vstr_cmp_cstr_eq(s2, kpos, klen, "name"))
    { /* FIXME: need to urlize it */
      vstr_add_fmt(s1, s1->len,
                   "%s<li>"
                   "<a href=\"${vstr:%p%zu%zu%u}\">${vstr:%p%zu%zu%u}</a>",
                   done ? "</li>\n" : "",
                   s2, vpos, vlen, 0,
                   s2, vpos, vlen, 0);
      done = TRUE;
    }
    else if (vstr_cmp_cstr_eq(s2, kpos, klen, "type"))
    {
      VSTR_AUTOCONF_uintmax_t val = vstr_parse_uintmax(s2, vpos, vlen, 10,
                                                       NULL, NULL);

      if (0) { }
      else if (S_ISREG(val))
      { /* do nothing */ }
      else if (S_ISDIR(val))
        vstr_add_cstr_buf(s1, s1->len, " (directory)");
      else if (S_ISCHR(val))
        vstr_add_cstr_buf(s1, s1->len, " (character device)");
      else if (S_ISBLK(val))
        vstr_add_cstr_buf(s1, s1->len, " (block device)");
      else if (S_ISLNK(val))
        vstr_add_cstr_buf(s1, s1->len, " (symbolic link)");
      else if (S_ISSOCK(val))
        vstr_add_cstr_buf(s1, s1->len, " (socket)");

      done = TRUE;
    }
    else if (vstr_cmp_cstr_eq(s2, kpos, klen, "size"))
    {
      VSTR_AUTOCONF_uintmax_t val = vstr_parse_uintmax(s2, vpos, vlen, 10,
                                                       NULL, NULL);
      vstr_add_fmt(s1, s1->len, " ${BKMG.ju:%ju}", val);
    }
  }

  if (done)
    vstr_add_cstr_buf(s1, s1->len, "</li>\n");

  vstr_del(s2, 1, ns1);

  return (TRUE);
}

static void ex_html_dir_list_process_limit(Vstr_base *s1, Vstr_base *s2,
                                           int *parsed_header)
{
  while (s2->len)
  { /* Finish processing read data (try writing if we need memory) */
    int proc_data = ex_html_dir_list_process(s1, s2, parsed_header);

    if (!proc_data && (io_put(s1, STDOUT_FILENO) == IO_BLOCK))
      io_block(-1, STDOUT_FILENO);
  }
}

static void ex_html_dir_list_beg(Vstr_base *s1, const char *fname)
{
  vstr_add_fmt(s1, s1->len, "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n\
<html>\n\
 <head>\n\
  <title>Directory listing of %s</title>\n\
 </head>\n\
 <body>\n\
  <h1>Directory listing of %s</h1>\n\
  <ul>\n\
", fname, fname);
}

static void ex_html_dir_list_end(Vstr_base *s1)
{
  vstr_add_cstr_buf(s1, s1->len, "\n\
  </ul>\n\
 </body>\n\
</html>\n\
");
}

static void ex_html_dir_list_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2,
                                                  int fd)
{
  int parsed_header[1] = {FALSE};
  
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s2, fd);

    if (io_r_state == IO_EOF)
      break;

    ex_html_dir_list_process(s1, s2, parsed_header);
    
    io_w_state = io_put(s1, STDOUT_FILENO);

    io_limit(io_r_state, fd, io_w_state, STDOUT_FILENO, s1);    
  }
  
  ex_html_dir_list_process_limit(s1, s2, parsed_header);
}

static int ex_html_dir_list_spawn_dir_list(const char *arg)
{ /* FIXME: should allow spawning of arbitrary apps. to generate dir list */
  abort();
  return -1;
}

/* This is "dir_list", without any command line options */
int main(int argc, char *argv[])
{
  Vstr_base *s1 = NULL;
  Vstr_base *s2 = ex_init(&s1); /* init the library etc. */
  int count = 1; /* skip the program name */
  const char *def_name = "&lt;stdin&gt;";
  
  if (!vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$') ||
      !vstr_sc_fmt_add_all(s1->conf))
    errno = ENOMEM, err(EXIT_FAILURE, "init");
    
  /* parse command line arguments... */
  while (count < argc)
  { /* quick hack getopt_long */
    if (!strcmp("--", argv[count]))
    {
      ++count;
      break;
    }
    EX_UTILS_GETOPT_CSTR("name", def_name);
    else if (!strcmp("--version", argv[count]))
    { /* print version and exit */
      vstr_add_fmt(s1, 0, "%s", "\
jhtml_dir_list 1.0.0\n\
Written by James Antill\n\
\n\
Uses Vstr string library.\n\
");
      goto out;
    }
    else if (!strcmp("--help", argv[count]))
    { /* print version and exit */
      vstr_add_fmt(s1, 0, "%s", "\
Usage: jhtml_dir_list [FILENAME]...\n\
   or: jhtml_dir_list OPTION\n\
Output filenames.\n\
\n\
      --help     Display this help and exit\n\
      --version  Output version information and exit\n\
      --name     Name to be used if input from stdin\n\
      --         Treat rest of cmd line as input filenames\n\
\n\
Report bugs to James Antill <james@and.org>.\n\
");
      goto out;
    }
    else
      break;
    ++count;
  }
  
  /* if no arguments are given just do stdin to stdout */
  if (count >= argc)
  {
    io_fd_set_o_nonblock(STDIN_FILENO);
    ex_html_dir_list_beg(s1, def_name);
    ex_html_dir_list_read_fd_write_stdout(s1, s2, STDIN_FILENO);
    ex_html_dir_list_end(s1);
  }
  
  /* loop through all arguments, open the dir specified
   * and do the read/write loop */
  while (count < argc)
  {
    int fd = ex_html_dir_list_spawn_dir_list(argv[count]);

    ex_html_dir_list_beg(s1, argv[count]);
    ex_html_dir_list_read_fd_write_stdout(s1, s2, fd);
    ex_html_dir_list_end(s1);

    if (close(fd) == -1)
      warn("close(%s)", argv[count]);

    ++count;
  }

  /* output all remaining data */
 out:
  io_put_all(s1, STDOUT_FILENO);

  exit (ex_exit(s1, s2));
}
