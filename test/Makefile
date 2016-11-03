CFLAGS += -I../src -std=c99 -Wall -pedantic \
	-Wunused -Wstrict-prototypes -Wmissing-prototypes \
	-Wshadow -Wmaybe-uninitialized -Wtype-limits

LDFLAGS += 

all: testharness perftest libtest
	./testharness

format.o: ../src/format.c
	$(CC) $(CFLAGS) -c $< -o $@

lib.o: ../lib/printf.c
	$(CC) $(CFLAGS) -I../lib -c $< -o $@

libtest.o: libtest.c
	$(CC) $(CFLAGS) -I../lib -c $< -o $@

testharness: testharness.o format.o
	$(CC) $(LDFLAGS) testharness.o format.o -o testharness

perftest: performance.o format.o
	$(CC) $(LDFLAGS) performance.o format.o -o perftest

libtest: libtest.o format.o lib.o
	$(CC) $(LDFLAGS) libtest.o format.o lib.o -o libtest

clean:
	rm -f testharness
	rm -f perftest
	rm -f libtest
	rm -f *.o

what:
	@echo "make targets:"
	@echo "   all         -- all known targets (the default), then runs the test harness"
	@echo "   testharness -- the format test program"
	@echo "   perftest    -- runs some float performance tests"
	@echo "   libtest     -- library tests"
	@echo "   clean       -- deletes all build artifacts"

