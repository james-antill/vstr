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

#define SHOW() \
 "File: %s\n" \
 "Line: %d\n" \
 "Function: %s\n" \
 "Problem: "



#define DIE(x) ex_utils_die(SHOW() x , __FILE__, __LINE__, __PRETTY_FUNCTION__)

#define assert(x) do { if (!(x)) DIE("Assert=\"" #x "\""); } while (FALSE)

extern void ex_utils_die(const char *, ...)
    __attribute__ ((__format__ (__printf__, 1, 2)));
extern int ex_utils_read(struct Vstr_base *, int);
extern int ex_utils_write(struct Vstr_base *, int);
extern void ex_utils_cpy_write_all(struct Vstr_base *, int);
extern void ex_utils_append_file(struct Vstr_base *, const char *, int, size_t);
extern void ex_utils_check(void);

#endif
