CC=gcc
CFLAGS=-c -Wall

all: queue.o

clean: clean-protocol

sha256.o: sha256.h sha256.c
	$(CC) $(CFLAGS) sha256.h sha256.c

sha256_tests.o: sha256_tests.c
	$(CC) $(CFLAGS) sha256_tests.c

sha256_tests: sha256_tests.o sha256.o
	$(CC) -lcmocka sha256.o sha256_tests.o -o sha256_tests

clean-sha256:
	rm sha256.o sha256_tests.o sha256_tests

queue.o: queue.h queue.c
	$(CC) $(CFLAGS) queue.h queue.c

queue-test.o: queue_test.c
	$(CC) $(CFLAGS) queue_test.c

queue-test: queue_test.o queue.o
	$(CC) -lcmocka queue.o queue_test.o -o queue_test

protocol.o: queue.o
	$(CC) $(CFLAGS) protocol.c

protocol: protocol.o queue.o
	$(CC) protocol.o queue.o -ljansson -o protocol

clean-protocol:
	rm protocol protocol.o