#include "sha256.h"
#include "util.h"

/* Macros used to compute the transform */
#define Ch(x, y, z)     ((x & (y ^ z)) ^ z)
#define Maj(x, y, z)    ((x & (y | z)) | (y & z))
#define ROTR(x, n)      ((x >> n) | (x << (32 - n)))
#define S0(x)           (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S1(x)           (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

/* Extension macros */
#define s0(x)           (ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3))
#define s1(x)           (ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10))

#define SHA256_LENGTH 32
#define WORK_DATA_LENGTH 256

static const uint32_t k[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t initial_state[8] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

uint32_t w[64];

static uint8_t sha256_padding[64] = {
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Initiate the sha256 state that will hold our binary hash */
void sha256_init(sha256_ctx *ctx) {
	/* Using memcpy as a faster way to copy the initial state */
	memcpy(ctx->state, initial_state, 32);
}

static void sha256_init_scan(uint32_t *state) {
	/* Using memcpy as a faster way to copy the initial state */
	memcpy(state, initial_state, 32);
}

/* Processes 64 bytes of data */
static void sha256_transform(sha256_ctx *ctx) {
	/* Copies data (16 32bits uint in big endian) */
	for(int i = 0; i < 16; ++i) {
		w[i] = swap_uint32(ctx->data[i]);
	}
	/* Extends w */
	for(int i = 16; i < 64; ++i) {
		w[i] = w[i-16] + s0(w[i-15]) + w[i-7] + s1(w[i-2]);
	}

	uint32_t ls[8]; // local state
	memcpy(ls, ctx->state, sizeof(uint32_t) * 8);
	/* 
		ls[0] -> a
		ls[1] -> b
		ls[2] -> c
		ls[3] -> d
		ls[4] -> e
		ls[5] -> f
		ls[6] -> g
		ls[7] -> h 
	*/

	/* Main loop */
	uint32_t t1, t2;
	for(int i = 0; i < 64; ++i) {
		t1 = ls[7] + S1(ls[4]) + Ch(ls[4], ls[5], ls[6]) + k[i] + w[i];
		t2 = S0(ls[0]) + Maj(ls[0], ls[1], ls[2]);
		ls[7] = ls[6];
		ls[6] = ls[5];
		ls[5] = ls[4];
		ls[4] = ls[3] + t1;
		ls[3] = ls[2];
		ls[2] = ls[1];
		ls[1] = ls[0];
		ls[0] = t1 + t2;
	}

	/* Adds the result to the state */

	ctx->state[0] += ls[0];
	ctx->state[1] += ls[1];
	ctx->state[2] += ls[2];
	ctx->state[3] += ls[3];
	ctx->state[4] += ls[4];
	ctx->state[5] += ls[5];
	ctx->state[6] += ls[6];
	ctx->state[7] += ls[7];

}

static void sha256_transform_scan(uint32_t *state, const uint32_t *data, uint32_t *w) {
	/* Copies data (16 32bits uint) */
	memcpy(w, data, 64);

	/* Extends w */
	for(int i = 16; i < 64; ++i) {
		w[i] = w[i-16] + s0(w[i-15]) + w[i-7] + s1(w[i-2]);
	}

	uint32_t ls[8]; // local state
	memcpy(ls, state, sizeof(uint32_t) * 8);
	/* 
		ls[0] -> a
		ls[1] -> b
		ls[2] -> c
		ls[3] -> d
		ls[4] -> e
		ls[5] -> f
		ls[6] -> g
		ls[7] -> h 
	*/

	/* Main loop */

	uint32_t t1, t2;
	for(int i = 0; i < 64; ++i) {
		t1 = ls[7] + S1(ls[4]) + Ch(ls[4], ls[5], ls[6]) + k[i] + w[i];
		t2 = S0(ls[0]) + Maj(ls[0], ls[1], ls[2]);
		ls[7] = ls[6];
		ls[6] = ls[5];
		ls[5] = ls[4];
		ls[4] = ls[3] + t1;
		ls[3] = ls[2];
		ls[2] = ls[1];
		ls[1] = ls[0];
		ls[0] = t1 + t2;
	}

	/* Adds the result to the state */

	state[0] += ls[0];
	state[1] += ls[1];
	state[2] += ls[2];
	state[3] += ls[3];
	state[4] += ls[4];
	state[5] += ls[5];
	state[6] += ls[6];
	state[7] += ls[7];

}

void sha256(sha256_ctx *context, const uint8_t *data, const size_t length) {
	if(length == 0) return;
	uint32_t bytes_left, bytes_hashed = 0;
	/* Processes data to be hashed */
	while(bytes_hashed < length) {
		bytes_left = length - bytes_hashed;
		/* If the size left is less t han 56 bytes, extend to 56 
			and append the lenght in bits in big endian
		*/
		if(bytes_left < 56) {
			memcpy((uint8_t *) context->data, data + bytes_hashed, bytes_left);
			bytes_hashed += bytes_left;
			memcpy((uint8_t *) context->data + bytes_left, sha256_padding, 56 - bytes_left);
			uint64_t be_bit_lenght = swap_uint64(bytes_hashed * 8);
			memcpy((uint8_t *) context->data + 56, &be_bit_lenght, 8);

			sha256_transform(context);
		}
		/* Less than 64 bytes of data left but more than 56 */
		/* The lenght in bits doesn't fit */
		else if(bytes_left <= 64) {
			/* Uses the padding to get to 64 bytes and hashes */
			memcpy((uint8_t *) context->data, data + bytes_hashed, bytes_left);
			bytes_hashed += bytes_left;
			memcpy((uint8_t *) context->data + bytes_left, sha256_padding, 64 - bytes_left);
			sha256_transform(context);

			/*
				Padds with 0's (the padding that was left in the last transform)
				 and then adds the padded messge size in bits, in a 64 be uint 
			*/
			if(bytes_left != 64) {
				memset(context->data, 0, 56);
			}
			else {
				memcpy(context->data, sha256_padding, 56);
			}
			uint64_t be_bit_lenght = swap_uint64(bytes_hashed * 8);
			memcpy((uint8_t *) context->data + 56, &be_bit_lenght, 8);
			sha256_transform(context);
		}
		else {
			memcpy((uint8_t *) context->data, data + bytes_hashed, 64);
			bytes_hashed += 64;
			sha256_transform(context);
		}
	}
	/* Converts result to big endian */
	for (int i = 0; i < 8; ++i) {
		context->state[i] = swap_uint32(context->state[i]);
	}

}

void sha256d(const uint8_t *data, const size_t length, uint8_t *hash) {
	sha256_ctx first_context, second_context;
	/* First hash */
	sha256_init(&first_context);
	sha256(&first_context, data, length);
	/* Second hash */
	// for(int i=0; i < 8; i++) {
	// 	printf("first_context.state[%i] = %08x\n", i, first_context.state[i]);
	// }
	sha256_init(&second_context);
	sha256(&second_context, (uint8_t *) first_context.state, SHA256_LENGTH);
	memcpy(hash, second_context.state, SHA256_LENGTH);
}

void sha256d_hash_le(const uint8_t *data, const size_t length, char *hash) {
	uint8_t raw_hashed[SHA256_LENGTH];
	sha256d(data, length, raw_hashed);
	/* Converts result to little endian */
	byte_swap(raw_hashed, SHA256_LENGTH);
	/* Creates hex hash */
	bin2hex(hash, raw_hashed, SHA256_LENGTH);
}

void sha256d_hash_be(const uint8_t *data, const size_t length, char *hash) {
	uint8_t raw_hashed[SHA256_LENGTH];
	sha256d(data, length, raw_hashed);
	/* Creates hex hash */
	bin2hex(hash, raw_hashed, SHA256_LENGTH);
}

void sha256_hash(const uint8_t *data, const size_t length, char *hash) {
	sha256_ctx context;
	sha256_init(&context);
	sha256(&context, data, length);

	bin2hex(hash, (uint8_t *) context.state, SHA256_LENGTH);
}

void sha256d_scan(uint32_t *fst_state, uint32_t *snd_state, const uint32_t *data, uint32_t *lw) {
	/* First hash */
	sha256_init_scan(fst_state);
	sha256_transform_scan(fst_state, data, lw);
	sha256_transform_scan(fst_state, data + 16, lw);
	/* Second hash */
	sha256_init_scan(snd_state);
	sha256_transform_scan(snd_state, fst_state, lw);
	/* Converts result to big endian */
	for (int i = 0; i < 8; ++i) {
		snd_state[i] = swap_uint32(snd_state[i]);
	}
}
