# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_read \
      tst_write \
      tst_mmap

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @CFLAGS@ @WARNS@
LDADD = ../../src/.libs/libvstr.a

