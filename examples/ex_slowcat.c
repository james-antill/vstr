/* This is a slowcat program, you can limit the number of bytes written and
 * how often they are written.
 * Does stdin if no args are given */

#define EX_UTILS_NO_USE_INIT   1
#define EX_UTILS_NO_USE_EXIT   1
#define EX_UTILS_NO_USE_LIMIT  1
#define EX_UTILS_NO_USE_PUT    1
#define EX_UTILS_NO_USE_PUTALL 1
#include "ex_utils.h"

#include <timer_q.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

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

#undef MIN
#define MIN(x, y, z) ((((z) (x)) < ((z) (y))) ? ((z) (x)) : ((z) (y)))

static void ex_slowcat_timer_func(int type, void *data)
{
 ex_slowcat_vars *v = data;
 struct timeval s_tv;
 int fin_data = v->finished_reading_data;
 size_t len = 0;
 
 if (type == TIMER_Q_TYPE_CALL_DEL)
   return;

 if (!v->finished_reading_data && (v->str1->len < v->opt_write_bytes))
 {
   if (!v->argc && !v->arg_count)
   {
     v->finished_reading_file = FALSE;
     v->fd = 0; /* use stdin -- do read on already open file */
   }
   else
   {
     if (v->finished_reading_file)
     {
       assert(v->arg_count < v->argc);

       v->finished_reading_file = FALSE;

       v->fd = io_open(v->argv[v->arg_count]);

       if (v->fd == -1)
         err(EXIT_FAILURE, "open(%s)", v->argv[v->arg_count]);

       ++v->arg_count;

       if (vstr_sc_mmap_fd(v->str1, v->str1->len, v->fd, 0, 0, NULL))
       {
         if (v->arg_count >= v->argc)
           v->finished_reading_data = TRUE;
         v->finished_reading_file = TRUE;
       }
       else
         io_fd_set_o_nonblock(v->fd);
     }
   }

   if (!v->finished_reading_file)
     do
     {
       int state = io_get(v->str1, v->fd);

       if (state == IO_EOF)
       {
         if (close(v->fd) == -1)
           err(EXIT_FAILURE, "close");
       
         if (v->arg_count >= v->argc)
           v->finished_reading_data = TRUE;
         v->finished_reading_file = TRUE;
       }
     } while (v->opt_write_bytes > v->str1->len);
 }

 if (!fin_data && v->finished_reading_data)
 { /* we've just finished */
  if (v->str1->len) /* set it back to blocking, to be nice */
    if (fcntl(1, F_SETFL, v->fcntl_flags & ~O_NONBLOCK) == -1)
      err(EXIT_FAILURE, "fcntl(SET BLOCK)");
 }

 len = MIN(v->opt_write_bytes, v->str1->len, size_t);
 /* do a write of the right ammount */
 if (!vstr_sc_write_fd(v->str1, 1, len, STDOUT_FILENO, NULL))
 {
   if (errno != EAGAIN)
     err(EXIT_FAILURE, "write");
   if (v->str1->len > EX_MAX_W_DATA_INCORE)
     io_block(-1, STDOUT_FILENO);
 }

 if (v->finished_reading_data && !v->str1->len)
   return;

 if (type == TIMER_Q_TYPE_CALL_RUN_ALL)
   return;

 gettimeofday(&s_tv, NULL);
 TIMER_Q_TIMEVAL_ADD_SECS(&s_tv, v->opt_write_wait_sec, v->opt_write_wait_usec);

 v->node = timer_q_add_node(v->base, v, &s_tv, TIMER_Q_FLAG_NODE_DEFAULT);
 if (!v->node)
   errno = ENOMEM, err(EXIT_FAILURE, "timer_q_add_node");
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
   errno = ENOMEM, err(EXIT_FAILURE, "vstr_init");

 /* setup code... */

 ex_slowcat_init_cmd_line(&v, argc, argv);

 v.argc -= optind;
 v.argv += optind;

 v.base = timer_q_add_base(ex_slowcat_timer_func, TIMER_Q_FLAG_BASE_DEFAULT);
 if (!v.base)
   errno = ENOMEM, err(EXIT_FAILURE, "timer_q_add_base");

 gettimeofday(&s_tv, NULL);
 TIMER_Q_TIMEVAL_ADD_SECS(&s_tv, 1, 500000); /* 1.5 seconds */

 v.node = timer_q_add_node(v.base, &v, &s_tv, TIMER_Q_FLAG_NODE_DEFAULT);
 if (!v.node)
   errno = ENOMEM, err(EXIT_FAILURE, "timer_q_add_node");

 v.str1 = vstr_make_base(NULL);
 if (!v.str1)
   errno = ENOMEM, err(EXIT_FAILURE, "vstr_make_base");

 if ((v.fcntl_flags = fcntl(1, F_GETFL)) == -1)
   err(EXIT_FAILURE, "fcntl(GET NONBLOCK)");
 if (!(v.fcntl_flags & O_NONBLOCK) &&
     (fcntl(1, F_SETFL, v.fcntl_flags | O_NONBLOCK) == -1))
   err(EXIT_FAILURE, "fcntl(SET NONBLOCK)");

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

 timer_q_del_base(v.base);

 vstr_free_base(v.str1);

 vstr_exit();

 exit (EXIT_SUCCESS);
}
