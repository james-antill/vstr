
CC=gcc
#CC=gcc3
# CC=diet gcc
#CC=diet gcc3

VSTR_LOCAL_DIR=../jwrap
# VSTR_LOCAL_DIR=../
VSTR_LIBS_DIR=$(VSTR_LOCAL_DIR)/src/.libs

OPTWARNS=-W -Wall -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline -O0 -ggdb3 # -ansi -pedantic

LOCAL=1
PROFILE=0
# 1 == Static with libvstr, 2 == really static
STATIC=1
FAST=1

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
CFLAGS=-I$(VSTR_LOCAL_DIR)/include -I../include $(OPTWARNS) $(PROF)
LDFLAGS=$(ST) $(LOCAL_LIBS) $(PROF)
else
CFLAGS=`pkg-config --cflags vstr` $(OPTWARNS) $(PROF)
LDFLAGS=$(ST) `pkg-config --libs vstr` $(PROF)
endif

# Isn't converted to pkg-config yet...
TQ_CFLAGS=`gnome-config --cflags timer_q`
TQ_LDFLAGS=`gnome-config --libs timer_q`

FILES= ex_cat ex_hexdump ex_nl ex_mon_cp ex_lookup_ip ex_slowcat # ex_tst1

all: $(FILES)

ex_utils.o: ex_utils.c ex_utils.h
		$(CC) $(CFLAGS) -o ex_utils.o -c ex_utils.c

ex%.o: ex%.c
		$(CC) $(CFLAGS) -o $@ -c $<

ex%: ex%.o ex_utils.o
		$(CC) -o $@ $< ex_utils.o $(LDFLAGS)

ex_slowcat: ex_slowcat.c ex_utils.o
		$(CC) $(CFLAGS) $(TQ_CFLAGS) -o ex_slowcat.o -c $<
		$(CC) -o $@ ex_slowcat.o ex_utils.o $(TQ_LDFLAGS) $(LDFLAGS)

clean:
		rm -f *.o $(FILES)
