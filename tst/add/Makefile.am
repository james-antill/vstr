# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_buf \
    tst_ptr \
    tst_ref \
    tst_vstr \
    tst_non \
    tst_iovec_buf \
    tst_netstr \
    tst_netstr2


check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @WARNS@
LDADD = ../../src/.libs/libvstr.a

