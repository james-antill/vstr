#ifndef ASSERT_LOOP_EXTERN_H
#define ASSERT_LOOP_EXTERN_H

#undef  assert
#undef  ASSERT

#ifdef NDEBUG
#  define assert(x) ((void) 0)
#  define ASSERT(x) ((void) 0)
#else
# ifdef USE_ASSERT_LOOP
extern void vstr__assert_loop(const char *,
                              const char *, int, const char *);
#  define assert(x) do { \
 if (x) {} else \
  vstr__assert_loop(#x, __FILE__, __LINE__, __func__); } \
 while (FALSE)
#  define ASSERT(x) do { \
 if (x) {} else \
  vstr__assert_loop(#x, __FILE__, __LINE__, __func__); } \
 while (FALSE)
# else
#  define assert(x) do { \
 if (x) {} else { \
  fprintf(stderr, " -=> ASSERT (%s) failed in (%s) from %d %s.\n", \
          #x , __func__, __LINE__, __FILE__); \
  abort(); } } \
 while (FALSE)
#  define ASSERT(x) do { \
 if (x) {} else { \
  fprintf(stderr, " -=> ASSERT (%s) failed in (%s) from %d %s.\n", \
          #x , __func__, __LINE__, __FILE__); \
  abort(); } } \
 while (FALSE)
# endif
#endif

#endif
