CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

EXECS=pagerank.out

all: $(EXECS)


%.out: %.o xerrori.o utilities.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c xerrori.h utilities.h
	$(CC) $(CFLAGS) -c $<

clean: 
	rm -f *.o $(EXECS)

