
CC=gcc
#CC=gcc3
# CC=diet gcc
#CC=diet gcc3

VSTR_LOCAL_DIR=../j
# VSTR_LOCAL_DIR=../
VSTR_LIBS_DIR=$(VSTR_LOCAL_DIR)/src/.libs

OPTWARNS=-W -Wall -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline -O0 -ggdb3 # -ansi -pedantic

HAVE_TIMER_Q=0
HAVE_SSL=1

LOCAL=1
PROFILE=1
# 1 == Static with libvstr, 2 == really static
STATIC=1
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
EX_CFLAGS=-I$(VSTR_LOCAL_DIR)/include -I../include $(OPTWARNS) $(PROF)
EX_LDFLAGS=$(ST) $(LOCAL_LIBS) $(PROF)
else
EX_CFLAGS=`pkg-config --cflags vstr` $(OPTWARNS) $(PROF)
EX_LDFLAGS=$(ST) `pkg-config --libs vstr` $(PROF)
endif

# Isn't converted to pkg-config yet...
TQ_CFLAGS=`gnome-config --cflags timer_q`
TQ_LDFLAGS=`gnome-config --libs timer_q`

SSL_CFLAGS=
SSL_LDFLAGS=-lssl

ifeq ($(HAVE_TIMER_Q),1)
TIMER_Q_PROGS=ex_slowcat
endif

ifeq ($(HAVE_SSL),1)
SSL_PROGS=ex_sg_compare
endif

FILES= ex_cat ex_hexdump ex_nl ex_rot13 ex_yes ex_mon_cp ex_lookup_ip \
  $(TIMER_Q_PROGS) \
  $(SSL_PROGS)

all: $(FILES)

ex_utils.o: ex_utils.c ex_utils.h
		$(CC) $(EX_CFLAGS) $(CFLAGS) -o ex_utils.o -c ex_utils.c

ex%.o: ex%.c
		$(CC) $(EX_CFLAGS) $(CFLAGS) -o $@ -c $<

ex%: ex%.o ex_utils.o
		$(CC) -o $@ $< ex_utils.o $(EX_LDFLAGS) $(LDFLAGS)

ex_sg_compare: ex_sg_compare.c ex_utils.o
		$(CC) $(EX_CFLAGS) $(SSL_CFLAGS) $(CFLAGS) -o ex_sg_compare.o -c $<
		$(CC) -o $@ ex_sg_compare.o ex_utils.o $(SSL_LDFLAGS) $(EX_LDFLAGS) $(LDFLAGS)

ex_slowcat: ex_slowcat.c ex_utils.o
		$(CC) $(EX_CFLAGS) $(TQ_CFLAGS) $(CFLAGS) -o ex_slowcat.o -c $<
		$(CC) -o $@ ex_slowcat.o ex_utils.o $(TQ_LDFLAGS) $(EX_LDFLAGS)
$(LDFLAGS)

clean:
		rm -f *.o $(FILES)

