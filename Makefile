CC=gcc
CFLAGS=-c -Wall

all: protocol tests

tests: sha256_tests queue_tests

clean:
	rm *.o *.ghc tests/*.o

sha256.o: sha256.h sha256.c
	$(CC) $(CFLAGS) sha256.h sha256.c

sha256_tests.o: tests/sha256_tests.c
	$(CC) $(CFLAGS) tests/sha256_tests.c -o tests/sha256_tests.o

sha256_tests: tests/sha256_tests.o sha256.o
	$(CC) -lcmocka sha256.o tests/sha256_tests.o -o tests/sha256_tests

queue.o: queue.h queue.c
	$(CC) $(CFLAGS) queue.h queue.c

queue_tests.o: tests/queue_tests.c
	$(CC) $(CFLAGS) tests/queue_tests.c -o tests/queue_tests.o

queue_tests: tests/queue_tests.o queue.o
	$(CC) -lcmocka queue.o tests/queue_tests.o -o tests/queue_tests

protocol.o: protocol.c
	$(CC) $(CFLAGS) protocol.c

protocol: protocol.o queue.o sha256.o
	$(CC) protocol.o queue.o sha256.o -ljansson -o protocol