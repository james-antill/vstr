# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_ip \
      tst_int \
      tst_netstr \
      tst_netstr2

# FIXME
#   tst_short
#   tst_long
#   tst_intmax_t

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @CFLAGS@ @WARNS@
LDADD = ../../src/.libs/libvstr.a
