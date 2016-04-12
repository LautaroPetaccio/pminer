CC=gcc
ASM=nasm
ASMFLAGS=-f macho64
CFLAGS=-c -Wall -I/usr/local/include -O2 -g
LDFLAGS=-L/usr/local/lib


all: main tests

tests: sha256_tests queue_tests mining_test

clean:
	rm *.o *.gch tests/*.o tests/queue_tests tests/sha256_asm_tests tests/sha256_tests main analisis

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

worker.o: worker.c worker.h
	$(CC) $(CFLAGS) worker.h worker.c

queue.o: queue.h queue.c
	$(CC) $(CFLAGS) queue.h queue.c

queue_tests.o: tests/queue_tests.c
	$(CC) $(CFLAGS) tests/queue_tests.c -o tests/queue_tests.o

protocol.o: protocol.h protocol.c
	$(CC) $(CFLAGS) protocol.h protocol.c

mining_test.o: tests/mining_test.c
	$(CC) $(CFLAGS) tests/mining_test.c -o tests/mining_test.o

sha256_asm_tests.o: tests/sha256_asm_tests.c
	$(CC) $(CFLAGS) tests/sha256_asm_tests.c -o tests/sha256_asm_tests.o

main: main.o queue.o parser.o protocol.o sha256.o util.o worker.o nasm_sha256.o
	$(CC) $(LDFLAGS) main.o parser.o protocol.o queue.o sha256.o util.o worker.o nasm_sha256.o -o main -ljansson -pthread -o main

sha256_asm_tests: sha256_asm_tests.o sha256.o util.o nasm_sha256.o
	$(CC) $(LDFLAGS) -lcmocka sha256.o util.o nasm_sha256.o tests/sha256_asm_tests.o -o tests/sha256_asm_tests

mining_test: tests/mining_test.o sha256.o util.o nasm_sha256.o
	$(CC) $(LDFLAGS) -lcmocka sha256.o util.o nasm_sha256.o tests/mining_test.o -o tests/mining_test

sha256_tests: tests/sha256_tests.o sha256.o util.o nasm_sha256.o
	$(CC) $(LDFLAGS) -lcmocka sha256.o util.o nasm_sha256.o tests/sha256_tests.o -o tests/sha256_tests

queue_tests: tests/queue_tests.o queue.o
	$(CC) $(LDFLAGS) -lcmocka queue.o tests/queue_tests.o -o tests/queue_tests

analisis: nasm_sha256.o analisis.o util.o sha256.o
	ld -macosx_version_min 10.10 -lSystem -o analisis analisis.o nasm_sha256.o util.o sha256.o

analisis.o: analisis.c
	$(CC) $(CFLAGS) analisis.c

nasm_sha256.o: nasm_sha256.asm
	$(ASM) $(ASMFLAGS) nasm_sha256.asm