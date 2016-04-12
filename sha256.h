#ifndef _SHA256_H
#define _SHA256_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	uint32_t state[8];
	uint32_t data[16];
} sha256_ctx;

void sha256_hash(const uint8_t *data, const size_t length, char *hash);

void sha256d_hash_le(const uint8_t *data, const size_t length, char *hash);

void sha256d_hash_be(const uint8_t *data, const size_t length, char *hash);

void sha256d(const uint8_t *data, const size_t length, uint8_t *hash);

void sha256d_scan(uint32_t *fst_state, uint32_t *snd_state, const uint32_t *data, uint32_t *lw);

#endif /* sha256.h */

