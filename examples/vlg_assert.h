#ifndef VLG_ASSERT_H
#define VLG_ASSERT_H

#ifndef VSTR_AUTOCONF_NDEBUG
# define ASSERT(x) do { if (x) {} else vlg_abort(vlg, "ASSERT(" #x "), FAILED at %s:%u\n", __FILE__, __LINE__); } while (FALSE)
# define assert(x) do { if (x) {} else vlg_abort(vlg, "assert(" #x "), FAILED at %s:%u\n", __FILE__, __LINE__); } while (FALSE)
#else
# define ASSERT(x)
# define assert(x)
#endif
#define ASSERT_NOT_REACHED() assert(FALSE)

#endif
