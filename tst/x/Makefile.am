# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_typedef \
      tst_inline \
      tst_include \
      tst_misc \
      tst_swap_conf \
      tst_cntl_base \
      tst_cntl_opt


check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = -I../../../include -I- @CFLAGS@ @WARNS@
LDADD = -L../../src/ -lvstr


