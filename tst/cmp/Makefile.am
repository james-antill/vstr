# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst \
    tst_case \
    tst_vers \
    tst_non

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = -I../../../include -I-  @CFLAGS@ @WARNS@
LDADD = ../../src/.libs/libvstr.a

