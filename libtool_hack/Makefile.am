# See if we can cast the automake spell. -*-Makefile-*-
#

AUTOMAKE_OPTIONS = no-dependencies 1.4

# This needs to be done before src/Makefile is run so we need an extra dir.
if HAVE_LINKER_MAIN_RELOCATE
VSTR__MAIN_RELOCATE_AM=-Wl,-e,vstr_version_func
endif

if HAVE_LINKER_VERSION_SCRIPT
VSTR__VERSION_SCRIPT_AM=-Wl,--version-script=$(top_srcdir)/src/linker-script
endif


VSTR__XTRA_LDFLAGS_AM=$(VSTR__MAIN_RELOCATE_AM) $(VSTR__VERSION_SCRIPT_AM)


all-local-vstr: $(top_builddir)/libtool
# This is _such_ a disgusting _hack_ ... but there is no other way to do it.
# Go libtool for now way tp specify anything... gee thanks.
	sed -e 's?archive_cmds="\\$$CC -shared?archive_cmds="\\$$CC $(VSTR__XTRA_LDFLAGS_AM) -shared?g' \
		< $(top_builddir)/libtool > $(top_builddir)/libtool.tmp \
	&& mv $(top_builddir)/libtool.tmp $(top_builddir)/libtool \
	&& chmod 755 $(top_builddir)/libtool
	touch all-local-vstr

clean-local:
	rm -f all-local-vstr


# This is kind of hacky too, but I think it's the best way to only do the above
# if either variable is set...
if HAVE_LINKER_MAIN_RELOCATE
all-local: all-local-vstr
else
if HAVE_LINKER_VERSION_SCRIPT
all-local: all-local-vstr
endif
endif
