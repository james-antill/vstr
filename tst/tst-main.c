
#include "autoconf.h"

#ifdef HAVE_POSIX_HOST
# define USE_MMAP 1
# include "main_system.h"
#else
# include "main_noposix_system.h"
#endif

#include <float.h>

#include <assert.h>

#include "fix.h"

#include "vstr.h"

static int tst(void); /* fwd */

#define TST_BUF_SZ (1024 * 32)

static Vstr_base *s1 = NULL; /* normal */
static Vstr_base *s2 = NULL; /* locale en_US */
static Vstr_base *s3 = NULL; /* buf size = 4 */
static char buf[TST_BUF_SZ];

static const char *rf;

static void die(void)
{
  fprintf(stderr, "Error(%s) abort\n", rf);
  abort();
}

#define PRNT_CSTR(s) \
 fprintf(stderr, "cstr(%s):%zu%*s = %s\n", #s , strlen(s), \
         ((4 - strlen(#s)) > 0) ? (4 - (int)strlen(#s)) : 0,"",s)
#define PRNT_VSTR(s) \
 fprintf(stderr, "vstr(%s):%zu%*s = %s\n", #s , (s)->len, \
         ((4 - strlen(#s)) > 0) ? (4 - (int)strlen(#s)) : 0,"", \
         vstr_export_cstr_ptr((s), 1, (s)->len))
#define EXIT_FAILED_OK 77

#define TST_B_TST(val, num, tst) ((val) |= (1U<< ((num) - 1)) * (tst))
 /* make sure it isn't FAILED_OK */
#define TST_B_RET(val) (val ? ((1U<<31) | val) : 0)


int main(void)
{
  int ret = 0;
  Vstr_conf *conf1 = NULL;
  Vstr_conf *conf2 = NULL;

  buf[0] = 0; /* make sure the compiler thinks buf is used ... */
  
  if (!vstr_init())
    die();

  if (!setlocale(LC_ALL, "en_US"))
    die();
  
  if (!(conf1 = vstr_make_conf()))
    die();
  if (!(conf2 = vstr_make_conf()))
    die();
  
  if (!vstr_cntl_conf(conf1,
                      VSTR_CNTL_CONF_SET_LOC_CSTR_AUTO_NAME_NUMERIC, "en_US"))
    die();

  vstr_cntl_conf(conf2, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 4);
  
  if (!(s1 = vstr_make_base(NULL)))
    die();
  if (!(s2 = vstr_make_base(conf1)))
    die();
  if (!(s3 = vstr_make_base(conf2)))
    die();

  vstr_free_conf(conf1);
  vstr_free_conf(conf2);
  
  if ((ret = tst()) && (ret != EXIT_FAILED_OK))
    fprintf(stderr, "Error(%s) value = %x\n", rf, ret);

  vstr_free_base(s1);
  vstr_free_base(s2);
  vstr_free_base(s3);
  
  vstr_exit();

  switch (ret)
  {
    case EXIT_FAILED_OK: exit (EXIT_FAILED_OK);
    case EXIT_SUCCESS:   exit (EXIT_SUCCESS);
    default:             exit (EXIT_FAILURE);
  }
}
