#ifndef _QUEUE_H
#define _QUEUE_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	size_t buffer_size;
	char *buffer_end_pointer;
	char *buffer;
} connection_buffer;

void print_queue_buffer(connection_buffer *cb);
connection_buffer* queue_create(size_t size);
void queue_free(connection_buffer *cb);
size_t queue_pop(connection_buffer *cb, char **message, char delimiter);
size_t queue_free_size(connection_buffer *cb);
int queue_push(connection_buffer *cb, char *message, size_t size);

#endif /* queue.h */
