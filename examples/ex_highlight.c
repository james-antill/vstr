/* quick and dirty copy of the
 * highlight (http://freshmeat.net/projects/highlight/) command */
#define VSTR_COMPILE_INLINE 0

#include "ex_utils.h"

#include <sys/time.h>
#include <time.h>

static struct 
{
 const char *name;
 const char *seq;
 size_t off;
 size_t eoff;
 size_t len;
} ex_hl_seqs[] = 
/* This assumes a certain style...
   
   for instance neither of...

   if(foo)
   foo;if (bar)

   ...will be seen as an if statement, as there is no space before and after
   the if. This is a feature, as the style should be the same.
*/
#define EX_HL_SEQ_T(x, y) \
   x, " "  y " ", 1, 1, 0 }, \
 { x, "("  y " ", 1, 1, 0 }, \
 { x, "*"  y " ", 1, 1, 0 }, \
 { x, "\n" y " ", 1, 1, 0
#define EX_HL_SEQ_WS(x, y) \
   x, " " y " ", 1, 1, 0
/* custom for default */
#define EX_HL_SEQ_DEF(x, y) \
   x, " " y ":\n", 1, 2, 0
#define EX_HL_SEQ_SB(x, y) \
   x, " " y " (", 1, 2, 0
#define EX_HL_SEQ_B(x, y)  \
   x, " " y "(", 1, 1, 0
#define EX_HL_SEQ_CPP(x, y) \
   x, "#"     y " ",  1, 1, 0 }, \
 { x, "#"     y "\n", 1, 1, 0 }, \
 { x, "# "    y " ",  2, 1, 0 }, \
 { x, "# "    y "\n", 2, 1, 0 }, \
 { x, "#  "   y " ",  3, 1, 0 }, \
 { x, "#  "   y "\n", 3, 1, 0 }, \
 { x, "#   "  y " ",  4, 1, 0 }, \
 { x, "#   "  y "\n", 4, 1, 0 }, \
 { x, "#    " y " ",  5, 1, 0 }, \
 { x, "#    " y "\n", 5, 1, 0
{
 /* order matters */
 { EX_HL_SEQ_T("vstrbase", "Vstr_base") },
 { EX_HL_SEQ_T("pollfd", "struct pollfd") },
 
 { EX_HL_SEQ_CPP("if", "if") },
 { EX_HL_SEQ_CPP("ifndef", "ifndef") },
 { EX_HL_SEQ_CPP("ifdef", "ifdef") },
 { EX_HL_SEQ_CPP("endif", "endif") },
 { EX_HL_SEQ_CPP("define", "define") },
 { EX_HL_SEQ_CPP("include", "include") },
 { EX_HL_SEQ_CPP("elif", "elif") },
 { EX_HL_SEQ_SB("defined", "defined") },
 { EX_HL_SEQ_B("defined", "defined") },
 
 { EX_HL_SEQ_SB("if", "if") },
 { EX_HL_SEQ_WS("else", "else") },
 { EX_HL_SEQ_WS("while", "while") },
 { EX_HL_SEQ_WS("for", "for") },
 { EX_HL_SEQ_WS("return", "return") },
 { EX_HL_SEQ_WS("switch", "switch") },
 { EX_HL_SEQ_WS("case", "case") },
 { EX_HL_SEQ_DEF("defalt", "default") },
 { EX_HL_SEQ_WS("goto", "goto") },
 { EX_HL_SEQ_T("static", "static") },
 { EX_HL_SEQ_T("const", "const") },
 { EX_HL_SEQ_T("void", "void") },
 { EX_HL_SEQ_T("unsigned", "unsigned") },
 { EX_HL_SEQ_T("int", "int") },
 { EX_HL_SEQ_T("char", "char") },
 { EX_HL_SEQ_T("double", "double") },
 { EX_HL_SEQ_T("float", "float") },
 
 { EX_HL_SEQ_SB("exit", "exit") },
 { EX_HL_SEQ_B("err", "err") },
 { EX_HL_SEQ_B("err", "errx") },
 { EX_HL_SEQ_B("warn", "warn") },
 { EX_HL_SEQ_B("warn", "warnx") },

 {NULL, NULL, 0, 0, 0},
};
static size_t ex_hl_max_seq_len = 0;

#define C_DEF 0
#define C_SEQ 1
#define C_STR 2
#define C_CHR 3
#define C_CMO 4
#define C_CMN 5

static void ex_hl_mov_clean(Vstr_base *s1, Vstr_base *s2, size_t len)
{ /* html the elements... */
  while (len > 0)
  {
    size_t count = vstr_cspn_cstr_chrs_fwd(s2, 1, len, "&<>");
    
    vstr_add_vstr(s1, s1->len, s2, 1, count, VSTR_TYPE_ADD_BUF_REF);
    vstr_del(s2, 1, count);
    len -= count;

    if (count)
      continue;

    --len;
    
    switch (vstr_export_chr(s2, 1))
    {
      case '<': /* html stuff ... */
        vstr_add_cstr_ptr(s1, s1->len, "&lt;");
        vstr_del(s2, 1, 1);
        continue;
        
      case '>':
        vstr_add_cstr_ptr(s1, s1->len, "&gt;");
        vstr_del(s2, 1, 1);
        continue;
        
      case '&':
        vstr_add_cstr_ptr(s1, s1->len, "&amp;");
        vstr_del(s2, 1, 1);
        continue;
        
      default:
        ASSERT(FALSE);
        break;
    }  
  }
}
   
static int ex_hl_process(Vstr_base *s1, Vstr_base *s2, int last)
{
  static unsigned int state = C_DEF;

  /* we don't want to create more data, if we are over our limit */
  if (s1->len > EX_MAX_W_DATA_INCORE)
    return (FALSE);

  if (s2->len < ex_hl_max_seq_len)
  {
    if (!s2->len || !last)
      return (FALSE);
  }
  
  while (((s2->len >= ex_hl_max_seq_len) || (s2->len && last)) &&
         (s1->len <= EX_MAX_W_DATA_INCORE))
  {
    size_t len = 0;
  
    switch (state)
    {
      case C_DEF:
        /* this is pretty intimately tied with the array above */
        len = vstr_cspn_cstr_chrs_fwd(s2, 1, s2->len, "# \n(*\"'/&<>");
        vstr_add_vstr(s1, s1->len, s2, 1, len, VSTR_TYPE_ADD_BUF_REF);
        vstr_del(s2, 1, len);
        state = C_SEQ;
        break;
        
      case C_SEQ:
      {
        unsigned int scan = 0;
        const char *seq = NULL;

        switch (vstr_export_chr(s2, 1))
        {
          case '\'':
            state = C_CHR;
            vstr_add_cstr_ptr(s1, s1->len, "<span class=\"chr\">'");
            vstr_del(s2, 1, 1);
            continue;
            
          case '/':
            if (s2->len < 2)
              break;
            
            if (vstr_cmp_cstr_eq(s2, 1, 2, "/*"))
            {
              state = C_CMO;
              vstr_add_cstr_ptr(s1, s1->len, "<span class=\"comment\">/*");
              vstr_del(s2, 1, 2);
              continue;
            }
            if (vstr_cmp_cstr_eq(s2, 1, 2, "//"))
            {
              state = C_CMN;
              vstr_add_cstr_ptr(s1, s1->len, "<span class=\"comment\">//");
              vstr_del(s2, 1, 2);
              continue;
            }
            break;
            
          case '"':
            state = C_STR;
            vstr_add_cstr_ptr(s1, s1->len, "<span class=\"str\">\"");
            vstr_del(s2, 1, 1);
            continue;

          case '<': /* html stuff ... */
            vstr_add_cstr_ptr(s1, s1->len, "&lt;");
            vstr_del(s2, 1, 1);
            continue;
            
          case '>':
            vstr_add_cstr_ptr(s1, s1->len, "&gt;");
            vstr_del(s2, 1, 1);
            continue;
            
          case '&':
            vstr_add_cstr_ptr(s1, s1->len, "&amp;");
            vstr_del(s2, 1, 1);
            continue;
            
          default:
            break;
        }
        
        while (ex_hl_seqs[scan].name)
        {
          seq = ex_hl_seqs[scan].seq;
          len = ex_hl_seqs[scan].len;
          
          if ((len <= s2->len) && vstr_cmp_buf_eq(s2, 1, len, seq, len))
          {
            size_t off  = ex_hl_seqs[scan].off;
            size_t eoff = ex_hl_seqs[scan].eoff;
            unsigned int mid = len - (off + eoff);
            
            vstr_add_vstr(s1, s1->len, s2, 1, off, VSTR_TYPE_ADD_BUF_REF);
            vstr_del(s2, 1, off);

            
            vstr_add_cstr_ptr(s1, s1->len, "<span class=\"");
            vstr_add_cstr_ptr(s1, s1->len, ex_hl_seqs[scan].name);
            vstr_add_cstr_ptr(s1, s1->len, "\">");

            vstr_add_vstr(s1, s1->len, s2, 1, mid, VSTR_TYPE_ADD_BUF_REF);
            vstr_del(s2, 1, mid);
            
            vstr_add_cstr_ptr(s1, s1->len, "</span>");
            /* don't output end marker */
            break;
          }
          
          ++scan;
        }

        if (!ex_hl_seqs[scan].name)
        {
          vstr_add_vstr(s1, s1->len, s2, 1, 1, VSTR_TYPE_ADD_BUF_REF);
          vstr_del(s2, 1, 1);
        }
        
        state = C_DEF;
      }
      break;

      case C_CMO:
        len = vstr_srch_cstr_buf_fwd(s2, 1, s2->len, "*/");
        if (!len)
        {
          ex_hl_mov_clean(s1, s2, s2->len);
          return (TRUE);
        }

        ++len; /* move to last character */
        
        state = C_DEF;
        ex_hl_mov_clean(s1, s2, len);
        vstr_add_cstr_ptr(s1, s1->len, "</span>");
        break;
        
      case C_CMN:
        len = vstr_srch_chr_fwd(s2, 1, s2->len, '\n');
        if (!len)
        {
          ex_hl_mov_clean(s1, s2, s2->len);
          return (TRUE);
        }
        
        state = C_DEF;
        ex_hl_mov_clean(s1, s2, len);
        vstr_add_cstr_ptr(s1, s1->len, "</span>");
        break;
        
      case C_CHR:
        len = vstr_srch_cstr_buf_fwd(s2, 1, s2->len, "'");
        if (!len)
        {
          ex_hl_mov_clean(s1, s2, s2->len);
          return (TRUE);
        }
        
        if ((len == 1) || /* even number of \'s going backwards */
            (!(vstr_spn_cstr_chrs_rev(s2, 1, len - 1, "\\") & 1)))
          state = C_DEF;
        ex_hl_mov_clean(s1, s2, len);
        if (state == C_DEF)
          vstr_add_cstr_ptr(s1, s1->len, "</span>");
        break;
        
      case C_STR:
        len = vstr_srch_cstr_buf_fwd(s2, 1, s2->len, "\"");
        if (!len)
        {
          ex_hl_mov_clean(s1, s2, s2->len);
          return (TRUE);
        }
        
        if ((len == 1) || /* even number of \'s going backwards */
            (!(vstr_spn_cstr_chrs_rev(s2, 1, len - 1, "\\") & 1)))
          state = C_DEF;
        ex_hl_mov_clean(s1, s2, len);
        if (state == C_DEF)
          vstr_add_cstr_ptr(s1, s1->len, "</span>");
        break;
        

      default:
        ASSERT(FALSE);
    }
  }
  
  return (TRUE);
}

static void ex_hl_read_fd_write_stdout(Vstr_base *s1, Vstr_base *s2, int fd)
{
  while (TRUE)
  {
    int io_w_state = IO_OK;
    int io_r_state = io_get(s2, fd);

    if (io_r_state == IO_EOF)
      break;

    ex_hl_process(s1, s2, FALSE);

    io_w_state = io_put(s1, 1);

    io_limit(io_r_state, fd, io_w_state, 1, s1);
  }
}

static void ex_hl_process_limit(Vstr_base *s1, Vstr_base *s2, unsigned int lim)
{
  while (s2->len > lim)
  {
    int proc_data = ex_hl_process(s1, s2, !lim);
    if (!proc_data && (io_put(s1, STDOUT_FILENO) == IO_BLOCK))
      io_block(-1, STDOUT_FILENO);
  }
}


int main(int argc, char *argv[])
{
  Vstr_base *s2 = NULL;
  Vstr_base *s1 = ex_init(&s2);
  int count = 1;
  time_t now = time(NULL);
  
  {
    size_t scan = 0;
    
    while (ex_hl_seqs[scan].name)
    {
      size_t len = strlen(ex_hl_seqs[scan].seq);

      ex_hl_seqs[scan].len = len;
      
      if (ex_hl_max_seq_len < len)
        ex_hl_max_seq_len = len;
      
      ++scan;
    }
  }
  
  /* if no arguments are given just do stdin to stdout */
  if (count >= argc)
  {
    io_fd_set_o_nonblock(STDIN_FILENO);
    ex_hl_read_fd_write_stdout(s1, s2, STDIN_FILENO);
  }

  /* loop through all arguments, open the file specified
   * and do the read/write loop */
  while (count < argc)
  {
    unsigned int ern = 0;

    if (s2->len <= EX_MAX_R_DATA_INCORE)
      vstr_sc_mmap_file(s2, s2->len, argv[count], 0, 0, &ern);

    vstr_add_cstr_ptr(s1, s1->len, "<pre class=\"c2html\">\n");

    if ((ern == VSTR_TYPE_SC_MMAP_FILE_ERR_FSTAT_ERRNO) ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_MMAP_ERRNO) ||
        (ern == VSTR_TYPE_SC_MMAP_FILE_ERR_TOO_LARGE))
    {
      int fd = io_open(argv[count]);

      ex_hl_read_fd_write_stdout(s1, s2, fd);

      if (close(fd) == -1)
        warn("close(%s)", argv[count]);
    }
    else if (ern && (ern != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
      err(EXIT_FAILURE, "add");
    else
      ex_hl_process_limit(s1, s2, 0);
    
    vstr_add_fmt(s1, s1->len,
                 "</pre>\n"
                 "<!-- C to html convertion of %s -->\n"
                 "<!--   done on %s -->\n"
                 "<!--   done by ex_highlight -->\n",
                 argv[count], ctime(&now));
    
    ++count;
  }

  ex_hl_process_limit(s1, s2, 0);

  io_put_all(s1, STDOUT_FILENO);

  exit (ex_exit(s1, s2));
}
