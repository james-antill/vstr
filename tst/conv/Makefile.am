# See if we can cast the automake spell. -*-Makefile-*-
#


AUTOMAKE_OPTIONS = no-dependencies 1.4

tst = tst_encode \
    tst_decode

# FIXME
# 	tst_lowercase
# 	tst_uppercase
# 	tst_unprintable_chr
# 	tst_unprintable_del

check_PROGRAMS = $(tst)

TESTS = $(tst)

CFLAGS = @WARNS@
LDADD = ../../src/.libs/libvstr.a

