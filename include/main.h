#ifndef MAIN_H
#define MAIN_H

#include "autoconf.h"

#ifdef HAVE_POSIX_HOST
# define USE_MMAP 1
# include "main_system.h"
#else
# include "main_noposix_system.h"
#endif

#define FIX_NAMESPACE_SYMBOL vstr__autoconf_

#ifdef USE_RESTRICTED_HEADERS
# define USE_WIDE_CHAR_T 0
#endif

#include "fix.h"

#include "assert_loop-def.h"
#include "assert_loop-extern.h"

/* vstr includes done by hand... */
#define VSTR__HEADER_H

#include "vstr-conf.h"

#ifndef VSTR_AUTOCONF_HAVE_POSIX_HOST
# undef VSTR_AUTOCONF_HAVE_MMAP
# undef VSTR_AUTOCONF_HAVE_WRITEV
# undef  VSTR_AUTOCONF_mode_t
# define VSTR_AUTOCONF_mode_t int
# undef  VSTR_AUTOCONF_off64_t
# define VSTR_AUTOCONF_off64_t long
#endif

#include "vstr-switch.h"
#include "vstr-const.h"
#include "vstr-def.h"

#include "vstr-internal.h" /* inline done in here */

#include "tools-def.h"
#include "tools-extern.h"

#endif
