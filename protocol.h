#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include <stdio.h>
#include <jansson.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "queue.h"
#include "sha256.h"
#include "util.h"

typedef enum {SUBS_SENT, AUTH_SENT, AUTHORIZED} stratum_state;

struct work {
	uint32_t data[32];
	uint32_t target[8];

	int height;
	char *txs;
	char *workid;

	char *job_id;
	size_t nonce2_len;
	unsigned char *nonce2;
};

struct stratum_job {
	char *job_id;
	unsigned char prevhash[32];
	size_t coinbase_size;
	unsigned char *coinbase;
	unsigned char *nonce2;
	size_t merkle_count;
	unsigned char **merkle;
	char *merkle_root;
	unsigned char version[4];
	unsigned char nbits[4];
	unsigned char ntime[4];
	bool clean;
	double diff;
};

struct stratum_connection {
	int socket;
	stratum_state state;
	char *user;
	unsigned int send_id;
	double next_diff;
	char *subscription_id;
	char *notification_id;
	size_t nonce1_size;
	unsigned char *nonce1;
	size_t nonce2_size;
	struct stratum_job job;
};

struct work* create_work();
void destruct_work(struct work* work);
void stratum_notify(struct stratum_connection *connection, json_t *json_obj);
void stratum_submit_share(struct stratum_connection *connection, struct work *work);
void stratum_authorize(struct stratum_connection *connection,
	const char *user, const char *password);
void stratum_load_subscription(struct stratum_connection *connection, json_t *json_message);
void stratum_subscribe(struct stratum_connection *connection);
int sock(char *ip, char *port);
unsigned int recv_data(int sock, connection_buffer *recv_queue, unsigned int recv_size);
void send_data(int sock, char *data, unsigned int data_size);
void create_new_job(struct stratum_connection *connection, struct work* work);
void destruct_stratum_connection(struct stratum_connection *connection);
struct stratum_connection* create_stratum_connection(int socket);

#endif /* protocol.h */
