#ifndef QUEUE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct connection_buffer {
	size_t buffer_size;
	char *buffer_end_pointer;
	char *buffer;
};

void print_queue_buffer(struct connection_buffer *cb);
struct connection_buffer* queue_create(size_t size);
void queue_free(struct connection_buffer *cb);
size_t queue_pop(struct connection_buffer *cb, char **message, char delimiter);
size_t queue_free_size(struct connection_buffer *cb);
int queue_push(struct connection_buffer *cb, char *message, size_t size);

#define QUEUE
#endif
