# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_buf \
      tst_chr \
      tst_case_buf \
      tst_case_chr \
      tst_chrs \
      tst_csrch_chrs \
      tst_vstr \
      tst_case_vstr

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = -I../../../include -I- @CFLAGS@ @WARNS@
LDADD = -L../../src/ -lvstr


