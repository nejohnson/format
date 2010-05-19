CFLAGS = -g
LDFLAGS = -g


all: testharness
	./testharness
    
testharness: testharness.o format.o
	$(CC) testharness.o format.o -o testharness

