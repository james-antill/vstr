#ifndef MAIN_H
#define MAIN_H

#include "autoconf.h"

#ifdef HAVE_POSIX_HOST
# define USE_MMAP 1
# include "main_system.h"
#else
# include "main_noposix_system.h"
#endif

#define FIX_NAMESPACE_SYMBOL vstr_autoconf_

#include "fix.h"

#include "assert_loop-def.h"
#include "assert_loop-extern.h"

/* vstr includes done by hand... */
#define VSTR__HEADER_H

#include "vstr-conf.h"
#include "vstr-switch.h"
#include "vstr-const.h"
#include "vstr-def.h"
#include "vstr-extern.h"

#include "vstr-internal.h" /* inline done in here */

#include "tools-def.h"
#include "tools-extern.h"

#endif
