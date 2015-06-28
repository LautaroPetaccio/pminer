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

char buffer[256];
unsigned int id;
unsigned int MAX_SEND_RETY = 4;

void create_new_job(struct stratum_connection *connection, struct work* work) {
	uint8_t merkle_root[65];
	work->job_id = strdup(connection->job.job_id);
	work->nonce2_len = connection->nonce2_size;
	work->nonce2 = realloc(work->nonce2, connection->nonce2_size);
	memcpy(work->nonce2, connection->job.nonce2, connection->nonce2_size);

	////////////////////////////////////////////////
	//       Transaction header creation          //
	////////////////////////////////////////////////
	/*
	   Version         4 bytes
	   hashPrevBlock  32 bytes
	   hashMerkleRoot 32 bytes
	   Time            4 bytes
	   Bits            4 bytes
	   Nonce           4 bytes
	*/

	/* Generates merkle root */
	sha256d(connection->job.coinbase, connection->job.coinbase_size, merkle_root);
	for(int i = 0; i < connection->job.merkle_count; i++) {
		memcpy(merkle_root + 32, connection->job.merkle[i], 32);
		sha256d(merkle_root, 64, merkle_root);
	}
	/* Increment extranonce2 ??????*/
	for (int i = 0; i < connection->nonce2_size && !++connection->job.nonce2[i]; i++);

	/* Assemble block header */
	memset(work->data, 0, 128);
	/* Version */
	work->data[0] = swap_uint32(*((uint32_t *) connection->job.version));
	/* hashPrevBlock */
	for(int i = 0; i < 8; i++)
		work->data[1 + i] = swap_uint32(*((uint32_t *) connection->job.prevhash + i));
	/* hashMerkleRoot */
	for(int i = 0; i < 8; i++)
		work->data[9 + i] = swap_uint32(*((uint32_t *) merkle_root + i));

	/* Little endian Time */
	work->data[17] = swap_uint32(*((uint32_t *) connection->job.ntime));
	/* Little endian Bits */
	work->data[18] = swap_uint32(*((uint32_t *) connection->job.nbits));
	/* work->data[19]; Nonce */
	/* Padding data, optimizing the transform */
	/* Useful for the fpga miner */
	work->data[20] = 0x80000000;
	work->data[31] = 0x00000280;
	//diff_to_target(work->target, connection->job.diff);

}

void send_data(int sock, char *data, unsigned int data_size) {
	unsigned int sent_data_size = 0;
	int n;
	// Writes to socket
	for (unsigned int i = 0; i < MAX_SEND_RETY; ++i) {
		n = write(sock, (void *) (data + sent_data_size), data_size - sent_data_size);
		if(n < 0) {
			fprintf(stderr, "ERROR writing to socket, wrote 0 bytes\n");
		}
		sent_data_size += n;
		if(sent_data_size == data_size) return;
	}
	fprintf(stderr, "ERROR writing to socket, max retries\n");
}

unsigned int recv_data(int sock, struct connection_buffer *recv_queue, unsigned int recv_size) {
	if(queue_free_size(recv_queue) < recv_size) return 0;

	char recv_buffer[recv_size];
	int recieved_size = read(sock, (void *) recv_buffer, recv_size);
	if(recieved_size < 0) {
		fprintf(stderr, "ERROR reading from socket\n");
		// Socket closed, kill everything
	}
	printf("Recieved %d bytes in total \n", recieved_size);
	queue_push(recv_queue, recv_buffer, recieved_size);
	return recieved_size;
}


int sock(char *ip, char *port) {
	int sockfd, portno;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	portno = atoi(port);
	// Open connection socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
    	fprintf(stderr,"ERROR opening socket \n");
		exit(0);
	}
	// Getting hostname
	server = gethostbyname(ip);
	if(server == NULL) {
	    fprintf(stderr,"ERROR, no such host %s \n", ip);
	    exit(0);
	}
	// Zeroes serv_addr memory
	bzero((char *) &serv_addr, sizeof(serv_addr));
	// Sets sin_family as AF_INTE
	serv_addr.sin_family = AF_INET;
	// Sets server's address using server's ip from gethostbyname
	bcopy((char *)server->h_addr, 
	     (char *)&serv_addr.sin_addr.s_addr,
	     server->h_length);
	// Sets server's ip
	serv_addr.sin_port = htons(portno);
	// Connects to server
	if(connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	    fprintf(stderr, "ERROR connecting\n");
	    exit(0);
	}

	return sockfd;
}

void stratum_subscribe(struct stratum_connection *connection) {
	//Send connection request
	json_t *root = json_object();
	json_object_set_new(root, "id", json_integer(connection->send_id));
	json_object_set_new(root, "method", json_string("mining.subscribe"));
	json_object_set_new(root, "params", json_array());
	char *res_string = json_dumps(root, JSON_ENCODE_ANY | JSON_PRESERVE_ORDER);
	printf("String to be sent: \n%s\n", res_string);
	size_t res_string_length = strlen(res_string);
	//Converts endstring character to newline to be read by the stratum server
	res_string[res_string_length] = '\n';
	send_data(connection->socket, res_string, res_string_length + 1);
	free(res_string);
	json_decref(root);
	connection->send_id++;
	printf("Connection request sent \n");
}

void stratum_load_subscription(struct stratum_connection *connection, json_t *json_message) {
	json_t *result = json_object_get(json_message, "result");
	//json_t *error = json_object_get(json_message, "error");
	json_t *result_fst = json_array_get(result, 0);
	json_t *result_fst_fst = json_array_get(result_fst, 0);
	json_t *result_fst_snd = json_array_get(result_fst, 1);
	json_t *set_difficulty = json_array_get(result_fst_fst, 1);
	json_t *notify = json_array_get(result_fst_snd, 1);
	// Stores subscription details
	printf("Storing subscription details \n");
	connection->notification_id = strdup(json_string_value(notify));
	printf("Malloc notification id \n");
	connection->subscription_id = strdup(json_string_value(set_difficulty));

	// Nonce 1 is stored as the binary conversion from the hex string
	size_t nonce1_size = strlen(json_string_value(json_array_get(result, 1)))/2;
	connection->nonce1 = malloc(nonce1_size * sizeof(char));
	hex2bin(connection->nonce1, json_string_value(json_array_get(result, 1)), nonce1_size);
	printf("Storing nonce 1\n");
	connection->nonce2_size = json_integer_value(json_array_get(result, 2));
	printf("End storing subscription details \n");

	printf("notify : %s , set_difficulty : %s \n", 
		json_string_value(notify), json_string_value(set_difficulty));
}

void stratum_authorize(struct stratum_connection *connection,
	const char *user, const char *password) {
	//Send authorize request
	json_t *root = json_object();
	json_object_set_new(root, "id", json_integer(connection->send_id));
	json_object_set_new(root, "method", json_string("mining.authorize"));
	json_t *array = json_array();
	json_array_append_new(array, json_string(user));
	json_array_append_new(array, json_string(password));
	json_object_set_new(root, "params", array);
	char *res_string = json_dumps(root, JSON_ENCODE_ANY | JSON_PRESERVE_ORDER);
	size_t message_len = strlen(res_string);
	printf("Adding endline \n");
	res_string[strlen(res_string) - 1] = '\n';
	printf("Preparing to send message \n");
	send_data(connection->socket, res_string, message_len);	
	printf("Message sent \n");
	json_decref(root);
	free(res_string);
	connection->send_id++;
	printf("Subscription request sent \n");
}

void stratum_submit_share(struct stratum_connection *connection, struct work *work) {
	uint32_t ntime, nonce;
	char time_string[9], nonce_string[9], nonce2_string[(connection->nonce2_size * 2) + 1];
	// Converts to little endian?
	//le32enc(&ntime, work->data[17]);
	//le32enc(&nonce, work->data[19]);
	bin2hex(time_string, (const unsigned char *) (&ntime), 4);
	bin2hex(nonce_string, (const unsigned char *) (&nonce), 4);
	bin2hex(nonce2_string,(const unsigned char *) (&work->nonce2), connection->nonce2_size);

	json_t *root = json_object();
	json_t *params = json_array();
	json_object_set_new(root, "params", params);
	json_object_set_new(root, "id", json_integer(connection->send_id));
	json_object_set_new(root, "method", json_string("mining.submit"));
	// User, previously authorized
	json_array_append_new(params, json_string(connection->user));
	// Job id
	json_array_append_new(params, json_string(connection->job.job_id));
	// extra nonce2
	json_array_append_new(params, json_string(nonce2_string));
	json_array_append_new(params, json_string(time_string));
	json_array_append_new(params, json_string(nonce_string));
	char *res_string = json_dumps(root, JSON_ENCODE_ANY | JSON_PRESERVE_ORDER);
	res_string[strlen(res_string)] = '\n';
	send_data(connection->socket, res_string, strlen(res_string) + 1);
	// El incremento deberia estar en el send?
	connection->send_id++;
	json_decref(root);
	free(res_string);
}

void  stratum_notify(struct stratum_connection *connection, json_t *json_obj) {
	//json_t *id = json_object_get(json_obj, "id");
	json_t *params = json_object_get(json_obj, "params");
	// job_id - ID of the job. Use this ID while submitting share generated from this job.
	json_t *job_id = json_array_get(params, 0);
	connection->job.job_id = strdup(json_string_value(job_id));
	printf("Checkpoint 1 \n");
	printf("Job id collected: %s \n", connection->job.job_id);

	// prevhash - Hash of previous block.
	// VER SI LO NECESITO PARA ALGO
	json_t *prev_hash = json_array_get(params, 1);
	hex2bin(connection->job.prevhash, json_string_value(prev_hash), 32);
	printf("Checkpoint 2 \n");
	////////////////////////////////////////////////
	// 			Start coinbase building			  //
	////////////////////////////////////////////////
	// coinb1 - Initial part of coinbase transaction.
	json_t *coinb1 = json_array_get(params, 2);
	// coinb2 - Final part of coinbase transaction.
	json_t *coinb2 = json_array_get(params, 3);
	size_t coinb1_size = strlen(json_string_value(coinb1)) / 2;
	size_t coinb2_size = strlen(json_string_value(coinb2)) / 2;
	printf("Checkpoint 3 \n");
	// Total coinbase size
	connection->job.coinbase_size = coinb1_size + connection->nonce1_size + 
		connection->nonce2_size + coinb2_size;
	connection->job.coinbase = realloc(connection->job.coinbase, connection->job.coinbase_size);
	printf("Checkpoint 4 \n");
	// Copies coinbase1 to the start of coinbase
	hex2bin(connection->job.coinbase, json_string_value(coinb1), coinb1_size);
	printf("Checkpoint 5 \n");
	// Copies the nonce1 next to the coinbase1
	memcpy(connection->job.coinbase + coinb1_size, connection->nonce1, connection->nonce1_size);
	printf("Checkpoint 6 \n");
	// Sets the nonce2 pointer where it is in the coinbase array
	connection->job.nonce2 = connection->job.coinbase + coinb1_size + connection->nonce1_size;
	printf("Checkpoint 7 \n");
	// Hace esto???
	if (!connection->job.job_id || strcmp(connection->job.job_id, json_string_value(job_id)))
		memset(connection->job.nonce2, 0, connection->nonce2_size);
	printf("Checkpoint 8 \n");
	// Copies the nonce2 to the coinbase
	hex2bin(connection->job.nonce2 + connection->nonce2_size, 
		json_string_value(coinb2), coinb2_size);
	printf("Checkpoint 9 \n");
	//
	// merkle_branch - List of hashes, will be used for calculation of merkle root.
	// This is not a list of all transactions, it only contains prepared hashes of steps of merkle tree algorithm.
	json_t *merkle_branch = json_array_get(params, 4);
	connection->job.merkle_count = json_array_size(merkle_branch);
	printf("Checkpoint 10 \n");
	// Creates an array of pointers
	connection->job.merkle = realloc(connection->job.merkle, 
		connection->job.merkle_count * sizeof(unsigned char *));
	// For each merkle hash
	for(int i=0;i < connection->job.merkle_count; i++) {
		connection->job.merkle[i] = malloc(sizeof(unsigned char) * 32);
		json_t *merkle_hash = json_array_get(merkle_branch, i);
		hex2bin(connection->job.merkle[i], json_string_value(merkle_hash), 32);
	}
	printf("Checkpoint 11 \n");
	////////////////////////////////////////////////
	// 		   End coinbase building			  //
	////////////////////////////////////////////////

	// version - Bitcoin block version.
	json_t *version = json_array_get(params, 5);
	printf("Checkpoint 12 \n");
	printf("Got version %s \n", json_string_value(version));
	// nbits - Encoded current network difficulty
	json_t *nbits = json_array_get(params, 6);
	printf("Checkpoint 13 \n");
	printf("Got nbits %s \n", json_string_value(nbits));
	// ntime - Current ntime
	json_t *ntime = json_array_get(params, 7);
	printf("Checkpoint 14 \n");
	hex2bin(connection->job.version, (const char *) json_string_value(version), 4);
	printf("Checkpoint 15 \n");
	hex2bin(connection->job.nbits, (const char *) json_string_value(nbits), 4);
	printf("Checkpoint 16 \n");
	hex2bin(connection->job.ntime, (const char *) json_string_value(ntime), 4);
	printf("Checkpoint 17 \n");
	// clean_jobs - When true, server indicates that submitting shares 
	// from previous jobs don't have a sense and such shares will be rejected.
	// When this flag is set, miner should also drop all previous jobs, 
	// so job_ids can be eventually rotated.
	json_t *clean_jobs = json_array_get(params, 8);
	if(json_is_true(clean_jobs)) {
		connection->job.clean = true;
		printf("Server requested to clean jobs \n");
	}
	
}

int main(int argc, char *argv[]) {
	if(strlen(argv[1]) == 0 || strlen(argv[2]) == 0) {
		printf("Parameters should be added \n");
		printf("./protocol server port user password \n");
		return 0;
	}
	printf("Connecting to %s:%s \n Username: %s \n Password: %s \n",
		argv[1], argv[2], argv[3], argv[4]);
	int socket = sock(argv[1], argv[2]);
	printf("Successfully connected \n");
	/* Stratum work */
	struct work work;
	// Creates stratum connection structure
	struct stratum_connection *connection = (struct stratum_connection*) malloc(sizeof(struct stratum_connection));
	connection->socket = socket;
	connection->state = SUBS_SENT;
	connection->send_id = 1;
	stratum_subscribe(connection);
	stratum_authorize(connection, argv[3], argv[4]);
	printf("Antes de crear queue \n");
	struct connection_buffer *recv_queue = queue_create(12288);
	printf("Queue creada \n");
	char *message;
	size_t queue_received_size;
	printf("Starting main loop \n");
	int j = 0;
	while(j< 100) {
		// Recieves data and places it in the queue
		recv_data(connection->socket, recv_queue, queue_free_size(recv_queue));
		// Proccess all the messages in the queue
		j++;
		while(1) {
			queue_received_size = queue_pop(recv_queue, &message, '\n');
			if(j == 100) {
				printf("Printed queue \n");
				print_queue_buffer(recv_queue);
			} 	
			// Nothing to read
			if(queue_received_size == 0) break;
			message[queue_received_size - 1] = '\0';
			printf("Mensaje recibido: \n %s \n", message);
			printf("Recieved message \n");
			// Process message
			json_error_t *decode_error = NULL;
			json_t *json_obj = json_loads(message, JSON_DECODE_ANY, decode_error);
			if(!json_obj) {
				printf("Error decoding JSON subscription \n");
				printf("%s \n", decode_error->text);
				printf("Error found in: \n");
				printf("%s \n", decode_error->source);
			}
			json_t *id = json_object_get(json_obj, "id");
			// If id is not null, it's a response
			if(!json_is_null(id)) {
				json_int_t id_value = json_integer_value(id);
				json_t *error = json_object_get(json_obj, "error");
				if(!json_is_null(error)) {
					printf("Error in response with id: %lld \n", id_value);
				}
				switch(id_value) {
					case 1:
						if(connection->state == SUBS_SENT) {
							printf("Recieved subscription \n");
							connection->state = AUTH_SENT;
							stratum_load_subscription(connection, json_obj);
						}
						else {
							printf("Error no deberia estar este mensaje \n");
							// Matar todo?
						}
						break;
					case 2:
						if(connection->state == AUTH_SENT) {
							printf("Recieved authorization \n");
							connection->state = AUTHORIZED;
							json_t *result = json_object_get(json_obj, "result");
							if(!json_is_true(result)) {
								printf("Error subscribing user \n");
								// Matar todo?
							}
						}
						else {
							printf("Error no deberia estar este mensaje \n");
							// Matar todo?
						}
						break;
					default:
						printf("Seguramente es un mensaje de aceptado de shares \n");
						break;
					//Tiene que ser un mensaje de aceptado de submit shares

				}
			}
			else {
				// Recibo oferta de trabajo o cambiar dificultad, etc
				json_t *method = json_object_get(json_obj, "method");
				if(!strcmp(json_string_value(method), "mining.notify")) {
					printf("Recibi un notify \n");
					stratum_notify(connection, json_obj);
					create_new_job(connection, &work);
				}
				else if(!strcmp(json_string_value(method), "mining.set_difficulty")) {
					printf("Recibi un cambio de dificultad \n");
				}
			}
			json_decref(json_obj);
			free(message);
		}
	}

	queue_free(recv_queue);
	close(connection->socket);
	return 0;
}