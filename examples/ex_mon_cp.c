#define _GNU_SOURCE

/* This monitors a copy to a file, and prints nice stats */

#define VSTR_COMPILE_INCLUDE 1

#include <vstr.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <time.h>

#include "ex_utils.h"

#define MAX_W_DATA_INCORE (1024 * 8)

#define CUR_RATE_NUM_SECS 60

int main(int argc, char *argv[])
{
  Vstr_conf *conf = NULL;
  Vstr_base *s1 = NULL;
  Vstr_base *s2 = NULL;
  int fd = -1;
  struct stat stat_buf;
  time_t beg_time;
  unsigned int beg_sz = 0;
  unsigned int last_sz = 0;
  unsigned int count = 0;
  struct
  {
   time_t timestamp;
   unsigned int sz;
  } cur[CUR_RATE_NUM_SECS];
  
  if (!vstr_init())
    errno = ENOMEM, DIE("vstr_init:");
  
  if (fstat(1, &stat_buf) == -1)
    DIE("fstat:");
  
  if (!(conf = vstr_make_conf()))
    errno = ENOMEM, DIE("vstr_make_conf:");
  
  if (!stat_buf.st_blksize)
    stat_buf.st_blksize = 4096;
  
  vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);
  
  s2 = vstr_make_base(NULL);
  if (!s2)
    errno = ENOMEM, DIE("vstr_make_base:");
  
  s1 = vstr_make_base(conf);
  if (!s1)
    errno = ENOMEM, DIE("vstr_make_base:");
  
  vstr_free_conf(conf);
  
  if (argc != 2)
  {
    size_t pos = 0;
    
    VSTR_ADD_CSTR_PTR(s1, 0, argc ? argv[0] : "mon_cp");
    
    if ((pos = vstr_srch_chr_rev(s1, 1, s1->len, '/')))
      vstr_del(s1, 1, pos);
    
    VSTR_ADD_CSTR_PTR(s1, 0, " Format: ");
    VSTR_ADD_CSTR_PTR(s1, s1->len, " <filename>\n");
    
    while (s1->len)
      EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 2);
  
    exit (EXIT_FAILURE);
  }
  
  if ((fd = open(argv[1], O_RDONLY | O_LARGEFILE | O_NOCTTY)) == -1)
    DIE("open:");

  if (fstat(fd, &stat_buf) == -1)
    DIE("fstat:");

  vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_cntl_conf(s2->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_sc_fmt_add_bkmg_Byte_uint(s1->conf, "{BKMG:%u}");
  vstr_sc_fmt_add_bkmg_Byte_uint(s2->conf, "{BKMG:%u}");
  vstr_sc_fmt_add_bkmg_Bytes_uint(s2->conf, "{BKMG/s:%u}");

  beg_time = time(NULL); --beg_time;
  beg_sz = last_sz = stat_buf.st_size;
  while (count < CUR_RATE_NUM_SECS)
  {
    cur[count].timestamp = beg_time;
    cur[count].sz = beg_sz;
    ++count;
  }

  vstr_add_fmt(s1, 0, "Start size = ${BKMG:%u}, current period = %u seconds.\n",
               beg_sz, CUR_RATE_NUM_SECS);

  count = 0;
  while (count < (5 * 60)) /* same for 5 minutes */
  {
    size_t prev_len = s2->len;
    time_t now;
    
    if (fstat(fd, &stat_buf) == -1)
      DIE("fstat:");

    ++count;
    if (last_sz != (unsigned int)stat_buf.st_size)
      count = 0;
    if (last_sz > (unsigned int)stat_buf.st_size)
      break;
    last_sz = stat_buf.st_size;

    now = time(NULL);
    memmove(cur, cur + 1, sizeof(cur[0]) * (CUR_RATE_NUM_SECS - 1));
    cur[CUR_RATE_NUM_SECS - 1].timestamp = now;
    cur[CUR_RATE_NUM_SECS - 1].sz = last_sz;
    
    vstr_del(s2, 1, s2->len);
    vstr_add_fmt(s2, 0,
                 "CP = $8{BKMG:%u} | "
                 "Rate = $10{BKMG/s:%u} | "
                 "CP(C) = $8{BKMG:%u} | "
                 "Rate(C) = $10{BKMG/s:%u}",
                 (last_sz - beg_sz),
                 (unsigned int)((last_sz - beg_sz) /
                                (now - beg_time)),
                 (last_sz - cur[0].sz),
                 (unsigned int)((last_sz - cur[0].sz) /
                                (now - cur[0].timestamp)));

    if (s2->len < prev_len)
    {
      vstr_add_rep_chr(s1, s1->len, '\b', prev_len - s2->len);
      vstr_add_rep_chr(s1, s1->len, ' ',  prev_len - s2->len);
    }
    
    vstr_add_rep_chr(s1, s1->len, '\b', prev_len);
    vstr_add_vstr(s1, s1->len, s2, 1, s2->len, 0);
    
    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);

    sleep(1);
  }
  
  close(fd);
  
  vstr_add_rep_chr(s1, s1->len, '\n', 1);
  while (s1->len)
    EX_UTILS_LIMBLOCK_WRITE_ALL(s1, 1);
  
  vstr_free_base(s1);
  vstr_free_base(s2);
  
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
