CFLAGS = -g -I../src
LDFLAGS = -g

all: testharness
	./testharness

format.o: ../src/format.c
	$(CC) -c $< -o $@
	
testharness: testharness.o format.o
	$(CC) testharness.o format.o -o testharness

clean:
	rm testharness
	rm *.o

