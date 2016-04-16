#include "queue.h"

void print_queue_buffer(connection_buffer *cb) {
	printf("Queue dump \n");
	for (size_t i = 0; i < (size_t) (cb->buffer_end_pointer - cb->buffer); ++i){
		printf("%c", cb->buffer[i]);
	}
	printf("\n");
}

connection_buffer* queue_create(size_t size) {
	connection_buffer *cb = (connection_buffer*) malloc(sizeof(connection_buffer));
	cb->buffer_size = size;
	cb->buffer = malloc(size * sizeof(char));
	cb->buffer_end_pointer = cb->buffer;
	return cb;
}

void queue_free(connection_buffer *cb) {
	free(cb->buffer);
	free(cb);
}

/* Extracts message from queue */
size_t queue_pop(connection_buffer *cb, char **message, char delimiter) {
	/* If the queue is empty return 0 */
	if(!(cb->buffer_end_pointer - cb->buffer)) return 0;

	char *str_end_ptr = (char *) memchr(cb->buffer, 
										(int) delimiter, 
										(cb->buffer_end_pointer - cb->buffer));
	if(str_end_ptr) {
		size_t message_size = (str_end_ptr - cb->buffer) + 1;
		(*message) = malloc(message_size * sizeof(char) + 1);
		memcpy((*message), cb->buffer, message_size);
		(*message)[message_size] = '\0';

		// Move queue
		memmove(cb->buffer, str_end_ptr + 1, ((size_t) cb->buffer_end_pointer - 1 - (size_t) str_end_ptr));
		cb->buffer_end_pointer = cb->buffer_end_pointer - message_size;

		return message_size;
	}
	// No message in queue
	return 0;
}

void queue_pop_size(connection_buffer *cb, size_t size) {
	/* Reduce the end_pointer by the size we want to pop */
	cb->buffer_end_pointer = cb->buffer_end_pointer - size;
	/* If we didn't pop all the data, move it */
	if(cb->buffer_end_pointer != cb->buffer) 
		memmove(cb->buffer, cb->buffer + size, size);
}

size_t queue_free_size(connection_buffer *cb) {
	return cb->buffer + cb->buffer_size - cb->buffer_end_pointer;
}

size_t queue_size(connection_buffer *cb) {
	return cb->buffer_size;
}

size_t queue_used_size(connection_buffer *cb) {
	return queue_size(cb) - queue_free_size(cb);
}

int queue_push(connection_buffer *cb, char *message, size_t size) {
	if(queue_free_size(cb) >= size) {
		memcpy(cb->buffer_end_pointer, message, size);
		cb->buffer_end_pointer += size;
		return 1;
	}
	return 0;
}
