CC=gcc-3.0
WARNS=-W -Wall -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline -O0 -ggdb3 # -ansi -pedantic

CFLAGS=`gnome-config --cflags vstr` $(WARNS)
LDFLAGS=`gnome-config --libs vstr`

TQ_CFLAGS=`gnome-config --cflags timer_q`
TQ_LDFLAGS=`gnome-config --libs timer_q`

FILES=ex1 ex2 ex3 ex4 ex5 ex6 ex7 ex8 ex9 ex10 ex11 \
          ex_test1 ex_test2 \
          ex_cat ex_nl ex_slowcat

all: $(FILES)

ex_utils.o: ex_utils.c ex_utils.h
		$(CC) $(CFLAGS) -o ex_utils.o -c ex_utils.c

ex%.o: ex%.c
		$(CC) $(CFLAGS) -o $@ -c $<

ex%: ex%.o ex_utils.o
		$(CC) -o $@ $< ex_utils.o $(LDFLAGS) 

ex_slowcat: ex_slowcat.c ex_utils.o
		$(CC) $(TQ_CFLAGS) $(CFLAGS) -o ex_slowcat.o -c $<
		$(CC) -o $@ ex_slowcat.o ex_utils.o $(TQ_LDFLAGS) $(LDFLAGS) 

clean:
		rm -f *.o $(FILES)
