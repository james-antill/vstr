# See if we can cast the automake spell. -*-Makefile-*-
#

AUTOMAKE_OPTIONS = no-dependencies 1.4

SUBDIRS = src include Documentation tst

pkgconfigdir := $(libdir)/pkgconfig
pkgconfig_DATA = vstr.pc

$(pkgconfig_DATA): config.status

