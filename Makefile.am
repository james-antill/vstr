# See if we can cast the automake spell. -*-Makefile-*-
#

AUTOMAKE_OPTIONS = no-dependencies 1.4

SUBDIRS = libtool_hack src include Documentation

INST_GNOME_CONFIG_HACK=@INST_GNOME_CONFIG_HACK@

pkgconfigdir := $(libdir)/pkgconfig
pkgconfig_DATA = vstr.pc

$(pkgconfig_DATA): config.status

