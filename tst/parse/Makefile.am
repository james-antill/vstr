# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_ip tst_int

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @WARNS@
LDADD = ../../src/.libs/libvstr.a

tst_ip_SOURCES = tst_ip.c
tst_int_SOURCES = tst_int.c

