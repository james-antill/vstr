confexecdir := $(datadir)/doc/vstr-devel-$(VERSION)/
confexec_DATA =                         \
                functions.txt \
                functions.html \
                constants.txt \
                constants.html \
                structs.txt \
                structs.html \
                namespace.html \
                design.html \
                overview.html \
                comparison.html \
                printf_comparison.html \
                security.html \
                speed.html \
                req_trunc_io.html \
                security_problems.html \
		size_cmp.gnumeric \
		internal_structure.dia

man_MANS = \
                vstr.3 \
                vstr_const.3

EXTRA_DIST = $(man_MANS)

security_problems.html: security_problems.txt
		$(srcdir)/sec_txt2html.pl

functions.html: functions.txt
		$(srcdir)/txt2html.pl

constants.html: constants.txt
		$(srcdir)/txt2html.pl

structs.html: structs.txt
		$(srcdir)/txt2html.pl

functions.3: functions.txt
		$(srcdir)/txt2man.pl

constants.3: constants.txt
		$(srcdir)/txt2man.pl

vstr.3: functions.3
		for i in functions.3; do \
		 if test -f $(srcdir)/$$i; then file=$(srcdir)/$$i; \
		 else file=$$i; fi; \
		 cp -p $$file vstr.3; \
		done

vstr_const.3: constants.3
		for i in constants.3; do \
		 if test -f $(srcdir)/$$i; then file=$(srcdir)/$$i; \
		 else file=$$i; fi; \
		 cp -p $$file vstr_const.3; \
		done
