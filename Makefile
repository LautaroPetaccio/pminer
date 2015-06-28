CC=gcc
CFLAGS=-c -Wall

all: main tests

tests: sha256_tests queue_tests

clean:
	rm *.o *.ghc tests/*.o

main.o: main.c
	$(CC) $(CFLAGS) main.c

parser.o: parser.h parser.c
	$(CC) $(CFLAGS) parser.h parser.c

util.o: util.h util.c
	$(CC) $(CFLAGS) util.h util.c

sha256.o: sha256.h sha256.c
	$(CC) $(CFLAGS) sha256.h sha256.c

sha256_tests.o: tests/sha256_tests.c
	$(CC) $(CFLAGS) tests/sha256_tests.c -o tests/sha256_tests.o

queue.o: queue.h queue.c
	$(CC) $(CFLAGS) queue.h queue.c

queue_tests.o: tests/queue_tests.c
	$(CC) $(CFLAGS) tests/queue_tests.c -o tests/queue_tests.o

protocol.o: protocol.h protocol.c
	$(CC) $(CFLAGS) protocol.h protocol.c

main: main.o queue.o parser.o protocol.o sha256.o util.o
	$(CC) main.o parser.o protocol.o queue.o sha256.o util.o -o main -ljansson -o main

sha256_tests: tests/sha256_tests.o sha256.o util.o
	$(CC) -lcmocka sha256.o util.o tests/sha256_tests.o -o tests/sha256_tests

queue_tests: tests/queue_tests.o queue.o
	$(CC) -lcmocka queue.o tests/queue_tests.o -o tests/queue_tests