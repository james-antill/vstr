#define _GNU_SOURCE
/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "ex_utils.h"
/* do a rot13 of ASCII text */

/* configuration:
   how to do it ... */
#define USE_ITER  1
#define USE_EXPORT_CHR  0
#define USE_SUB_CHR     0
#define USE_CSTR_MALLOC 0


#define MAX_R_DATA_INCORE (1024 * 1024)
#define MAX_W_DATA_INCORE (1024 * 8)

#define ROT13_LETTER(x) ( \
 (((x) >= 'A' && (x) <= 'M') || \
  ((x) >= 'a' && (x) <= 'm')) ? ((x) + 13) : ((x) - 13) \
 )

#if 0
#define ROT13_MAP(x) ( \
 ((((x) >= 'A') && ((x) <= 'Z')) || \
  (((x) >= 'a') && ((x) <= 'z'))) ? ROT13_LETTER(x) : (x) \
 )
#else
static char rot13_map[] = {
 1*   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 1*  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 1*  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 1*  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 1*  64, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 65, 66,
 1*  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 91, 92, 93, 94, 95,
 1*  96,110,111,112,113,114,115,116,117,118,119,120,121,122, 97, 98,
 1*  99,100,101,102,103,104,105,106,107,108,109,123,124,125,126,127,
 1* 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 1* 144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
 1* 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 1* 176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
 1* 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 1* 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 1* 224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
 1* 240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
#define ROT13_MAP(x) rot13_map[(unsigned char)(x)]
#endif

static void ex_rot13_process(Vstr_base *str1, Vstr_base *str2)
{
  size_t count = 0;
  static const char chrs[] = ("abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

#if USE_ITER
  Vstr_iter iter[1];

  if (!str2->len)
    return;
  
  if (!vstr_iter_fwd_beg(str2, 1, str2->len, iter))
    abort();
  
  do
  {
    unsigned int scan = 0;
    while (scan < iter->len)
    {
      char tmp = ROT13_MAP(iter->ptr[scan]);
      vstr_add_rep_chr(str1, str1->len, tmp, 1);
      ++scan;
    }
  } while (vstr_iter_fwd_nxt(iter));
  
  vstr_del(str2, 1, str2->len);
#endif
#if USE_EXPORT_CHR
  unsigned int scan = 0;
  while (scan++ < str2->len)
  {
    char tmp = vstr_export_chr(str2, scan);
    tmp = ROT13_MAP(tmp);
    vstr_add_rep_chr(str1, str1->len, tmp, 1);
  }
  vstr_del(str2, 1, str2->len);
#endif
#if USE_SUB_CHR
  unsigned int scan = 0;
  while (scan++ < str2->len)
  {
    char tmp = vstr_export_chr(str2, scan);
    tmp = ROT13_MAP(tmp);
    vstr_sub_rep_chr(str2, scan, 1, tmp, 1);
  }
  vstr_add_vstr(str1, str1->len, str2, 1, str2->len,
                VSTR_TYPE_ADD_BUF_REF);
  vstr_del(str2, 1, str2->len);
#endif
#if USE_CSTR_MALLOC
  while (str2->len)
  {
    if ((count = VSTR_CSPN_CSTR_CHRS_FWD(str2, 1, str2->len, chrs)))
    {
      vstr_add_vstr(str1, str1->len, str2, 1, count,
                    VSTR_TYPE_ADD_BUF_REF);
      vstr_del(str2, 1, count);
      
      if (str1->len > MAX_W_DATA_INCORE)
        return;
    }
    
    if ((count = VSTR_SPN_CSTR_CHRS_FWD(str2, 1, str2->len, chrs)))
    {
      char *ptr = vstr_export_cstr_malloc(str2, 1, count);
      Vstr_ref *ref = vstr_ref_make_ptr(ptr, vstr_ref_cb_free_ptr_ref);
      
      if (!ref || !ref->ptr)
        errno = ENOMEM, DIE("vstr_make_conf:");
      
      while (*ptr)
      {
        *ptr = ROT13_LETTER(*ptr);
        ++ptr;
      }
      
      vstr_add_ref(str1, str1->len, ref, 0, count);
      vstr_del(str2, 1, count);
    }
  }
#endif
}


static void ex_rot13_read_fd_write_stdout(Vstr_base *str1, Vstr_base *str2,
                                          int fd)
{
  unsigned int err = 0;
  int keep_going = TRUE;
  
  while (keep_going)
  {
    EX_UTILS_LIMBLOCK_READ_ALL(str2, fd, keep_going);
    
    ex_rot13_process(str1, str2);

    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
  }
}

int main(int argc, char *argv[])
{
  Vstr_conf *conf = NULL;
  Vstr_base *str1 = NULL;
  Vstr_base *str2 = NULL;
  int count = 1;
  struct stat stat_buf;

  if (!vstr_init())
      exit (EXIT_FAILURE);

  if (fstat(1, &stat_buf) == -1)
    DIE("fstat:");

  if (!(conf = vstr_make_conf()))
    errno = ENOMEM, DIE("vstr_make_conf:");

  if (!stat_buf.st_blksize)
    stat_buf.st_blksize = 4 * 1024;
  
  vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, stat_buf.st_blksize);
  
  str1 = vstr_make_base(conf);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");

  str2 = vstr_make_base(conf);
  if (!str2)
    errno = ENOMEM, DIE("vstr_make_base:");

  vstr_free_conf(conf);
  
  ex_utils_set_o_nonblock(1);
  
  if (count >= argc)  /* use stdin */
  {
    ex_utils_set_o_nonblock(0);
    ex_rot13_read_fd_write_stdout(str1, str2, 0);
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
        DIE("open:");
      
      ex_utils_set_o_nonblock(fd);
      ex_rot13_read_fd_write_stdout(str1, str2, fd);
      
      close(fd);
    }
    else if (err && (err != VSTR_TYPE_SC_MMAP_FILE_ERR_CLOSE_ERRNO))
      DIE("add:");
    else
      ex_rot13_process(str1, str2);

    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
   
    ++count;
  }

  while (str2->len)
  {
    ex_rot13_process(str1, str2);
    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
  }
  
  vstr_free_base(str2);

  while (str1->len)
    EX_UTILS_LIMBLOCK_WRITE_ALL(str1, 1);
  
  vstr_free_base(str1);
 
  vstr_exit();

  exit (EXIT_SUCCESS);
}
