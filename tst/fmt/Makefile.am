# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_char \
    tst_short \
    tst_int \
    tst_long \
    tst_intmax_t \
    tst_double \
    tst_long_double \
    tst_str_norm \
    tst_num 

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @WARNS@
LDADD = ../../src/.libs/libvstr.a

tst_char_SOURCES = tst_char.c
tst_short_SOURCES = tst_short.c
tst_int_SOURCES = tst_int.c
tst_long_SOURCES = tst_long.c
tst_intmax_t_SOURCES = tst_intmax_t.c
tst_double_SOURCES = tst_double.c
tst_long_double_SOURCES = tst_long_double.c
tst_str_norm_SOURCES = tst_str_norm.c
tst_num_SOURCES = tst_num.c


