/* quick and qirty static ssi processor. Only does ...

<!--#include file="foo" -->

*/
#include "ex_utils.h"

#include <sys/time.h>
#include <time.h>

static char  *dname_ptr = NULL;
static size_t dname_len = 0;

static void ex_ssi_cat_read_fd_write_stdout(Vstr_base *s1, int fd)
{
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s1, fd);
 
    if (io_r_state == IO_EOF)
      break;
                                                                                
    io_w_state = io_put(s1, 1);
                                                                                
    io_limit(io_r_state, fd, io_w_state, 1, s1);
  }
}


static int ex_ssi_process(Vstr_base *s1, Vstr_base *s2, int last)
{
  size_t srch = 0;
  int ret = FALSE;
  
  /* we don't want to create more data, if we are over our limit */
  if (s1->len > EX_MAX_W_DATA_INCORE)
    return (FALSE);
  
  while (s2->len >= strlen("<!--#"))
  {
    if (!(srch = vstr_srch_cstr_buf_fwd(s2, 1, s2->len, "<!--#")))
    {
      ret = TRUE;
      vstr_mov(s1, s1->len, s2, 1, s2->len);
      break;
    }
    
    if (srch > 1)
    {
      ret = TRUE;
      vstr_add_vstr(s1, s1->len, s2, 1, srch - 1, VSTR_TYPE_ADD_BUF_REF);
      vstr_del(s2, 1, srch - 1);
    }

    if (!(srch = vstr_srch_cstr_buf_fwd(s2, 1, s2->len, "\" -->")))
      break;
    srch += strlen(" -->"); /* point to the end */
    
    ret = TRUE;


    if (0) { }
    else if (vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "<!--#include"))
    {
      int fd = -1;
      size_t tmp = 0;

      tmp = strlen("<!--#include");
      vstr_del(s2, 1, tmp);
      srch         -= tmp;
      
      tmp = vstr_spn_cstr_chrs_fwd(s2, 1, srch, " ");
      vstr_del(s2, 1, tmp);
      srch         -= tmp;

      if (!vstr_cmp_case_bod_cstr_eq(s2, 1, srch, "file=\""))
      {
        vstr_add_cstr_ptr(s1, s1->len,
                          "<!-- \"include\" SSI command FAILED -->\n");
        break;
      }
      
      tmp = strlen("file=\"");
      vstr_del(s2, 1, tmp);
      srch         -= tmp;

      if (!(tmp = vstr_cspn_cstr_chrs_fwd(s2, 1, srch, "\"")))
      {
        vstr_add_cstr_ptr(s1, s1->len,
                          "<!-- \"include \" SSI command FAILED -->\n");
        break;
      }        

      vstr_add_fmt(s1, s1->len, "<!-- \"include %s\" SSI command OK -->\n",
                   vstr_export_cstr_ptr(s2, 1, tmp));
      
      vstr_add_buf(s2, 0, dname_ptr, dname_len);
      tmp                         += dname_len;

      if (s1->conf->malloc_bad)
        errno = ENOMEM, err(EXIT_FAILURE, "add data");

      fd = io_open(vstr_export_cstr_ptr(s2, 1, tmp));

      ex_ssi_cat_read_fd_write_stdout(s1, fd);

      if (close(fd) == -1)
        warn("close(%s)", vstr_export_cstr_ptr(s2, 1, tmp));
      
      vstr_del(s2, 1, dname_len);
    }
    else if (0 && vstr_cmp_case_bod_cstr_eq(s2, 1, s2->len, "<!--#exec"))
    {
      vstr_add_cstr_ptr(s1, s1->len, "<!-- \"exec\" SSI command -->\n");
    }
    else
      vstr_add_cstr_ptr(s1, s1->len, "<!-- UNKNOWN SSI command -->\n");
    
    vstr_del(s2, 1, srch);
  }

  if (last)
  {
    ret = TRUE;
    vstr_mov(s1, s1->len, s2, 1, s2->len);
  }
  
  return (ret);  
}


static void ex_ssi_process_limit(Vstr_base *s1, Vstr_base *s2, unsigned int lim)
{
  while (s2->len > lim)
  {
    int proc_data = ex_ssi_process(s1, s2, !lim);
    if (!proc_data && (io_put(s1, STDOUT_FILENO) == IO_BLOCK))
      io_block(-1, STDOUT_FILENO);
  }
}

static void ex_ssi_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2, int fd)
{
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s2, fd);

    if (io_r_state == IO_EOF)
      break;

    ex_ssi_process(s1, s2, FALSE);
    
    io_w_state = io_put(s1, 1);

    io_limit(io_r_state, fd, io_w_state, 1, s1);    
  }

  ex_ssi_process_limit(s1, s2, 0);
}

int main(int argc, char *argv[])
{
  Vstr_base *s2 = NULL;
  Vstr_base *s1 = ex_init(&s2);
  int count = 1;
  time_t now = time(NULL);
  
  if (!(dname_ptr = strdup("")))
    errno = ENOMEM, err(EXIT_FAILURE, "add data");
    
  if (count >= argc)
  {
    io_fd_set_o_nonblock(STDIN_FILENO);
    ex_ssi_read_fd_write_stdout(s1, s2, STDIN_FILENO);
    vstr_add_fmt(s1, s1->len,
                 "<!-- SSI processing of %s -->\n"
                 "<!--   done on %s -->\n"
                 "<!--   done by jssi -->\n",
                 "stdin", ctime(&now));
  }
  
  while (count < argc)
  {
    int fd = io_open(argv[count]);
    size_t len = strlen(argv[count]);
    size_t dbeg = 0;
    
    if (!vstr_add_buf(s1, s1->len, argv[count], len))
      errno = ENOMEM, err(EXIT_FAILURE, "add data");

    dbeg = s1->len - len + 1;
    vstr_sc_dirname(s1, dbeg, len, &dname_len);
    if (dname_len)
    {
      free(dname_ptr);
      if (!(dname_ptr = vstr_export_cstr_malloc(s1, dbeg, dname_len)))
        errno = ENOMEM, err(EXIT_FAILURE, "add data");
    }
    vstr_del(s1, s1->len - len + 1, len);
    
    ex_ssi_read_fd_write_stdout(s1, s2, fd);

    if (close(fd) == -1)
      warn("close(%s)", argv[count]);

    vstr_add_fmt(s1, s1->len,
                 "<!-- SSI processing of %s -->\n"
                 "<!--   done on %s -->\n"
                 "<!--   done by jssi -->\n",
                 argv[count], ctime(&now));
    
    ++count;
  }

  free(dname_ptr);
  
  io_put_all(s1, STDOUT_FILENO);

  exit (ex_exit(s1, s2));
}
