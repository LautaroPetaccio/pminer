#include "sender.h"

void *sender_thread(void *arg) {
	struct stratum_connection* connection = (struct stratum_connection*) arg;
	while(true) {
		pthread_mutex_lock(&connection->send_queue_mutex);
		/* If our queue doesn't have any data, wait for condition */
		while(queue_used_size(connection->send_queue) == 0) {
			/* Waits */
			pthread_cond_wait(&connection->sender_condition, &connection->send_queue_mutex);
		}
		send_messages(connection);
		pthread_mutex_unlock(&connection->send_queue_mutex);
	}

}

pthread_t * create_sender(struct stratum_connection* connection) {
	pthread_t * sender_thr = malloc(sizeof(pthread_t));
	if(pthread_create(sender_thr, NULL, sender_thread, connection))
		return NULL;
	return sender_thr;
}
