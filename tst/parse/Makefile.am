# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_ip \
      tst_num \
      tst_netstr \
      tst_netstr2

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @CFLAGS@ @WARNS@
LDADD = ../../src/.libs/libvstr.a
