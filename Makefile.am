# See if we can cast the automake spell. -*-Makefile-*-
#

AUTOMAKE_OPTIONS = no-dependencies 1.4

SUBDIRS = src include tst examples Documentation

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = vstr.pc

$(pkgconfig_DATA): config.status

