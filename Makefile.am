# See if we can cast the automake spell. -*-Makefile-*-
#

AUTOMAKE_OPTIONS = no-dependencies 1.4

SUBDIRS = libtool_hack src include Documentation

INST_GNOME_CONFIG_HACK=@INST_GNOME_CONFIG_HACK@

confexecdir := $(INST_GNOME_CONFIG_HACK)`@GNOME_CONFIG_PATH@ --libdir`
confexec_DATA =                         \
        vstrConf.sh               


EXTRA_DIST =                            \
        vstrConf.sh.in            




vstrConf.sh: vstrConf.sh.in Makefile
## Use sed and then mv to avoid problems if the user interrupts.
# Done here, explicitly, to work around bug of having -L${prefix}/include
# as _LIBDIR
	sed -e 's?\@VSTR_LIBDIR\@?$(VSTR_LIBDIR)?g' \
             -e 's?\@VSTR_INCLUDEDIR\@?$(VSTR_INCLUDEDIR)?g' \
             -e 's?\@VERSION\@?$(VERSION)?g' \
             -e 's?\@VSTR_LIBS\@?$(VSTR_LIBS)?g' \
                < $(srcdir)/vstrConf.sh.in > vstrConf.sh.tmp \
        && mv vstrConf.sh.tmp vstrConf.sh

