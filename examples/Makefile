CC=gcc
WARNS=-W -Wall -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline -O0 -ggdb3 # -ansi -pedantic

LOCAL=0

# PROF=-pg

ifeq ($(LOCAL),1)
CFLAGS=-I../include $(WARNS) $(PROF)
LDFLAGS=../src/.libs/libvstr.a $(PROF)
else
CFLAGS=`pkg-config --cflags vstr` -O2 $(PROF)
LDFLAGS=`pkg-config --libs vstr` $(PROF)
endif

# Isn't converted to pkg-config yet...
TQ_CFLAGS=`gnome-config --cflags timer_q`
TQ_LDFLAGS=`gnome-config --libs timer_q`

FILES= ex_cat ex_hexdump ex_nl ex_slowcat # ex_tst1

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
