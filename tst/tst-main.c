
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

#define PRNT_BUF()  fprintf(stderr, "buf  = %s\n", buf);
#define PRNT_VSTR(s) fprintf(stderr, "vstr(%s):%zu = %s\n", #s , (s)->len, \
                             vstr_export_cstr_ptr((s), 1, (s)->len))

int main(void)
{
  int ret = 0;
  Vstr_conf *conf1 = NULL;
  Vstr_conf *conf2 = NULL;
  
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
  
  if ((ret = tst()))
    fprintf(stderr, "Error(%s) value = %d\n", rf, ret);

  vstr_free_base(s1);
  vstr_free_base(s2);
  
  vstr_exit();
  
  exit (ret);
}
