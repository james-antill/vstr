# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_cstr_buf \
    tst_cstr_ptr \
    tst_cstr_ref \
    tst_iovec_ptr_all \
    tst_iovec_cpy_buf \
    tst_iovec_cpy_ptr \
    tst_buf \
    tst_chr \
    tst_ref

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @WARNS@
LDADD = ../../src/.libs/libvstr.a

