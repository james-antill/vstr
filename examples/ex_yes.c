/* Unix "yes" command
 */

#define VSTR_COMPILE_INCLUDE 1 /* make Vstr include it's own system headers */
#include <vstr.h>

#include <sys/types.h> /* stat + open */
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>

#include <limits.h>

#define TRUE 1
#define FALSE 0

#define OUT_DEF_NUM 2 /* how many lines to add at once, by default */

#define OUT_LIM_DEF_NUM 0 /* how many lines to do in total, by default */

#define OUT_PTR 1 /* copy ptr's to data */
#define OUT_BUF 2 /* copy data to */
#define OUT_REF 3 /* copy data once, to coalese */

#define MAX_W_DATA_INCORE (1024 * 128)

static unsigned int sz = MAX_W_DATA_INCORE;

/* Creates a Vstr string, prints an error message to it and dies
 * don't really need to bother free'ing things as it's only called on a
 * major error -- but it's a good habit */
static void DIE(const char *msg, ...)
{
  Vstr_base *err = vstr_make_base(NULL);

  if (err)
  {
    va_list ap;
    
    va_start(ap, msg);
    vstr_add_vsysfmt(err, 0, msg, ap);
    va_end(ap);

    while (err->len)
      if (!vstr_sc_write_fd(err, 1, err->len, 1, NULL))
      {
        if ((errno != EAGAIN) && (errno != EINTR))
          break; /* don't recurse */
      }
  }
  vstr_free_base(err);

  exit (EXIT_FAILURE);
}

#define ASSERT(x) if (x) {} else DIE("Assert: %s\n", #x )

#define GETOPT_NUM(name, var) \
    else if (!strncmp("--" name, argv[count], strlen("--" name))) \
    { \
      if (!strncmp("--" name "=", argv[count], strlen("--" name "="))) \
        (var) = strtol(argv[count] + strlen("--" name "="), NULL, 0); \
      else \
      { \
        (var) = 0; \
        \
        ++count; \
        if (count >= argc) \
          break; \
        \
        (var) = strtol(argv[count], NULL, 0); \
      } \
    } \
    else if (0) ASSERT(FALSE)

#define GETOPT_CSTR(name, var) \
    else if (!strncmp("--" name, argv[count], strlen("--" name))) \
    { \
      if (!strncmp("--" name "=", argv[count], strlen("--" name "="))) \
        (var) = argv[count] + strlen("--" name "="); \
      else \
      { \
        (var) = NULL; \
        \
        ++count; \
        if (count >= argc) \
          break; \
        \
        (var) = argv[count]; \
      } \
    } \
    else if (0) ASSERT(FALSE)

int main(int argc, char *argv[])
{ /* This is "yes", without any command line options */
  Vstr_base *s1 = NULL;
  Vstr_base *cmd_line = NULL;
  int count = 1; /* skip program name */
  struct stat stat_buf;
  unsigned int out_type = OUT_PTR;
  unsigned int out_num = OUT_DEF_NUM;
  unsigned int lim_num = OUT_LIM_DEF_NUM;
  const char *out_filename = NULL;
  int out_fd = -1;
  
  /* init the Vstr string library, note that if this fails we can't call DIE
   * or the program will crash */
  if (!vstr_init())
    exit (EXIT_FAILURE);

  /* Alter the node buffer size to be whatever the stdout block size is */
  if (fstat(1, &stat_buf) == -1)
    DIE("fstat(STDOUT): %m\n");

  if (!stat_buf.st_blksize) /* or 4k if there is no block size... */
    stat_buf.st_blksize = 4 * 1024;
  
  vstr_cntl_conf(NULL, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);
  
  /* create a Vstr string to hold the command line string */  
  cmd_line = vstr_make_base(NULL);
  if (!cmd_line)
    errno = ENOMEM, DIE("vstr_make_base: %m\n");

  /* create a Vstr string for doing the IO with */  
  s1 = vstr_make_base(NULL);
  if (!s1)
    errno = ENOMEM, DIE("vstr_make_base: %m\n");

  while (count < argc)
  { /* quick hack getopt_long */
    if (!strcmp("--", argv[count]))
    {
      ++count;
      break;
    }
    else if (!strcmp("--ptr", argv[count]))
      out_type = OUT_PTR;
    else if (!strcmp("--buf", argv[count]))
      out_type = OUT_BUF;
    else if (!strcmp("--ref", argv[count]))
      out_type = OUT_REF;
    GETOPT_NUM("sz",    sz);
    GETOPT_NUM("num",   out_num);
    GETOPT_NUM("limit", lim_num);
    GETOPT_NUM("lim",   lim_num);
    GETOPT_CSTR("file",  out_filename);
    else if (!strcmp("--version", argv[count]))
    {
      vstr_add_fmt(s1, 0, "%s", "\
yes 1.0.0\n\
Written by James Antill\n\
\n\
Uses Vstr string library.\n\
");
      goto out;
    }
    else if (!strcmp("--help", argv[count]))
    {
      vstr_add_fmt(s1, 0, "%s", "\
Usage: yes [STRING]...\n\
   or: yes OPTION\n\
Repeatedly output a line with all specified STRING(s), or `y'.\n\
\n\
      --help     display this help and exit\n\
      --version  output version information and exit\n\
      --ptr      output using pointers to data\n\
      --buf      output using copies of data\n\
      --ref      output using single copy of data, with references\n\
      --num      number of times to add the line, between output calls\n\
      --sz       output maximum size, lower bound\n\
      --         Use rest of cmd line input regardless of if it's an option\n\
\n\
Report bugs to James Antill <james@and.org>.\n\
");
      goto out;
    }
    else
      break;
    ++count;
  }
  
  if (count >= argc)
    VSTR_ADD_CSTR_PTR(cmd_line, cmd_line->len, "y");
  
  {
    int added = FALSE;

    while (count < argc)
    {
      if (added) /* already done one argument */
        VSTR_ADD_CSTR_PTR(cmd_line, cmd_line->len, " ");
      VSTR_ADD_CSTR_PTR(cmd_line, cmd_line->len, argv[count]);
      
      ++count;
      added = TRUE;
    }
  }

  VSTR_ADD_CSTR_PTR(cmd_line, cmd_line->len, "\n");

  if (0)
  { ASSERT(FALSE); }
  else if (out_type == OUT_PTR)
  { /* do nothing */ }
  else if (out_type == OUT_BUF)
  {
    size_t len = cmd_line->len;
    vstr_add_vstr(cmd_line, len, cmd_line, 1, len, VSTR_TYPE_ADD_ALL_BUF);
    vstr_del(cmd_line, 1, len);
  }
  else if (out_type == OUT_REF)
  {
    size_t len = cmd_line->len;
    vstr_add_vstr(cmd_line, len, cmd_line, 1, len, VSTR_TYPE_ADD_ALL_REF);
    vstr_del(cmd_line, 1, len);
  }
  else
    DIE("INTERNAL ERROR ... bad out_type");
  
  if (cmd_line->conf->malloc_bad)
    errno = ENOMEM, DIE("Creating output string: %m\n");    

  if (!out_num)
    ++out_num;

  if (!out_filename)
    out_fd = 1;
  else
  {
    out_fd = open(out_filename, O_WRONLY | O_TRUNC | O_LARGEFILE | O_NOCTTY);
    
    if (out_fd == -1)
      DIE("open(%s): %m\n", out_filename);
  }
  
  /* BEG: main loop */
  
  do
  {
    count = out_num;
    
    while ((s1->len < sz) && (--count >= 0))
      vstr_add_vstr(s1, s1->len, cmd_line, 1, cmd_line->len, VSTR_TYPE_ADD_DEF);
    
    if (!vstr_sc_write_fd(s1, 1, s1->len, out_fd, NULL))
    {
      if ((errno != EAGAIN) && (errno != EINTR))
        DIE("write: %m\n");
    }

    /* 0 == infinity */
    if    (lim_num == 1) break;
    if    (lim_num >  1) --lim_num;
  } while (TRUE);
  
 out:
  while (s1->len)
    if (!vstr_sc_write_fd(s1, 1, s1->len, out_fd, NULL))
    {
      if ((errno != EAGAIN) && (errno != EINTR))
        DIE("write: %m\n");
    }

  /* END: main loop */

  /* These next three calls are only really needed to make valgrind happy,
   * but that does help in that any memory leaks are easier to see.
   */
  
  /* free s1 and cmd_line strings */
  vstr_free_base(cmd_line);
  vstr_free_base(s1);
  
  /* "exit" Vstr, this free's all internal data and no library calls apart from
   * vstr_init() should be called after this.
   */
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
