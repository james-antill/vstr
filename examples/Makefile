
CC=gcc
#CC=gcc3
# CC=diet gcc
#CC=diet gcc3

VSTR_LOCAL_DIR=../
# VSTR_LOCAL_DIR=../j
VSTR_LIBS_DIR=$(VSTR_LOCAL_DIR)/src/.libs

OPTWARNS=-W -Wall -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline -O0 -ggdb3 # -ansi -pedantic

HAVE_TIMER_Q=0
HAVE_SSL=0
HAVE_LIBZ=1
HAVE_LIBBZ2=1

LOCAL=0
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
OPTWARNS=-O2 -march=i386 -mcpu=i686
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

SSL_CFLAGS=$(EX_CFLAGS)
SSL_LDFLAGS=-lssl $(EX_LDFLAGS)

ZCAT_CFLAGS=$(EX_CFLAGS)
ZCAT_LDFLAGS=-lz -lbz2 $(EX_LDFLAGS)

ifeq ($(HAVE_TIMER_Q),1)
TIMER_Q_PROGS=ex_slowcat
endif

ifeq ($(HAVE_SSL),1)
SSL_PROGS=ex_sg_compare
endif

ifeq ($(HAVE_LIBZ),1)
ifeq ($(HAVE_LIBBZ2),1)
ZCAT=ex_zcat 
endif
endif

FILES= ex_cat ex_hexdump ex_nl ex_rot13 ex_yes ex_mon_cp ex_lookup_ip \
  $(ZCAT)
  $(TIMER_Q_PROGS) \
  $(SSL_PROGS)

all: $(FILES)

ex_utils.o: ex_utils.c ex_utils.h
		$(CC) $(EX_CFLAGS) -o ex_utils.o -c ex_utils.c

ex%.o: ex%.c
		$(CC) $(EX_CFLAGS) -o $@ -c $<

ex%: ex%.o ex_utils.o
		$(CC) -o $@ $< ex_utils.o $(EX_LDFLAGS)

ex_zcat: ex_zcat.c ex_utils.o
		$(CC) $(ZCAT_CFLAGS) -o $@.o -c $<
		$(CC) -o $@ $@.o ex_utils.o $(ZCAT_LDFLAGS)

ex_sg_compare: ex_sg_compare.c ex_utils.o
		$(CC) $(SSL_CFLAGS) -o $@.o -c $<
		$(CC) -o $@ $@.o ex_utils.o $(SSL_LDFLAGS)

ex_slowcat: ex_slowcat.c ex_utils.o
		$(CC) $(TQ_CFLAGS) -o $@.o -c $<
		$(CC) -o $@ $@.o ex_utils.o $(TQ_LDFLAGS)
$(LDFLAGS)

clean:
		rm -f *.o $(FILES) gmon.out

