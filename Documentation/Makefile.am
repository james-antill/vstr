confexecdir := $(datadir)/doc/vstr-devel-$(VERSION)/
confexec_DATA =                         \
                functions.txt \
                functions.html \
                functions.3 \
                constants.txt \
                constants.html \
                namespace.txt \
                namespace.html \
                overview.txt \
                overview.html \
                structs.txt \
                structs.html \
		size_cmp.gnumeric

man_MANS = \
                vstr.3

EXTRA_DIST = $(man_MANS)

vstr.3: functions.3
		cp -a functions.3 vstr.3
