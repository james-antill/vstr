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

#include "vstr.h"
#include "vstr-internal.h"

#include "tools-def.h"
#include "tools-extern.h"

#endif
