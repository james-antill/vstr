# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_add \
    tst_del \
    tst_srch \
    tst_foreach \
    tst_join \
    tst_update


check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = -I../../../include -I- @CFLAGS@ @WARNS@
LDADD = -L../../src/ -lvstr

