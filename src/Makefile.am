# See if we can cast the automake spell. -*-Makefile-*-
#

#
AUTOMAKE_OPTIONS = no-dependencies 1.4

lib_LTLIBRARIES = libvstr.la

INCLUDES = -I$(top_srcdir)/include

if HAVE_LINKER_MAIN_RELOCATE
CFLAGS = @CFLAGS@ @WARNS@
else
CFLAGS = @CFLAGS@ @WARNS@ -DHAVE_NO_RELOC_MAIN
endif


libvstr_la_SOURCES = assert_loop.c fix.c \
                        vstr.c \
                        vstr_add.c \
                        vstr_add_netstr.c \
                        vstr_cache.c \
                        vstr_cmp.c \
                        vstr_cntl.c \
                        vstr_conv.c \
                        vstr_cstr.c \
                        vstr_del.c \
                        vstr_dup.c \
                        vstr_export.c \
                        vstr_mov.c \
                        vstr_ref.c \
                        vstr_spn.c \
                        vstr_srch.c \
                        vstr_srch_netstr.c \
                        vstr_sub.c \
                        vstr_version.c \
                        vstr_fmt.c

libvstr_la_LDFLAGS = \
        -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
        -release $(LT_RELEASE) \
        -export-dynamic $(VSTR__LINKER_RELOC_MAIN) -lm

