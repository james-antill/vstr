#ifndef ASSERT_LOOP_EXTERN_H
#define ASSERT_LOOP_EXTERN_H

#ifdef USE_ASSERT_LOOP
# ifdef NDEBUG
#  define assert(x) ((void) 0)
# else
extern void vstr__assert_loop(const char *,
                              const char *, int, const char *);
#  define assert(x) do { \
 if (!(x)) \
  vstr__assert_loop(#x, __FILE__, __LINE__, __PRETTY_FUNCTION__); } while (FALSE)
# endif
#endif

#endif
