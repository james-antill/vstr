# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_ipv4 \
      tst_ipv6 \
      tst_num \
      tst_netstr \
      tst_netstr2

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = -I../../../include -I- @CFLAGS@ @WARNS@
LDADD = -L../../src/ -lvstr
