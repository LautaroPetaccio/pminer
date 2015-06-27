#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "queue.h"

extern void* _test_malloc(const size_t size, const char* file, const int line);
extern void* _test_calloc(const size_t number_of_elements, const size_t size,
                          const char* file, const int line);
extern void _test_free(void* const ptr, const char* file, const int line);

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)

static void test_create(void **state) {
	struct connection_buffer* queue = queue_create(2000);
	(void) state; /* unused */
	assert_true(queue->buffer == queue->buffer_end_pointer);
	assert_int_equal(queue->buffer_size, 2000);
	queue_free(queue);
}

static void test_push_pop(void **state) {
	struct connection_buffer* queue = queue_create(2000);
	(void) state; /* unused */
	queue_push(queue, "Mensaje1\n", sizeof(char) * 9);
	char *message;
	size_t message_size = queue_pop(queue, &message, '\n');
	assert_int_equal(message_size, 9);
	message[message_size-1] = '\0';
	assert_string_equal("Mensaje1", message);
	free(message);
	queue_free(queue);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_create),
		cmocka_unit_test(test_push_pop),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}