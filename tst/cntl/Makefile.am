# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_conf \
      tst_base \
      tst_opt


check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = -I../../../include @CFLAGS@ @WARNS@
LDADD = -L../../src/ -lvstr


