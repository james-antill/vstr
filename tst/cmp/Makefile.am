# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_vstr \
    tst_vstr_case \
    tst_vstr_vers \
    tst_vstr_non

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @WARNS@
LDADD = ../../src/.libs/libvstr.a

tst_vstr_SOURCES = tst_vstr.c
tst_vstr_case_SOURCES = tst_vstr_case.c
tst_vstr_vers_SOURCES = tst_vstr_vers.c
tst_vstr_non_SOURCES = tst_vstr_non.c
