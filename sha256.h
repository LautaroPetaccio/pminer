#ifndef _SHA256_H
#define _SHA256_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
	uint32_t state[8];
	uint32_t data[16];
} sha256_ctx;

void sha256_hash(const uint8_t *data, const size_t length, char *hash);

void sha256d_hash(const uint8_t *data, const size_t length, char *hash);

#endif /* sha256.h */

