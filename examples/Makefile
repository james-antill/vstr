
CC=gcc
#CC=gcc3
# CC=diet gcc
#CC=diet gcc3

VSTR_LOCAL_DIR=../
# VSTR_LOCAL_DIR=../j
VSTR_LOCAL_DIR=../dbg
VSTR_LIBS_DIR=$(VSTR_LOCAL_DIR)/src/.libs

OPTWARNS=-W -Wall -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline -Wno-format-zero-length -Wformat-nonliteral -Wformat-security -O1 -ggdb3 # -ansi -pedantic

HAVE_TIMER_Q=1
HAVE_SSL=1
HAVE_LIGZ=1
HAVE_LIBBZ2=1
HAVE_GMP=1

LOCAL=1
PROFILE=0
# 1 == Static with libvstr, 2 == really static
STATIC=0
FAST=0

ifeq ($(PROFILE),1)
PROF=-pg
endif

# Do these hacks so that valgrind is happy with a dynamic glibc
LOCAL_LIBS=-L$(VSTR_LIBS_DIR) -lvstr
ifeq ($(STATIC),1)
 ifeq ($(LOCAL),1)
LOCAL_LIBS=$(VSTR_LIBS_DIR)/libvstr.a
 else
ST=-static
 endif
endif
ifeq ($(STATIC),2)
ST=-static
endif

ifeq ($(FAST),1)
OPTWARNS=-O2 -mcpu=i686 -fexpensive-optimizations
endif

ifeq ($(LOCAL),1)
EX_CFLAGS=-I$(VSTR_LOCAL_DIR)/include -I../include $(OPTWARNS) $(PROF) $(CFLAGS)
EX_LDFLAGS=$(ST) $(LOCAL_LIBS) $(PROF) $(LDFLAGS)
else
EX_CFLAGS=`pkg-config --cflags vstr` $(OPTWARNS) $(PROF) $(CFLAGS)
EX_LDFLAGS=$(ST) `pkg-config --libs vstr` $(PROF) $(LDFLAGS)
endif

TQ_CFLAGS=`pkg-config --cflags timer_q` $(EX_CFLAGS)
TQ_LDFLAGS=`pkg-config --libs timer_q` $(EX_LDFLAGS)

GLIB_CFLAGS=`pkg-config --cflags glib-2.0` $(EX_CFLAGS)
GLIB_LDFLAGS=`pkg-config --libs glib-2.0` $(EX_LDFLAGS)

SSL_CFLAGS=$(EX_CFLAGS)
SSL_LDFLAGS=-lcrypto $(EX_LDFLAGS)

ZCAT_CFLAGS=$(EX_CFLAGS)
ZCAT_LDFLAGS=-lz -lbz2 $(EX_LDFLAGS)

GMP_CFLAGS=$(EX_CFLAGS)
GMP_LDFLAGS=-lgmp $(EX_LDFLAGS)

ifeq ($(HAVE_TIMER_Q),1)
TIMER_Q_APPS=ex_slowcat
endif

ifeq ($(HAVE_SSL),1)
SSL_APPS=ex_perf_sgopenssl
endif

ifeq ($(HAVE_LIGZ),1)
ifeq ($(HAVE_LIBBZ2),1)
ZCAT_APPS=ex_zcat
endif
endif

ifeq ($(HAVE_GMP),1)
GMP_APPS= ex_gmp_factorials ex_gmp_nums ex_gmp_num_roman ex_gmp_num_words
endif

NORM_APPS = \
  ex_cat \
  ex_hello_world ex_hello_world_s ex_hello_world_m ex_hello_world_x \
  ex_hexdump ex_nl ex_rot13 ex_yes ex_mon_cp ex_lookup_ip \
  ex_csv ex_highlight ex_ssi ex_netstr ex_rainbow ex_changelog_diff \
  ex_perf_nodesize \
  ex_perf_nodesize_foreach

APPS = \
  $(NORM_APPS) \
  $(GMP_APPS) \
  $(ZCAT_APPS) \
  $(TIMER_Q_APPS) \
  $(SSL_APPS)

HTML = \
  $(patsubst %.c,html/%.c.html,$(wildcard *.c)) \
  $(patsubst %.h,html/%.h.html,$(wildcard *.h))

all: $(APPS) $(HTML)

html/ex%.html: ex% ex_highlight
		@echo "Converting to HTML $<"
		@./ex_highlight --cssfile=f_c.css $< > $@

# Custom rules for these due to different cflags/ldflags
ex_zcat: ex_zcat.c
		@echo "Compiling (Z) $<"
		@$(CC) $(ZCAT_CFLAGS) -o $@.o -c $<
		@echo "Linking (Z) $@"
		@$(CC) -o $@ $@.o $(ZCAT_LDFLAGS)

ex_perf_nodesize_foreach: ex_perf_nodesize_foreach.c ex_perf.h
		@echo "Compiling (GLIB) $<"
		@$(CC) $(GLIB_CFLAGS) -o $@.o -c $<
		@echo "Linking (GLIB) $@"
		@$(CC) -o $@ $@.o $(GLIB_LDFLAGS)

ex_perf_sgopenssl: ex_perf_sgopenssl.c ex_perf.h
		@echo "Compiling (SSL) $<"
		@$(CC) $(SSL_CFLAGS) -o $@.o -c $<
		@echo "Linking (SSL) $@"
		@$(CC) -o $@ $@.o $(SSL_LDFLAGS)

ex_slowcat: ex_slowcat.c
		@echo "Compiling (TQ) $<"
		@$(CC) $(TQ_CFLAGS) -o $@.o -c $<
		@echo "Linking (TQ) $@"
		@$(CC) -o $@ $@.o $(TQ_LDFLAGS)

ex_gmp_%.o: ex_gmp_%.c
		@echo "Compiling (GMP) $<"
		@$(CC) $(GMP_CFLAGS) -o $@ -c $<

ex_gmp_%: ex_gmp_%.o
		@echo "Linking (GMP) $@"
		@$(CC) -o $@ $< $(GMP_LDFLAGS)
		@echo ""

# Default compile...
ex%.o: ex%.c
		@echo "Compiling $<"
		@$(CC) $(EX_CFLAGS) -o $@ -c $<

ex%: ex%.o
		@echo "Linking $@"
		@$(CC) -o $@ $< $(EX_LDFLAGS)
		@echo ""

clean:
		rm -f *.o $(APPS) gmon.out

