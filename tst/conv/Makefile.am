# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_encode \
    tst_decode

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @WARNS@
LDADD = ../../src/.libs/libvstr.a

tst_encode_SOURCES = tst_encode.c
tst_decode_SOURCES = tst_decode.c



