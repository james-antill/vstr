#include "../../include/autoconf.h"

#ifdef USE_RESTRICTED_HEADERS
# define USE_WIDE_CHAR_T 0
#endif

#ifdef HAVE_POSIX_HOST
# define USE_MMAP 1
# include "../../include/main_system.h"
#else
# include "../../include/main_noposix_system.h"
#endif

#include <float.h>

#include "../../include/fix.h"

/* not from system ... just from here */
/* #include "../../vstr.h" -- have to do it manually */

#define VSTR__HEADER_H 1

#include "../../include/vstr-conf.h"

#ifndef VSTR_AUTOCONF_HAVE_POSIX_HOST
/* only undef the things we use externaly */
# undef VSTR_AUTOCONF_HAVE_MMAP
# undef VSTR_AUTOCONF_HAVE_WRITEV
#endif

#include "../../include/vstr-switch.h"

#include "../../include/vstr-const.h"
#include "../../include/vstr-def.h"
#include "../../include/vstr-extern.h"

#if defined(VSTR_AUTOCONF_HAVE_INLINE) && VSTR_COMPILE_INLINE
# include "../../include/vstr-inline.h"
#endif

/* use system assert, so we get abort()s which fail tests */
#undef NDEBUG /* always use assert */
#include <assert.h>
#define ASSERT(x) assert(x)



static int tst(void); /* fwd */

#define TST_BUF_SZ (1024 * 512)

static struct Vstr_base *s1 = NULL; /* normal */
static struct Vstr_base *s2 = NULL; /* locale en_US */
static struct Vstr_base *s3 = NULL; /* buf size = 4 */
static struct Vstr_base *s4 = NULL; /* no cache */
static char buf[TST_BUF_SZ];

static const char *rf;

static void die(void)
{
  fprintf(stderr, "Error(%s) abort\n", rf);
  abort();
}

#define PRNT_CSTR(s) do { \
 int pad = 0; \
 fprintf(stderr, "cstr(%s):%zu%n", #s , strlen(s), &pad); \
 if (pad > 12) pad = 0; else pad = 12 - pad; \
 fprintf(stderr, "%*s = %s\n", pad, "", s); \
 } while (FALSE)
#define PRNT_VSTR(s) do { \
 int pad = 0; \
 fprintf(stderr, "vstr(%s):%zu%n", #s , (s)->len, &pad); \
 if (pad > 12) pad = 0; else pad = 12 - pad; \
 fprintf(stderr, "%*s = %s\n", pad, "", \
         vstr_export_cstr_ptr((s), 1, (s)->len)); \
 } while (FALSE)

#define PRNT_DEBUG_VSTR(s) do { \
  int pad = 0; \
  unsigned int count = 1; \
  struct Vstr_node *scan = (s)->beg; \
  \
  fprintf(stderr, "vstr(%s)%n:%zu NUM=%u\n", #s , &pad, (s)->len, (s)->num); \
  while (scan) \
  { \
    fprintf(stderr, "%*u:%zu", pad, count, scan->len); \
    \
    switch (scan->type) \
    { \
      case VSTR_TYPE_NODE_BUF: \
      fprintf(stderr, " (%s) = %.*s\n", "BUF", (int)scan->len, \
              ((Vstr_node_buf *)scan)->buf); break; \
      case VSTR_TYPE_NODE_NON: \
      fprintf(stderr, " (%s) = %.*s\n", "NON", (int)scan->len, \
              ""); break; \
      case VSTR_TYPE_NODE_PTR: \
      fprintf(stderr, " (%s) = %.*s\n", "PTR", (int)scan->len, \
              (const char *)((Vstr_node_ptr *)scan)->ptr); break; \
      case VSTR_TYPE_NODE_REF: \
      fprintf(stderr, " (%s) = %.*s\n", "REF", (int)scan->len, \
              (const char *)((Vstr_node_ref *)scan)->ref->ptr + \
              ((Vstr_node_ref *)scan)->off); break; \
      default: assert(FALSE); break; \
    } \
    \
    scan = scan->next; \
    ++count; \
  } \
 } while (FALSE)

#define EXIT_FAILED_OK 77

#define TST_B_TST(val, num, tst) ((val) |= (1U<< ((num) - 1)) * (tst))
 /* make sure it isn't FAILED_OK */
#define TST_B_RET(val) (val ? ((1U<<31) | val) : 0)


int main(void)
{
  int ret = 0;
  struct Vstr_conf *conf1 = NULL;
  struct Vstr_conf *conf2 = NULL;
  struct Vstr_conf *conf3 = NULL;

  buf[0] = 0; /* make sure the compiler thinks buf is used ... */
  
  if (!vstr_init())
    die();

#ifndef USE_RESTRICTED_HEADERS
  if (!setlocale(LC_ALL, "en_US"))
  {
    fprintf(stderr,
" This library does things with the locale.\n"
" So to run a \"make check\" the locale en_US needs to be available\n"
" on your system.\n"
"\n"
" You can install the en_US locale on your system by doing:\n"
"\n"
"Debian: \"dpkg-reconfigure locales\" (choose 54 and 55)\n"
"Red Hat:\n"
"  Either choose the en_US locale in the installer,\n"
" or find your glibc-common rpm and run...\n\n"
" . /etc/sysconfig/i18n\n"
" rpm --define \"_install_langs en_US:$SUPPORTED\" -Uvh --force glibc-common*\n"
"\n"
" You can check that it is installed by running \"locale -a\"\n"
"\n"
"\n"
            );
    exit (EXIT_FAILURE);
  }
#endif
  
  if (!(conf1 = vstr_make_conf()))
    die();
  if (!(conf2 = vstr_make_conf()))
    die();
  if (!(conf3 = vstr_make_conf()))
    die();
  
  if (!vstr_cntl_conf(conf1,
                      VSTR_CNTL_CONF_SET_LOC_CSTR_AUTO_NAME_NUMERIC, "en_US"))
    die();

  vstr_cntl_conf(conf2, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, 4);
  
  vstr_cntl_conf(conf3, VSTR_CNTL_CONF_SET_FLAG_NO_ALLOC_CACHE, 1);
  
  if (!(s1 = vstr_make_base(NULL)))
    die();
  if (!(s2 = vstr_make_base(conf1)))
    die();
  if (!(s3 = vstr_make_base(conf2)))
    die();
  if (!(s4 = vstr_make_base(conf3)))
    die();

  vstr_free_conf(conf1);
  vstr_free_conf(conf2);
  vstr_free_conf(conf3);
  
  if ((ret = tst()) && (ret != EXIT_FAILED_OK))
    fprintf(stderr, "Error(%s) value = %x\n", rf, ret);

  vstr_free_base(s1);
  vstr_free_base(s2);
  vstr_free_base(s3);
  vstr_free_base(s4);
  
  vstr_exit();

  switch (ret)
  {
    case EXIT_FAILED_OK: exit (EXIT_FAILED_OK);
    case EXIT_SUCCESS:   exit (EXIT_SUCCESS);
    default:             exit (EXIT_FAILURE);
  }
}
