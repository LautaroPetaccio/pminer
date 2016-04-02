CC=gcc
ASM=nasm
ASMFLAGS=-f macho64
CFLAGS=-c -Wall -I/usr/local/include
LDFLAGS=-L/usr/local/lib


all: main tests

tests: sha256_tests queue_tests

clean:
	rm *.o *.gch tests/*.o tests/queue_tests tests/sha256_tests main analisis

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
	$(CC) $(LDFLAGS) main.o parser.o protocol.o queue.o sha256.o util.o -o main -ljansson -o main

sha256_tests: tests/sha256_tests.o sha256.o util.o
	$(CC) $(LDFLAGS) -lcmocka sha256.o util.o tests/sha256_tests.o -o tests/sha256_tests

queue_tests: tests/queue_tests.o queue.o
	$(CC) $(LDFLAGS) -lcmocka queue.o tests/queue_tests.o -o tests/queue_tests

analisis: nasm_sha256.o analisis.o util.o sha256.o
	ld -macosx_version_min 10.10 -lSystem -o analisis analisis.o nasm_sha256.o util.o sha256.o

analisis.o: analisis.c
	$(CC) $(CFLAGS) analisis.c

nasm_sha256.o:
	$(ASM) $(ASMFLAGS) nasm_sha256.asm