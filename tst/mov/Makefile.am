# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_empty \
      tst_data

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @CFLAGS@ @WARNS@
LDADD = ../../src/.libs/libvstr.a

