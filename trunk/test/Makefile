CFLAGS = -g -I../src -std=c99 -Wall -pedantic -Wunused
#CFLAGS = -O2 -I../src -std=c99 -Wall -pedantic -Wunused  
LDFLAGS = -g

all: testharness
	./testharness

format.o: ../src/format.c
	$(CC) $(CFLAGS) -c $< -o $@
	
testharness: testharness.o format.o
	$(CC) $(LDFLAGS) testharness.o format.o -o testharness

perftest: performance.o format.o
	$(CC) $(LDFLAGS) performance.o format.o -o perftest

clean:
	rm -f testharness
	rm -f perftest
	rm *.o

