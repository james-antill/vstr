CC=gcc-3.0
WARNS=-W -Wall -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline -O0 -ggdb3

CFLAGS=`gnome-config --cflags vstr` $(WARNS)
LDFLAGS=`gnome-config --libs vstr`

TQ_CFLAGS=`gnome-config --cflags timer_q`
TQ_LDFLAGS=`gnome-config --libs timer_q`

FILES=ex1 ex2 ex3 ex4 ex5 ex6 ex_cat ex_slowcat

all: $(FILES)


ex%: ex%.c
		$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

ex_slowcat: ex_slowcat.c
		-$(CC) $(TQ_CFLAGS) $(CFLAGS) -o $@ $< $(TQ_LDFLAGS) $(LDFLAGS) 

clean:
		rm -f *.o $(FILES)
