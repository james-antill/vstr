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

