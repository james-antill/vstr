#define _GNU_SOURCE 1

/* This is a slowcat program, you can limit the number of bytes written and
 * how often they are written.
 * Does stdin if no args are given */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/poll.h>

#include <timer_q.h>

#include <vstr.h>

#include "ex_utils.h"

#define MAX_R_DATA_INCORE (1024 * 1024)
#define MAX_W_DATA_INCORE (1024 * 8)

#define EX_SLOWCAT_WRITE_BYTES 80
#define EX_SLOWCAT_WRITE_WAIT_SEC 1
#define EX_SLOWCAT_WRITE_WAIT_USEC 0 /* 500000 */

typedef struct ex_slowcat_vars
{
 unsigned int opt_write_bytes;
 unsigned int opt_write_wait_sec;
 unsigned int opt_write_wait_usec;
 
 int argc;
 char **argv;
 Vstr_base *str1;
 int arg_count;
 int fcntl_flags;
 Timer_q_base *base;
 Timer_q_node *node;
 int fd;

 unsigned int finished_reading_data : 1;
 unsigned int finished_reading_file : 1;
} ex_slowcat_vars;

typedef struct ex_slowcat_mmap_ref
{
 Vstr_ref ref;
 size_t len;
} ex_slowcat_mmap_ref;

static int have_mmaped_file = 0;

static void ex_slowcat_ref_munmap(Vstr_ref *passed_ref)
{
 ex_slowcat_mmap_ref *ref = (ex_slowcat_mmap_ref *)passed_ref;
 munmap(ref->ref.ptr, ref->len);
 free(ref);

 --have_mmaped_file;
}

static void ex_slowcat_del_write(Vstr_base *base, int fd, size_t max_bytes)
{
 Vstr_base *cpy = base;
 struct iovec *vec;
 unsigned int num = 0;
 size_t len = 0;
 ssize_t bytes = 0;
 unsigned int hacked_off = 0;
 size_t hacked_len = 0;
 
 if (!base->len)
   return;
 
 len = vstr_export_iovec_ptr_all(cpy, &vec, &num);
 if (!len)
   errno = ENOMEM, DIE("vstr_export_iovec_ptr_all:");

 if (max_bytes > len)
   max_bytes = len;
 else
 {
  unsigned int count = 0;
  
  while (count < max_bytes)
  {
   size_t tmp = vec[hacked_off].iov_len;

   hacked_len = tmp;
   
   if (count + tmp > max_bytes)
     tmp = max_bytes - count;

   vec[hacked_off++].iov_len = tmp;
   count += tmp;
  }
  assert(count == max_bytes);

  num = hacked_off;
 }
 
 if ((size_t)(bytes = writev(fd, vec, num)) != len)
 {
  if ((bytes == -1) && (errno != EAGAIN))
    DIE("writev:");
  if (bytes == -1)
    return;
 }

 if (hacked_off)
   vec[hacked_off - 1].iov_len = hacked_len; /* restore */
 
 vstr_del(cpy, 1, (size_t)bytes);
}

static void ex_slowcat_mmap_file(Vstr_base *str1, int fd, size_t len)
{
 ex_slowcat_mmap_ref *ref = NULL;
 caddr_t addr = NULL;

 if (!len)
   return;
 
 if (!(ref = malloc(sizeof(ex_slowcat_mmap_ref))))
   errno = ENOMEM, DIE("malloc ex_slowcat_mmap:");
 
 ref->len = len;
 
 addr = mmap(NULL, ref->len, PROT_READ, MAP_SHARED, fd, 0);
 if (addr == (caddr_t)-1)
   DIE("mmap:");
 
 if (close(fd) == -1)
   DIE("close:");
 
 ref->ref.func = ex_slowcat_ref_munmap;
 ref->ref.ptr = (char *)addr;
 ref->ref.ref = 0;
 
 if (offsetof(ex_slowcat_mmap_ref, ref))
   DIE("assert");
 
 if (!vstr_add_ref(str1, str1->len, &ref->ref, 0, ref->len))
   errno = ENOMEM, DIE("vstr_add_ref:");

 ++have_mmaped_file;
}

static void ex_slowcat_timer_func(int type, void *data)
{
 ex_slowcat_vars *v = data;
 struct timeval s_tv;
 int fin_data = v->finished_reading_data;
 
 if (type == TIMER_Q_TYPE_CALL_DEL)
   return;

 if (!v->finished_reading_data && (v->str1->len < v->opt_write_bytes))
 { /* do a read of the right ammount ... */
  if (!v->argc && !v->arg_count)
  {
   v->finished_reading_file = FALSE;
   v->fd = 0; /* use stdin -- do read on already open file */
  }
  else
  {
   if (v->finished_reading_file)
   {
    struct stat stat_buf;

    assert(v->arg_count < v->argc);
    
    v->finished_reading_file = FALSE;
    
    v->fd = open(v->argv[v->arg_count], O_RDONLY | O_LARGEFILE | O_NOCTTY);
    
    if (v->fd == -1)
      DIE("open(%s):", v->argv[v->arg_count]);
    
    if (fstat(v->fd, &stat_buf) == -1)
      DIE("fstat(%s):", v->argv[v->arg_count]);
    
    ++v->arg_count;
    
    if (S_ISREG(stat_buf.st_mode))
    {
     ex_slowcat_mmap_file(v->str1, v->fd, stat_buf.st_size);
     if (v->arg_count >= v->argc)
       v->finished_reading_data = TRUE;
     v->finished_reading_file = TRUE;
    }
    else
    {
     int tmp_fcntl_flags = 0;
     
     if ((tmp_fcntl_flags = fcntl(v->fd, F_GETFL)) == -1)
       DIE("fcntl(GET NONBLOCK):");
     if (!(tmp_fcntl_flags & O_NONBLOCK) &&
         (fcntl(v->fd, F_SETFL, tmp_fcntl_flags | O_NONBLOCK) == -1))
       DIE("fcntl(SET NONBLOCK):");
    }
   }
  }
  
  if (!v->finished_reading_file)
  {
    unsigned int err = 0;
    int keep_going = TRUE;
    
    EX_UTILS_LIMBLOCK_READ_ALL(v->str1, v->fd, keep_going);

    if (!keep_going)
    {
      if (close(v->fd) == -1)
        DIE("close:");
      
      if (v->arg_count >= v->argc)
        v->finished_reading_data = TRUE;
      v->finished_reading_file = TRUE;
    }
  }
 }
 
 if (!fin_data && v->finished_reading_data)
 { /* we've just finished */
  if (v->str1->len) /* set it back to blocking, to be nice */
    if (fcntl(1, F_SETFL, v->fcntl_flags & ~O_NONBLOCK) == -1)
      DIE("fcntl(SET BLOCK):");
 }
 
 /* do a write of the right ammount */
 if (!vstr_sc_write_fd(v->str1, 1, v->opt_write_bytes, v->fd, NULL))
 {
   if ((errno != EAGAIN) && (errno != EINTR))
     DIE("write:");
   if (v->str1->len > MAX_W_DATA_INCORE)
     ex_utils_poll_stdout();
 }

 if (v->finished_reading_data && !v->str1->len)
   return;

 if (type == TIMER_Q_TYPE_CALL_RUN_ALL)
   return;

 gettimeofday(&s_tv, NULL);
 TIMER_Q_TIMEVAL_ADD_SECS(&s_tv, v->opt_write_wait_sec, v->opt_write_wait_usec);

 v->node = timer_q_add_node(v->base, v, &s_tv, TIMER_Q_FLAG_NODE_DEFAULT);
 if (!v->node)
   errno = ENOMEM, DIE("timer_q_add_node:");
}

static int ex_slowcat_init_cmd_line(ex_slowcat_vars *v, int argc, char *argv[])
{
 char optchar = 0;
 const char *program_name = "talker";
 struct option long_options[] =
   {
    {"bytes", required_argument, NULL, 'b'},
    {"help", no_argument, NULL, 'h'},
    {"seconds", required_argument, NULL, 's'},
    {"useconds", required_argument, NULL, 'u'},
    {"version", no_argument, NULL, 'V'},   
    {NULL, 0, NULL, 0}
   };
 FILE *help_stdout = NULL;
 
 help_stdout = stdout;
 
 if (argv[0])
 {
  if ((program_name = strrchr(argv[0], '/')))
    ++program_name;
  else
    program_name = argv[0];
 }

 while ((optchar = getopt_long(argc, argv, "b:hs:u:vHV",
                               long_options, NULL)) != EOF)
   switch (optchar)
   {
    case 'b':
      v->opt_write_bytes = atoi(optarg);
      break;
 
    case '?':
      fprintf(stderr, " The option -- %c -- is not valid.\n", optchar);
      help_stdout = stderr;
    case 'H':
    case 'h':
      fprintf(help_stdout, "\n Format: %s [-bhsuvHV] [files]\n"
              " --bytes -b        - Number of bytes to write at once.\n"
              " --help -h         - Print this message.\n"
              " --seconds -s      - Number of seconds to wait between write calls.\n"
              " --useconds -u     - Number of micro seconds to wait between write calls.\n"
              " --version -v      - Print the version string.\n",
              program_name);
      if (optchar == '?')
        exit (EXIT_FAILURE);
      else
        exit (EXIT_SUCCESS);
      
    case 's':
      v->opt_write_wait_sec = atoi(optarg);
      break;

    case 'u':
      v->opt_write_wait_usec = atoi(optarg);
      break;

    case 'v':
    case 'V':
      printf(" %s is version 0.1, compiled on -- "
             __DATE__ " -- at -- " __TIME__" --.\n",
             program_name);
      exit (EXIT_SUCCESS);
      
   }

 return (optind);
}

int main(int argc, char *argv[])
{ /* This is "slowcat" */
 ex_slowcat_vars v;
 struct timeval s_tv;
 const struct timeval *tv = NULL;

 /* init stuff... */
 v.opt_write_bytes = EX_SLOWCAT_WRITE_BYTES;
 v.opt_write_wait_sec = EX_SLOWCAT_WRITE_WAIT_SEC;
 v.opt_write_wait_usec = EX_SLOWCAT_WRITE_WAIT_USEC;
 
 v.argc = argc;
 v.argv = argv;
 v.str1 = NULL;
 v.arg_count = 0;
 v.fcntl_flags = 0;
 v.base = NULL;
 v.node = NULL;
 v.finished_reading_data = FALSE;
 v.finished_reading_file = TRUE;
 
 if (!vstr_init())
   errno = ENOMEM, DIE("vstr_init:");

 /* setup code... */

 ex_slowcat_init_cmd_line(&v, argc, argv);

 v.argc -= optind;
 v.argv += optind;
 
 v.base = timer_q_add_base(ex_slowcat_timer_func, TIMER_Q_FLAG_BASE_DEFAULT);
 if (!v.base)
   errno = ENOMEM, DIE("timer_q_add_base:");
 
 gettimeofday(&s_tv, NULL);
 TIMER_Q_TIMEVAL_ADD_SECS(&s_tv, 1, 500000); /* 1.5 seconds */

 v.node = timer_q_add_node(v.base, &v, &s_tv, TIMER_Q_FLAG_NODE_DEFAULT);
 if (!v.node)
   errno = ENOMEM, DIE("timer_q_add_node:"); 
 
 v.str1 = vstr_make_base(NULL);
 if (!v.str1)
   errno = ENOMEM, DIE("vstr_make_base:"); 

 if ((v.fcntl_flags = fcntl(1, F_GETFL)) == -1)
   DIE("fcntl(GET NONBLOCK):");
 if (!(v.fcntl_flags & O_NONBLOCK) &&
     (fcntl(1, F_SETFL, v.fcntl_flags | O_NONBLOCK) == -1))
   DIE("fcntl(SET NONBLOCK):");

 while ((tv = timer_q_first_timeval()))
 {
  long wait_period = 0;
  
  gettimeofday(&s_tv, NULL);
  
  wait_period = timer_q_timeval_diff_usecs(tv, &s_tv);
  if (wait_period > 0)
    usleep(wait_period);

  gettimeofday(&s_tv, NULL);
  
  timer_q_run_norm(&s_tv);
 }
 
 assert(!have_mmaped_file);

 timer_q_del_base(v.base);
 
 vstr_free_base(v.str1);

 vstr_exit();
 
 exit (EXIT_SUCCESS);
}
