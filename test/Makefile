CFLAGS = -g -I../src -std=c99 -Wall -pedantic -Wunused
#CFLAGS = -O2 -I../src -std=c99 -Wall -pedantic -Wunused  
LDFLAGS = -g

all: testharness perftest
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
	rm -f *.o

what:
	@echo "make targets:"
	@echo "   all         -- all known targets (the default), then runs the test harness"
	@echo "   testharness -- the format test program"
	@echo "   perftest    -- runs some float performance tests"
	@echo "   clean       -- deletes all build artifacts"

