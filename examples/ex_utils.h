#ifndef EX_UTILS_H
#define EX_UTILS_H 1

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#ifndef __GNUC__
# define __PRETTY_FUNCTION__ "(unknown)"
#endif


#define DIE(x, args...)  \
  ex_utils_die(__FILE__, __LINE__, __FUNCTION__, x, ## args)
#define WARN(x, args...) \
  ex_utils_die(__FILE__, __LINE__, __FUNCTION__, x, ## args)

#define assert(x) do { if (!(x)) DIE("Assert=\"" #x "\""); } while (FALSE)

extern void ex_utils_die(const char *, unsigned int, const char *,
                         const char *, ...)
    __attribute__ ((__format__ (__printf__, 4, 5)));
extern void ex_utils_warn(const char *, unsigned int, const char *,
                          const char *, ...)
    __attribute__ ((__format__ (__printf__, 4, 5)));
extern int ex_utils_read(struct Vstr_base *, int);
extern int ex_utils_write(struct Vstr_base *, int);
extern void ex_utils_cpy_write_all(struct Vstr_base *, int);
extern void ex_utils_append_file(struct Vstr_base *, const char *, int, size_t);
extern void ex_utils_check(void);

#endif
