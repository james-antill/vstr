#define _GNU_SOURCE
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "ex_utils.h"

/* hexdump in "readable" format ... note this is a bit more fleshed out than
 * some of the other examples mainly because I actually use it */

/* this is roughly equiv. to...
   
hexdump -e '"%08_ax:"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"
            " " 2/1 "%02X"'
        -e '"  " 16 "%_p" "\n"'

 * ...except that it prints the address in big hex digits, and it doesn't take
 * you 30 minutes to remember how to type it out.
 */

 
#define MAX_R_DATA_INCORE (1024 * 1024)
#define MAX_W_DATA_INCORE (1024 * 8)

#if 0
# define EX_HEXDUMP_X8(s1, num) \
  vstr_add_fmt(s1, (s1)->len, "%08X:", (num))
# define EX_HEXDUMP_X2X2(s1, num1, num2) \
  vstr_add_fmt(s1, (s1)->len, " %02X%02X", (num1), (num2))
# define EX_HEXDUMP_X2__(s1, num1) \
  vstr_add_fmt(s1, (s1)->len, " %02X  ",   (num1))
#else
# define EX_HEXDUMP_X8(s1, num) do { unsigned char xbuf[9]; \
  const char *digs = "0123456789ABCDEF"; \
  xbuf[8] = ':'; \
  xbuf[7] = digs[(((num) >>  0) & 0xf)]; \
  xbuf[6] = digs[(((num) >>  4) & 0xf)]; \
  xbuf[5] = digs[(((num) >>  8) & 0xf)]; \
  xbuf[4] = digs[(((num) >> 12) & 0xf)]; \
  xbuf[3] = digs[(((num) >> 16) & 0xf)]; \
  xbuf[2] = digs[(((num) >> 20) & 0xf)]; \
  xbuf[1] = digs[(((num) >> 24) & 0xf)]; \
  xbuf[0] = digs[(((num) >> 28) & 0xf)]; \
  vstr_add_buf(s1, (s1)->len, xbuf, 9); } while (FALSE)
# define EX_HEXDUMP_X2X2(s1, num1, num2) do { unsigned char xbuf[5]; \
  const char *digs = "0123456789ABCDEF"; \
  xbuf[0] = ' '; \
  xbuf[4] = digs[(((num2) >> 0) & 0xf)]; \
  xbuf[3] = digs[(((num2) >> 4) & 0xf)]; \
  xbuf[2] = digs[(((num1) >> 0) & 0xf)]; \
  xbuf[1] = digs[(((num1) >> 4) & 0xf)]; \
  vstr_add_buf(s1, (s1)->len, xbuf, 5); } while (FALSE)
# define EX_HEXDUMP_X2__(s1, num1) do { unsigned char xbuf[5]; \
  const char *digs = "0123456789ABCDEF"; \
  xbuf[4] = ' '; \
  xbuf[3] = ' '; \
  xbuf[2] = digs[(((num1) >> 0) & 0xf)]; \
  xbuf[1] = digs[(((num1) >> 4) & 0xf)]; \
  xbuf[0] = ' '; \
  vstr_add_buf(s1, (s1)->len, xbuf, 5); } while (FALSE)
#endif


static void ex_hexdump_process(Vstr_base *str1, Vstr_base *str2, int last)
{
  static unsigned int addr = 0;
  unsigned int flags = VSTR_FLAG06(CONV_UNPRINTABLE_ALLOW,
		                   COMMA, DOT, _, SP, HSP, HIGH);

  while (str2->len >= 16)
  {
    unsigned char buf[16];
    
    vstr_export_buf(str2, 1, 16, buf, sizeof(buf));

    EX_HEXDUMP_X8(str1, addr);

    EX_HEXDUMP_X2X2(str1, buf[ 0], buf[ 1]);
    EX_HEXDUMP_X2X2(str1, buf[ 2], buf[ 3]);
    EX_HEXDUMP_X2X2(str1, buf[ 4], buf[ 5]);
    EX_HEXDUMP_X2X2(str1, buf[ 6], buf[ 7]);
    EX_HEXDUMP_X2X2(str1, buf[ 8], buf[ 9]);
    EX_HEXDUMP_X2X2(str1, buf[10], buf[11]);
    EX_HEXDUMP_X2X2(str1, buf[12], buf[13]);
    EX_HEXDUMP_X2X2(str1, buf[14], buf[15]);
    
    VSTR_ADD_CSTR_BUF(str1, str1->len, "  ");
    
    vstr_conv_unprintable_chr(str2, 1, 16, flags, '.');
    vstr_add_vstr(str1, str1->len, str2, 1, 16, VSTR_TYPE_ADD_ALL_BUF);
    VSTR_ADD_CSTR_BUF(str1, str1->len, "\n");
    vstr_del(str2, 1, 16);
    
    addr += 0x10;

    if (str1->len > MAX_W_DATA_INCORE)
      return;
  }

  if (last && str2->len)
  {
    unsigned char buf[16];
    size_t got = str2->len;
    size_t missing = 16 - str2->len;
    const char *ptr = buf;

    missing -= (missing % 2);
    vstr_export_buf(str2, 1, str2->len, buf, sizeof(buf));

    EX_HEXDUMP_X8(str1, addr);
    
    while (got >= 2)
    {
      EX_HEXDUMP_X2X2(str1, ptr[0], ptr[1]);
      got -= 2;
      ptr += 2;
    }
    if (got)
    {
      EX_HEXDUMP_X2__(str1, ptr[0]);
      got -= 2;
    }

    vstr_add_rep_chr(str1, str1->len, ' ', (missing * 2) + (missing / 2) + 2);
    
    vstr_conv_unprintable_chr(str2, 1, str2->len, flags, '.');
    vstr_add_vstr(str1, str1->len, str2, 1, str2->len, VSTR_TYPE_ADD_ALL_BUF);

    VSTR_ADD_CSTR_BUF(str1, str1->len, "\n");

    vstr_del(str2, 1, str2->len);
  }

  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("adding data:");
}

static void ex_hexdump_read_fd_write_stdout(Vstr_base *str1, Vstr_base *str2,
                                            int fd)
{
  unsigned int err = 0;
  int keep_going = TRUE;
  
  while (keep_going)
  {
    EX_UTILS_LIMBLOCK_READ_ALL(str2, fd, keep_going);
    
    ex_hexdump_process(str1, str2, !keep_going);

    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
  }
}


int main(int argc, char *argv[])
{ /* This is "hexdump", as it should be by default */
  Vstr_conf *conf = NULL;
  Vstr_base *str1 = NULL;
  Vstr_base *str2 = NULL;
  int count = 1; /* skip the program name */
  struct stat stat_buf;
  
  if (!vstr_init())
    errno = ENOMEM, DIE("vstr_init:");
  
  if (fstat(1, &stat_buf) == -1)
    stat_buf.st_blksize = 4 * 1024;
  
  if (!(conf = vstr_make_conf()))
    errno = ENOMEM, DIE("vstr_make_conf:");

  if (!stat_buf.st_blksize)
    stat_buf.st_blksize = 4 * 1024;
  
  vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);
  vstr_make_spare_nodes(conf, VSTR_TYPE_NODE_BUF, 32);
  
  str1 = vstr_make_base(conf);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");
  
  str2 = vstr_make_base(conf);
  if (!str2)
    errno = ENOMEM, DIE("vstr_make_base:");

  vstr_free_conf(conf);

  ex_utils_set_o_nonblock(1);
  
  if (count == argc)  /* use stdin */
  {
    ex_utils_set_o_nonblock(0);
    ex_hexdump_read_fd_write_stdout(str1, str2, 0);
  }
  
  while (count < argc)
  {
    unsigned int err = 0;
    
    if (str2->len < MAX_R_DATA_INCORE)
      vstr_sc_mmap_file(str2, str2->len, argv[count], 0, 0, &err);
    
    if ((err == VSTR_TYPE_SC_MMAP_FILE_ERR_MMAP_ERRNO) ||
        (err == VSTR_TYPE_SC_MMAP_FILE_ERR_TOO_LARGE))
    {
      int fd = open(argv[count], O_RDONLY | O_LARGEFILE | O_NOCTTY);
      
      if (fd == -1)
        WARN("open(%s):", argv[count]);

      ex_utils_set_o_nonblock(fd);
      ex_hexdump_read_fd_write_stdout(str1, str2, fd);
      
      close(fd);
    }
    else if (err && (err != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
      DIE("add:");
    else /* worked */
      ex_hexdump_process(str1, str2, FALSE);

    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
    
    ++count;
  }

  while (str2->len)
  {
    ex_hexdump_process(str1, str2, TRUE);

    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
  }
  
  vstr_free_base(str2);

  while (str1->len)
    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
  
  vstr_free_base(str1);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
