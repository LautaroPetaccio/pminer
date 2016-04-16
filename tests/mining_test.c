#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <jansson.h>
#include "../sha256.h"
#include "../util.h"
#include "../protocol.h"
#include "../worker.h"

// extern void* _test_malloc(const size_t size, const char* file, const int line);
// extern void* _test_calloc(const size_t number_of_elements, const size_t size,
//                           const char* file, const int line);
// extern void _test_free(void* const ptr, const char* file, const int line);

// #define malloc(size) _test_malloc(size, __FILE__, __LINE__)
// #define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
// #define free(ptr) _test_free(ptr, __FILE__, __LINE__)

static void correct_swap(void **state) {
	(void) state; /* unused */
	uint32_t normal = 0x12345678;
	uint32_t with_swap = swap_uint32(normal);
	uint32_t with_le32dec = le32dec((const void *) &normal);
	uint32_t with_be32dec = be32dec((const void *) &normal);
	uint32_t with_be32enc;
	be32enc(&with_be32enc, with_be32dec);
	assert_int_equal(normal, with_le32dec);
	assert_int_equal(with_swap, with_be32dec);
	assert_int_equal(normal, with_be32enc);
}

static void coinbase_build_test(void **state) {
	(void) state; /* unused */
	/* Test setup */
	char *subscribe_message = "{\"error\": null, \"id\": 1, \"result\": [[\"mining.notify\","
	" \"ae6812eb4cd7735a302a8a9dd95cf71f\"], \"60021014\", 4]}";
	char *notify_message = "{\"params\": [\"5c04\","
	"\"da0dadb0eda4381df442bde08d23d54d7d371d5ce7af3ee716bd2a7e017eacb8\","
	" \"01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff2a03700a08062f503253482f04953f1a5308\","
	" \"102f776166666c65706f6f6c2e636f6d2f0000000001d07e582a010000001976a9145d8f33b0a7c94c878d572c40cbff22a49268467d88ac00000000\","
	" [\"50a4a386ab344d40d29a833b6e40ea27dab6e5a79a2f8648d3bc0d1aa65ecd3f\","
	" \"7952ecc836fb104f41b2cb06608eeeaa6d1ca2fe4391708fb13bb10ccf8da179\","
	" \"9400ec6453aac577fb6807f11219b4243a3e50ca6d1c727e6d05663211960c94\","
	" \"c11a630fa9332ab51d886a47509b5cbace844316f4fc52b493359b305fd489ae\","
	" \"85891e7c5773f234d647f1d5fca7fbcabb59b261322d16c0ae486ccf5143383d\","
	" \"faa26bbc17f99659f64136bea29b3fc8d772b339c52707d5f2ccfe1195317f43\"],"
	" \"00000002\", \"1b10b60e\", \"531a3f95\", false], \"id\": null, \"method\": \"mining.notify\"}";
	json_error_t *decode_error = NULL;
	json_t *json_obj = NULL;
	struct stratum_context *context = create_stratum_context();
	uint8_t hashed_coinbase[65];
	uint8_t merkle_root[64];
	/* End test setup */

	json_obj = json_loads(subscribe_message, JSON_DECODE_ANY, decode_error);
	stratum_load_subscription(context, json_obj);
	json_decref(json_obj);
	json_obj = json_loads(notify_message, JSON_DECODE_ANY, decode_error);
	stratum_notify(context, json_obj);
	json_decref(json_obj);

	sha256d(context->job.coinbase, context->job.coinbase_size, merkle_root);
	bin2hex((char *) hashed_coinbase, merkle_root, 32);
	destruct_stratum_context(context);
}


static void block_hash_test(void **state) {
	/* Test block https://blockexplorer.com/block/00000000000000001e8d6829a8a21adc5d38d0a473b144b6765798e61f98bd1d */
	(void) state; /* unused */
	struct work *test_work = create_work();
	/* prev_hash */
	char *prev_hash = "81cd02ab7e569e8bcd9317e2fe99f2de44d49ab2b8851ba4a308000000000000";
	char prevhash_bin[32];
	hex2bin((uint8_t*) prevhash_bin, prev_hash, strlen(prev_hash));
	/* merkle_root */
	char *merkle_root = "2b12fcf1b09288fcaff797d71e950e71ae42b91e8bdb2304758dfcffc2b620e3";
	char merkle_bin[32];
	hex2bin((uint8_t*) merkle_bin, merkle_root, strlen(merkle_root));

	byte_swap((uint8_t*) merkle_bin, 32);
	test_work->block_header.version = 1;
	memcpy(test_work->block_header.prev_block, prevhash_bin, 32);
	memcpy(test_work->block_header.merkle_root, merkle_bin, 32);
	/* Little endian Time */
	test_work->block_header.timestamp = 0x4DD7F5C7;
	/* Little endian Bits */
	test_work->block_header.bits = 0x1a44b9f2;
	/* Nonce */
	test_work->block_header.nonce = 2504433986;

	char res_hash[65];
	uint8_t binary_hash[32];
	sha256d((uint8_t *) &test_work->block_header, 80, binary_hash);
	byte_swap(binary_hash, 32);
	bin2hex(res_hash, binary_hash, 32);
	assert_string_equal(res_hash, "00000000000000001e8d6829a8a21adc5d38d0a473b144b6765798e61f98bd1d");
	destruct_work(test_work);
}
static void mine_test2(void **state) {
	(void) state; /* unused */
	/* Test init */
	struct stratum_context *context = create_stratum_context();
	json_t *json_obj = NULL;
	struct work *test_work = create_work();
	uint32_t fst_state[16];
	uint32_t res_hash_bin[8];
	char res_hash_hex[65];
	uint32_t lw[64];
	json_error_t *decode_error = NULL;
	char *notify_message = "{\"params\": [\"3308\", \"fbd8725864ccc37bd06bf7f8e5e52896f8a2bc29a19c9ce18feebf06bcedfb66\", \"01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff270385a30b062f503253482f04016a0e5708\", \"0d2f7374726174756d506f6f6c2f0000000001c2eb0b00000000001976a914007cedfed869f050d36c16fb491b1d31da89ad3e88ac00000000\", [], \"00750003\", \"1a016a3c\", \"570e6996\", true], \"id\": null, \"method\": \"mining.notify\"}";
	char *subs_message = "{\"error\": null, \"id\": 1, \"result\": [[\"mining.notify\", \"ae6812eb4cd7735a302a8a9dd95cf71f\"], \"20081d29\", 4]}";
	/* Test setup end */

	/* Receives subscription */
	json_obj = json_loads(subs_message, JSON_DECODE_ANY, decode_error);
	stratum_load_subscription(context, json_obj);
	json_decref(json_obj);

	/* Receives notify */
	context->next_diff = 1;
	json_obj = json_loads(notify_message, JSON_DECODE_ANY, decode_error);
	stratum_notify(context, json_obj);
	json_decref(json_obj);

	/* Generates work */
	generate_new_work(context, test_work);
	/* Setts the nonce that results in a hash lower than the target */
	test_work->block_header.nonce = 0x431e3e53;

	/* Getting the hash */
	/* Preparing the padding */
	fst_state[8] = 0x80000000;
	memset(fst_state + 9, 0, 24);
	fst_state[15] = 0x00000100;

	/* Hashing */
	// sha256d_scan(fst_state, res_hash_bin, test_work.data, lw);
	sha256d_scan(fst_state, res_hash_bin, (uint32_t *) &test_work->block_header, lw);

	/* Checks if fulltest works */
	assert_true(fulltest(res_hash_bin, test_work->target));
	/* Asserting with the hex hash */
	byte_swap((uint8_t *) res_hash_bin, 32);
	bin2hex(res_hash_hex, (uint8_t*) res_hash_bin, 32);
	assert_string_equal(res_hash_hex, "00000000082f5e75206d5606248a09538e6ca5df4eedac20e8682b7832953ff8");
	destruct_work(test_work);
	destruct_stratum_context(context);


}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(correct_swap),
		cmocka_unit_test(block_hash_test),
		cmocka_unit_test(mine_test2),
		cmocka_unit_test(coinbase_build_test),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
