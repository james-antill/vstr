# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_vstr \
    tst_vstr_case \
    tst_vstr_vers \
    tst_vstr_non

# FIXME:
#
# tst_buf
# tst_case_buf
# tst_non_buf
# tst_case_non_buf
#
#

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @CFLAGS@ @WARNS@
LDADD = ../../src/.libs/libvstr.a

