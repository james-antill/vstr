# See if we can cast the automake spell. -*-Makefile-*-
#

#
AUTOMAKE_OPTIONS = no-dependencies 1.4

EX_B_DIR=$(top_builddir)/examples
EX_INST_DIR=$(datadir)/doc/vstr-devel-$(VERSION)/

GEN_CFLAGS = -I- -I../include -I../../include @CFLAGS@ @WARNS@
GEN_LDLIBS = @LOCAL_EX_PROG_LIBS@


ex_cat_DEPENDENCIES = ex_utils.h
ex_cat_CFLAGS = $(GEN_CFLAGS)
ex_cat_LDADD  = $(GEN_LDLIBS)
ex_ccount_DEPENDENCIES = ex_utils.h
ex_ccount_CFLAGS = $(GEN_CFLAGS)
ex_ccount_LDADD  = $(GEN_LDLIBS)
ex_hello_world_DEPENDENCIES = ex_utils.h
ex_hello_world_CFLAGS = $(GEN_CFLAGS)
ex_hello_world_LDADD  = $(GEN_LDLIBS)
ex_hello_world_s_DEPENDENCIES = ex_utils.h
ex_hello_world_s_CFLAGS = $(GEN_CFLAGS)
ex_hello_world_s_LDADD  = $(GEN_LDLIBS)
ex_hello_world_m_DEPENDENCIES = ex_utils.h
ex_hello_world_m_CFLAGS = $(GEN_CFLAGS)
ex_hello_world_m_LDADD  = $(GEN_LDLIBS)
ex_hello_world_x_DEPENDENCIES = ex_utils.h
ex_hello_world_x_CFLAGS = $(GEN_CFLAGS)
ex_hello_world_x_LDADD  = $(GEN_LDLIBS)
ex_hexdump_DEPENDENCIES = ex_utils.h
ex_hexdump_CFLAGS = $(GEN_CFLAGS)
ex_hexdump_LDADD  = $(GEN_LDLIBS)
ex_nl_DEPENDENCIES = ex_utils.h
ex_nl_CFLAGS = $(GEN_CFLAGS)
ex_nl_LDADD  = $(GEN_LDLIBS)
ex_rot13_DEPENDENCIES = ex_utils.h
ex_rot13_CFLAGS = $(GEN_CFLAGS)
ex_rot13_LDADD  = $(GEN_LDLIBS)
ex_yes_DEPENDENCIES = ex_utils.h
ex_yes_CFLAGS = $(GEN_CFLAGS)
ex_yes_LDADD  = $(GEN_LDLIBS)
ex_mon_cp_DEPENDENCIES = ex_utils.h
ex_mon_cp_CFLAGS = $(GEN_CFLAGS)
ex_mon_cp_LDADD  = $(GEN_LDLIBS)
ex_lookup_ip_DEPENDENCIES = ex_utils.h
ex_lookup_ip_CFLAGS = $(GEN_CFLAGS)
ex_lookup_ip_LDADD  = $(GEN_LDLIBS)
ex_csv_DEPENDENCIES = ex_utils.h
ex_csv_CFLAGS = $(GEN_CFLAGS)
ex_csv_LDADD  = $(GEN_LDLIBS)
ex_highlight_DEPENDENCIES = ex_utils.h
ex_highlight_CFLAGS = $(GEN_CFLAGS)
ex_highlight_LDADD  = $(GEN_LDLIBS)
ex_ssi_DEPENDENCIES = ex_utils.h
ex_ssi_CFLAGS = $(GEN_CFLAGS)
ex_ssi_LDADD  = $(GEN_LDLIBS)
ex_rainbow_DEPENDENCIES = ex_utils.h
ex_rainbow_CFLAGS = $(GEN_CFLAGS)
ex_rainbow_LDADD  = $(GEN_LDLIBS)
ex_changelog_diff_DEPENDENCIES = ex_utils.h
ex_changelog_diff_CFLAGS = $(GEN_CFLAGS)
ex_changelog_diff_LDADD  = $(GEN_LDLIBS)

ex_slowcat_DEPENDENCIES = ex_utils.h
ex_slowcat_CFLAGS   = $(GEN_CFLAGS) $(TIMER_Q_CFLAGS)
ex_slowcat_LDADD    = $(GEN_LDLIBS) $(TIMER_Q_LDLIBS)

ex_dns_SOURCES = ex_dns.c dns.c
ex_dns_DEPENDENCIES = dns.h
ex_dns_CFLAGS   = $(GEN_CFLAGS) $(SOCKET_POLL_CFLAGS) $(TIMER_Q_CFLAGS)
ex_dns_LDADD    = $(GEN_LDLIBS) $(SOCKET_POLL_LDLIBS) $(TIMER_Q_LDLIBS)

ex_tdns_SOURCES = ex_tdns.c dns.c
ex_tdns_DEPENDENCIES = dns.h
ex_tdns_CFLAGS   = -pthread $(ex_dns_CFLAGS)
ex_tdns_LDADD    = $(ex_dns_LDADD)
ex_tdns_LDFLAGS  = -pthread

ex_zcat_DEPENDENCIES = ex_utils.h
ex_zcat_CFLAGS = $(GEN_CFLAGS) $(ZLIB_CFLAGS) $(BZ2_CFLAGS)
ex_zcat_LDADD  = $(GEN_LDLIBS) $(ZLIB_LDLIBS) $(BZ2_LDLIBS)

ex_perf_sgopenssl_DEPENDENCIES = ex_perf.h
ex_perf_sgopenssl_CFLAGS = $(GEN_CFLAGS) $(OPENSSL_CFLAGS)
ex_perf_sgopenssl_LDADD  = $(GEN_LDLIBS) $(OPENSSL_LDLIBS)

ex_gmp_factorials_DEPENDENCIES = ex_utils.h
ex_gmp_factorials_CFLAGS = $(GEN_CFLAGS) $(GMP_CFLAGS)
ex_gmp_factorials_LDADD  = $(GEN_LDLIBS) $(GMP_LDLIBS)
ex_gmp_nums_DEPENDENCIES = ex_utils.h
ex_gmp_nums_CFLAGS = $(GEN_CFLAGS) $(GMP_CFLAGS)
ex_gmp_nums_LDADD  = $(GEN_LDLIBS) $(GMP_LDLIBS)
ex_gmp_num_roman_DEPENDENCIES = ex_utils.h
ex_gmp_num_roman_CFLAGS = $(GEN_CFLAGS) $(GMP_CFLAGS)
ex_gmp_num_roman_LDADD  = $(GEN_LDLIBS) $(GMP_LDLIBS)
ex_gmp_num_words_DEPENDENCIES = ex_utils.h
ex_gmp_num_words_CFLAGS = $(GEN_CFLAGS) $(GMP_CFLAGS)
ex_gmp_num_words_LDADD  = $(GEN_LDLIBS) $(GMP_LDLIBS)

ex_perf_nodesize_foreach_DEPENDENCIES = ex_perf.h
ex_perf_nodesize_foreach_CFLAGS = $(GEN_CFLAGS) $(GLIB20_CFLAGS)
ex_perf_nodesize_foreach_LDADD  = $(GEN_LDLIBS) $(GLIB20_LDLIBS)

ex_netstr_DEPENDENCIES = ex_perf.h
ex_netstr_CFLAGS = $(GEN_CFLAGS)
ex_netstr_LDADD  = $(GEN_LDLIBS)
ex_perf_nodesize_DEPENDENCIES = ex_perf.h
ex_perf_nodesize_CFLAGS = $(GEN_CFLAGS)
ex_perf_nodesize_LDADD  = $(GEN_LDLIBS)
ex_perf_add_DEPENDENCIES = ex_perf.h
ex_perf_add_CFLAGS = $(GEN_CFLAGS)
ex_perf_add_LDADD  = $(GEN_LDLIBS)


# Turn on Programs we can compile...

if HAVE_POSIX_HOST

hi_DEP =   ex_highlight
hi_APP = ./ex_highlight


norm_APPS = \
  ex_cat \
  ex_hello_world ex_hello_world_s ex_hello_world_m ex_hello_world_x \
  ex_hexdump ex_nl ex_rot13 ex_yes ex_mon_cp ex_lookup_ip \
  ex_csv $(hi_DEP) ex_ssi ex_rainbow ex_changelog_diff ex_ccount

if TIMER_Q_AVAIL
timer_q_APPS = ex_slowcat

if SOCKET_POLL_AVAIL
dns_APP = ex_dns ex_tdns
endif
# Timer_q
endif

if BZ2_AVAIL
if ZLIB_AVAIL
zcat_APPS = ex_zcat
endif
endif

# !HAVE_POSIX_HOST
else

# Use pre-installed version...
hi_DEP =
hi_APP = jhighlight

# HAVE_POSIX_HOST
endif

if OPENSSL_AVAIL
openssl_APPS = ex_perf_sgopenssl
endif

if GMP_AVAIL
gmp_APPS = ex_gmp_factorials ex_gmp_nums ex_gmp_num_roman ex_gmp_num_words
endif

if GLIB20_AVAIL
glib_APPS = ex_perf_nodesize_foreach
endif

examplesdir = $(EX_INST_DIR)
examples_PROGRAMS = \
  ex_netstr ex_perf_nodesize ex_perf_add \
  $(norm_APPS) \
  $(timer_q_APPS) \
  $(zcat_APPS) \
  $(openssl_APPS) \
  $(gmp_APPS) \
  $(glib_APPS) \
  $(dns_APP)

otherdir   = $(EX_INST_DIR)
other_DATA = \
  hexdump_data highlight2style.pl

htmldir   = $(EX_INST_DIR)/html
html_DATA = \
  html/f_c.css \
  html/dns.h.html \
  html/dns.c.html \
  html/ex_cat.c.html \
  html/ex_ccount.c.html \
  html/ex_changelog_diff.c.html \
  html/ex_csv.c.html \
  html/ex_dns.c.html \
  html/ex_gmp_factorials.c.html \
  html/ex_gmp_num_roman.c.html \
  html/ex_gmp_nums.c.html \
  html/ex_gmp_num_words.c.html \
  html/ex_hello_world.c.html \
  html/ex_hello_world.h.html \
  html/ex_hello_world_m.c.html \
  html/ex_hello_world_s.c.html \
  html/ex_hello_world_x.c.html \
  html/ex_hexdump.c.html \
  html/ex_highlight.c.html \
  html/ex_limited_cat.c.html \
  html/ex_lookup_ip.c.html \
  html/ex_mon_cp.c.html \
  html/ex_netstr.c.html \
  html/ex_nl.c.html \
  html/ex_perf_add.c.html \
  html/ex_perf.h.html \
  html/ex_perf_nodesize.c.html \
  html/ex_perf_nodesize_foreach.c.html \
  html/ex_perf_sgopenssl.c.html \
  html/ex_rainbow.c.html \
  html/ex_rot13.c.html \
  html/ex_slowcat.c.html \
  html/ex_ssi.c.html \
  html/ex_tdns.c.html \
  html/ex_utils.h.html \
  html/ex_yes.c.html \
  html/ex_zcat.c.html

CLEANFILES=$(html_DATA) html

html/f_c.css: ../Documentation/f_c.css
		-@mkdir html
		-$(LN_S) ../$< html

html/%.html: % html/f_c.css $(hi_DEP)
		$(hi_APP) --cssfile=f_c.css $< > $@

