CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

EXECS=pagerank
OBJS=pagerank.o xerrori.o utilities_pagerank.o helpers.o

all: $(EXECS)

$(EXECS): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c xerrori.h utilities_pagerank.h helpers.h
	$(CC) $(CFLAGS) -c $<

clean: 
	rm -f *.o $(EXECS)
