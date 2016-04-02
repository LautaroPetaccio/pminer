#include "sha256.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern void asm_sha256_hash(const uint8_t *data, const size_t length, char *hash);

int main() {
	int data[] = {0x00000000, 0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555, 0x66666666, 0x77777777, 0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF};
	char hash[65];
	clock_t begin, end;
	double total_time = 0;
	for (int i = 0; i < 100000; ++i) {
		begin = clock();
		for (int j = 0; j < 1000; ++j) {
			sha256_hash((uint8_t *) data, 16*4, hash);
		}
		end = clock();
		total_time += (double)(end - begin) / CLOCKS_PER_SEC;
	}
	printf("C: 1k hashes per %f sec \n", total_time / 100000);

	total_time = 0;
	for (int i = 0; i < 100000; ++i) {
		begin = clock();
		for (int j = 0; j < 1000; ++j) {
			asm_sha256_hash((uint8_t *) data, 16*4, hash);
		}
		end = clock();
		total_time += (double)(end - begin) / CLOCKS_PER_SEC;
	}
	printf("ASM: 1k hashes per %f sec \n", total_time / 100000);

	/* Randomized test */
	char c_hash[65];
	char asm_hash[65];
	int rand_data[500];
	time_t t;   
   	/* Intializes random number generator */
	srand((unsigned) time(&t));
	printf("Comparing if both results are equal: \n");
	for (int i = 0; i < 10000; ++i) {
		/* Fill rand_data */
		int rand_size = rand() % 500;
		for(int j = 0 ; j < rand_size ; j++)  {
			rand_data[j] = rand() % 5000000;
		}
		asm_sha256_hash((uint8_t *) data, rand_size * 4, asm_hash);
		sha256_hash((uint8_t *) data, rand_size * 4, c_hash);
		if(!strcmp(c_hash, asm_hash)) {
			printf("Hashes diferentes! C: %s\n ASM: %s \n", c_hash, asm_hash);
			return 0;
		}
	}
	printf("Todas las hashes son iguales!\n");
	return 0;
}