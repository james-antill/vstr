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
    tst_ptrdiff_t \
    tst_size_t \
    tst_m \
    tst_n \
    tst_str_norm \
    tst_num \
    tst_bkmg \
    tst_ip \
    tst_usr

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = -I../../../include -I- @CFLAGS@ @WARNS@
LDADD = -L../../src/ -lvstr

