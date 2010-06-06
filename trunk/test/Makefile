CFLAGS = -g -I../src -Wall -pedantic -Wunused 
LDFLAGS = -g

all: testharness
	./testharness

format.o: ../src/format.c
	$(CC) $(CFLAGS) -c $< -o $@
	
testharness: testharness.o format.o
	$(CC) $(CFLAGS) testharness.o format.o -o testharness

clean:
	rm testharness
	rm *.o

