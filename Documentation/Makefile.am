docmaindir = $(datadir)/doc/vstr-devel-$(VERSION)
docmain_DATA =                         \
                basic_cat.c.html \
                comparison.html \
                constants.txt \
                constants.html \
                design.html \
                f_c.css \
                functions.txt \
                functions.html \
		internal_structure.dia \
                structs.txt \
                structs.html \
                namespace.html \
                overview.html \
                printf_comparison.html \
                req_trunc_io.html \
                security.html \
                speed.html \
                security_problems.html \
		size_cmp.gnumeric \
                cost_tutorial.html \
                tutorial.html

doclicdir := $(datadir)/doc/vstr-devel-$(VERSION)/compare_licenses/
doclic_DATA =                         \
                compare_licenses/SafeStr.txt \
                compare_licenses/bstring.txt \
                compare_licenses/cords.txt \
                compare_licenses/ossp-str.txt \
                compare_licenses/rope.txt \
                compare_licenses/sz.txt \
                compare_licenses/toolbox.txt


man_MANS = \
                vstr.3 \
                vstr_const.3

EXTRA_DIST = $(man_MANS)

CLEANFILES = $(man_MANS)

TUT_SSI_FILES= \
  ../examples/ex_hello_world.c \
  ../examples/ex_hello_world.h \
  ../examples/ex_hello_world_s.c \
  ../examples/ex_hello_world_m.c \
  ../examples/ex_hello_world_x.c \
  ../examples/ex_utils.h \
  ../examples/ex_cat.c \
  ../examples/ex_nl.c \
  ../examples/ex_hexdump.c \
  ../examples/ex_lookup_ip.c \
  tut/mpz1.c \
  tut/mpz2.c \
  tut/mpz3.c \
  tut/mpz4.c \
  ../examples/ex_gmp_factorials.c \
  ../examples/ex_gmp_nums.c

COST_TUT_SSI_FILES= \
  cost_tut/add_buf_add_ptr.c \
  cost_tut/pos_add_buf.c \
  cost_tut/add_buf_sub_buf.c \
  cost_tut/add_buf_sub_ptr.c \
  cost_tut/add_ptr_sub_ptr.c \
  cost_tut/iter_mov_add_del.c

SEC_SSI_FILES= \
  sec/alloc1.c \
  sec/alloc2.c \
  sec/alloc3.c \
  sec/alloc4.c \
  sec/alloc5.c \
  sec/sprintf1.c \
  sec/sprintf2.c \
  sec/strcpy1.c \
  sec/strncpy1.c \
  sec/strncpy2.c \
  sec/strncpy3.c \
  sec/strncpy4.c \
  sec/strncpy5.c \
  sec/strncpy6.c \
  sec/strncpy7.c \
  sec/asprintf1.c \
  sec/snprintf1.c \
  sec/snprintf2.c

security_problems.html: security_problems.txt sec_txt2html.pl
		sec_txt2html.pl

functions.html: functions.txt txt2html.pl
		txt2html.pl

constants.html: constants.txt txt2html.pl
		txt2html.pl

structs.html: structs.txt txt2html.pl
		txt2html.pl

functions.3: functions.txt txt2man.pl
		txt2man.pl

constants.3: constants.txt txt2man.pl
		txt2man.pl

# FIXME: change to use examples versions...
# at which point they'll be generated every build...
tutorial.html: tutorial.shtml $(TUT_SSI_FILES)
		-@$(LN_S) $(top_srcdir)/Documentation/f_c.css f_c.css
		jssi $< > $@

cost_tutorial.html: cost_tutorial.shtml $(COST_TUT_SSI_FILES)
		-@$(LN_S) $(top_srcdir)/Documentation/f_c.css f_c.css
		jssi $< > $@

security.html: security.shtml $(SEC_SSI_FILES)
		-@$(LN_S) $(top_srcdir)/Documentation/f_c.css f_c.css
		jssi $< > $@

vstr.3: ./functions.3
		cp -p $< $@

vstr_const.3: ./constants.3
		cp -p $< $@
