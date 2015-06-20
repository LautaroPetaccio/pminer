#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "sha256.h"

extern void* _test_malloc(const size_t size, const char* file, const int line);
extern void* _test_calloc(const size_t number_of_elements, const size_t size,
                          const char* file, const int line);
extern void _test_free(void* const ptr, const char* file, const int line);

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)

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

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_length_smaller_than_56),
		cmocka_unit_test(test_length_equal_to_56),
		cmocka_unit_test(test_length_less_than_64),
		cmocka_unit_test(test_length_equal_to_64),
		cmocka_unit_test(test_length_greater_than_64),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}