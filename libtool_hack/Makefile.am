# See if we can cast the automake spell. -*-Makefile-*-
#

AUTOMAKE_OPTIONS = no-dependencies 1.4

VSTR__VERSION_FUNC=vstr__version_func

# This needs to be done before src/Makefile is run so we need an extra dir.
if HAVE_LINKER_MAIN_RELOCATE
all-local: 
# This is _such_ a disgusting _hack_ ... but there is no other way to do it.
# Go libtool for now way tp specify anything... gee thanks.
	sed -e 's?archive_cmds="\\$$CC -shared?archive_cmds="\\$$CC -Wl,-e,$(VSTR__VERSION_FUNC) -shared?g' \
		< $(top_builddir)/libtool > $(top_builddir)/libtool.tmp \
	&& mv $(top_builddir)/libtool.tmp $(top_builddir)/libtool \
	&& chmod 755 $(top_builddir)/libtool
endif

