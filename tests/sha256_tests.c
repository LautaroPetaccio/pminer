#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include "../sha256.h"
#include "../util.h"

extern void* _test_malloc(const size_t size, const char* file, const int line);
extern void* _test_calloc(const size_t number_of_elements, const size_t size,
                          const char* file, const int line);
extern void _test_free(void* const ptr, const char* file, const int line);

extern void asm_sha256_hash(const uint8_t *data, const size_t length, char *hash);
extern void asm_sha256d_scan(uint32_t *fst_state, uint32_t *snd_state, const uint32_t *data);

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)

void reverse(char *s) {
  char *end,temp;
  end = s;
  while(*end != '\0'){
    end++;
  }
  end--;  //end points to last letter now
  for(;s<end;s++,end--){
    temp = *end;
    *end = *s;
    *s = temp; 
  }
}

static void test_length_smaller_than_56(void **state) {
	(void) state; /* unused */
	char *data = "abc";
	char hash[65];
	sha256_hash((uint8_t *) data, strlen(data), hash);
	assert_string_equal(hash, 
		"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

static void test_length_equal_to_56(void **state) {
	(void) state; /* unused */
	/* 56 bytes */
	char *data = "abababababababababababababababababababababababababababab";
	char hash[65];
	sha256_hash((uint8_t *) data, strlen(data), hash);
	assert_string_equal(hash, 
		"ee7eebb48cfd2e30381bff1c9280da8f964a48776fe5023d935c1bdf8e4d5d1f");
}

static void test_length_less_than_64(void **state) {
	(void) state; /* unused */
	/* 62 bytes */
	char *data = "ababababababababababababababababababababababababababababababab";
	char hash[65];
	sha256_hash((uint8_t *) data, strlen(data), hash);
	assert_string_equal(hash, 
		"cf696841dde2e468783850b8160122ac722f368422453e52e8b635093d2f0d52");
}

static void test_length_equal_to_64(void **state) {
	(void) state; /* unused */
	/* 64 bytes */
	char *data = "abababababababababababababababababababababababababababababababab";
	char hash[65];
	sha256_hash((uint8_t *) data, strlen(data), hash);
	assert_string_equal(hash, 
		"271a413bd339c5709fdceaec41f14f11e9fbfb5042d72d331c65f32b284cd09a");
}

static void test_length_greater_than_64(void **state) {
	(void) state; /* unused */
	/* 66 bytes */
	char *data = "ababababababababababababababababababababababababababababababababab";
	char hash[65];
	sha256_hash((uint8_t *) data, strlen(data), hash);
	assert_string_equal(hash, 
		"fcae0435ecabaebf770d9506763a6e3cd0aaab640ff0305312d91fd6979a2c99");
}

static void test_with_other_source(void **state) {
	(void) state; /* unused */
	char hash[65];
	char *results[3] = {
		"a58dd8680234c1f8cc2ef2b325a43733605a7f16f288e072de8eae81fd8d6433",
		"0be82463c427624862fd06226c78e4c0cfd78fd4e5376da3110364bb0ef454a4",
		"67aed39c1e9b8a4c653c816d16f54e18c57a7ddfa8f1245958f4a750ebf4d4c2",
		};
	char *data[3] = {
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
		"Curabitur maximus dictum condimentum.",
		"Nulla gravida maximus velit, eu sodales mi pretium non. Fusce malesuada accumsan tortor in venenatis.",
		};
	char *shad_result[3] = {
		"6a7c126d622bcbb107ddf5f828730077872a2f4cfaabe2398847bd3a0ecb324a",
		"fa02ceca513217dcbe7e22ce2a1788a3b1a6458b9499a8b3ceaa734fc8ac5b26",
		"5d5c1179eb47ee31a279030b110add437c964b845cdab7b7b8ba8be95235ffcb",
		};
	for (int i = 0; i < 3; ++i) {
		sha256_hash((uint8_t *) data[i], strlen(data[i]), hash);
		assert_string_equal(hash, results[i]);
		/* The assembly function must give the same results */
		memset(hash, 0, 65);
		asm_sha256_hash((uint8_t *) data[i], strlen(data[i]), hash);
		assert_string_equal(hash, results[i]);
		memset(hash, 0, 65);
		sha256d_hash_be((uint8_t *) data[i], strlen(data[i]), hash);
		// sha256_hash((uint8_t *) results[i], strlen(results[i]), hash);
		assert_string_equal(hash, shad_result[i]);
	}

	uint32_t hash1[8];
	uint32_t hash2[8];
	uint32_t hash3[8];	   
	SHA256_CTX sha256_pass1, sha256_pass2;
	sha256_ctx my_context;
	uint32_t random_data[32];
	srand(time(NULL));
	for (int i = 0; i < 32; ++i) {
		random_data[i] = rand() % 2147483647;
	}

	for (int i = 0; i < 1000; ++i) {
		size_t size = rand() % 128;
		if(!size) continue;
		SHA256_Init(&sha256_pass1);
		SHA256_Update(&sha256_pass1, (unsigned char*) random_data, size);
		SHA256_Final((uint8_t *) hash1, &sha256_pass1);

		SHA256_Init(&sha256_pass2);
		SHA256_Update(&sha256_pass2, hash1, SHA256_DIGEST_LENGTH);
		SHA256_Final((uint8_t *) hash2, &sha256_pass2);

		sha256_init(&my_context);
		sha256(&my_context, (unsigned char*) random_data, size);
		for (int j = 0; j < 8; ++j) {
			assert_int_equal(hash1[j], my_context.state[j]);
		}
		sha256d((unsigned char*) random_data, size, (uint8_t *) hash3);
		for (int j = 0; j < 8; ++j) {
			assert_int_equal(hash2[j], hash3[j]);
		}
	}
}

static void test_256d_little_endian(void **state) {
	(void) state; /* unused */
	/* 64 bytes */
	char *data = "abc";
	char hash[65];
	sha256d_hash_le((uint8_t *) data, strlen(data), hash);
	/* Converts result to little endian */
	assert_string_equal(hash, 
		"58636c3ec08c12d55aedda056d602d5bcca72d8df6a69b519b72d32dc2428b4f");
}

static void test_256d_scan(void **state) {
	(void) state; /* unused */
	/* Hash scan */
	uint32_t wl[64];
	uint32_t data[32];
	uint32_t fst_state[16];
	uint32_t hash_result[8];
	srand(time(NULL));
	// memset(data, 0x00, 128);
	for (int i = 0; i < 20; ++i) {
		data[i] = rand() % 2147483647;
	}

	char hash_scan[65];
	char hash_normal[65];
	for (int i = 0; i < 32; ++i) {
		data[i] = swap_uint32(data[i]);
	}
	sha256d_hash_be((uint8_t *) data, 80, hash_normal);
	bin2hex(hash_scan, (uint8_t *) hash_result, 32);
	for (int i = 0; i < 32; ++i) {
		data[i] = swap_uint32(data[i]);
	}

	memset(data + 20, 0x00, 48);
	data[20] = 0x80000000;
	data[31] = 0x00000280;
	fst_state[8] = 0x80000000;
	memset(fst_state + 9, 0, 24);
	fst_state[15] = 0x00000100;

	sha256d_scan(fst_state, hash_result, data, wl);
	bin2hex(hash_scan, (uint8_t *) hash_result, 32);
	assert_string_equal(hash_scan, hash_normal);

	memset(hash_scan, 0, 65);
	asm_sha256d_scan(fst_state, hash_result, data);
	bin2hex(hash_scan, (uint8_t *) hash_result, 32);
	assert_string_equal(hash_scan, hash_normal);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_length_smaller_than_56),
		cmocka_unit_test(test_length_equal_to_56),
		cmocka_unit_test(test_length_less_than_64),
		cmocka_unit_test(test_length_equal_to_64),
		cmocka_unit_test(test_length_greater_than_64),
		cmocka_unit_test(test_256d_little_endian),
		cmocka_unit_test(test_256d_scan),
		cmocka_unit_test(test_with_other_source),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
