CC=gcc -std=c11
ASM=nasm

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CFLAGS=-c -Wall -Wextra -Wpedantic -O2 -g
    ASMFLAGS=-f elf64
    NASM_SHA256 = nasm_sha256_linux.asm
endif
ifeq ($(UNAME_S),Darwin)
    CFLAGS=-c -Wall -O2 -g -Wextra -Wpedantic -I/usr/local/include -g
    ASMFLAGS=-f macho64
    CC +=-L/usr/local/lib
    NASM_SHA256 = nasm_sha256.asm
endif


all: main tests

tests: sha256_tests queue_tests mining_test

clean:
	rm *.o *.gch tests/*.o tests/queue_tests tests/sha256_asm_tests tests/sha256_tests tests/mining_test main analisis

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

worker.o: worker.c worker.h work.h
	$(CC) $(CFLAGS) worker.h worker.c

queue.o: queue.h queue.c
	$(CC) $(CFLAGS) queue.h queue.c

queue_tests.o: tests/queue_tests.c
	$(CC) $(CFLAGS) tests/queue_tests.c -o tests/queue_tests.o

protocol.o: protocol.h protocol.c work.h
	$(CC) $(CFLAGS) protocol.h protocol.c

mining_test.o: tests/mining_test.c
	$(CC) $(CFLAGS) tests/mining_test.c -o tests/mining_test.o

main: main.o queue.o parser.o protocol.o sha256.o util.o worker.o nasm_sha256.o
	$(CC) main.o parser.o protocol.o queue.o sha256.o util.o worker.o nasm_sha256.o -o main -ljansson -pthread -o main

mining_test: tests/mining_test.o sha256.o util.o nasm_sha256.o protocol.o queue.o worker.o
	$(CC) -lcmocka -ljansson sha256.o util.o nasm_sha256.o protocol.o queue.o worker.o tests/mining_test.o -o tests/mining_test

sha256_tests: tests/sha256_tests.o sha256.o util.o nasm_sha256.o
	$(CC) -lcmocka -lcrypto sha256.o util.o nasm_sha256.o tests/sha256_tests.o -o tests/sha256_tests

queue_tests: tests/queue_tests.o queue.o
	$(CC) -lcmocka queue.o tests/queue_tests.o -o tests/queue_tests

analisis: nasm_sha256.o analisis.o util.o sha256.o
	#ld -macosx_version_min 10.10 -lSystem -o analisis analisis.o nasm_sha256.o util.o sha256.o
	$(CC) -o analisis analisis.o nasm_sha256.o util.o sha256.o

analisis.o: analisis.c
	$(CC) $(CFLAGS) analisis.c

nasm_sha256.o: $(NASM_SHA256)
	$(ASM) $(ASMFLAGS) $(NASM_SHA256) -o nasm_sha256.o
