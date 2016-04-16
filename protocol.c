#include "protocol.h"

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

struct stratum_context* create_stratum_context() {
		struct stratum_context *context = 
		(struct stratum_context*) malloc(sizeof(struct stratum_context));
		context->user = NULL;
		context->session_id = NULL;
		context->nonce1 = NULL;
		context->job.job_id = NULL;
		context->job.coinbase = NULL;
		context->job.nonce2 = NULL;
		context->job.merkle_root = NULL;
		context->job.merkle = NULL;
		context->job.merkle_count = 0;
		context->job.clean = false;
		context->state = SUBS_SENT;
		pthread_mutex_init(&context->context_mutex, NULL);
		return context;
}

struct stratum_connection* create_stratum_connection(char *ip, char *port) {
	int socket = sock(ip, port);
	if(socket < 0) return NULL;
	struct stratum_connection *connection = 
		(struct stratum_connection*) malloc(sizeof(struct stratum_connection));
	connection->recv_queue = queue_create(12288);
	connection->send_queue = queue_create(12288);
	pthread_mutex_init(&connection->send_queue_mutex, NULL);
	connection->send_id = 1;
	connection->socket = socket;
	return connection;
}

bool add_to_send_queue(struct stratum_connection* connection, char *data, size_t size) {
	if(queue_free_size(connection->send_queue) >= size) {
		queue_push(connection->send_queue, data, size);
	}
	else
		return false;
	return true;
}

int send_messages(struct stratum_connection* connection) {
	pthread_mutex_lock(&connection->send_queue_mutex);
	size_t size_to_send = queue_used_size(connection->send_queue);
	if(size_to_send == 0) {
		pthread_mutex_unlock(&connection->send_queue_mutex);
		return 0;
	}
	int sent_data_size = 0;
	sent_data_size = write(connection->socket, (void *) connection->send_queue->buffer, size_to_send);
	if(sent_data_size < 0) {
		fprintf(stderr, "ERROR writing to socket, wrote 0 bytes\n");
		pthread_mutex_unlock(&connection->send_queue_mutex);
		return sent_data_size;
	}
	else {
		// connection->send_queue->buffer[sent_data_size + 1] = 0;
		char mensaje[sent_data_size + 1];
		memcpy(mensaje, connection->send_queue->buffer, sent_data_size);
		mensaje[sent_data_size] = 0;
		printf("Message sent: %s with size: %d \n", mensaje, sent_data_size);
	}
	if(sent_data_size > 0)
		queue_pop_size(connection->send_queue, (size_t) sent_data_size);
	pthread_mutex_unlock(&connection->send_queue_mutex);
	return sent_data_size;
}

unsigned int recv_data(struct stratum_connection* connection) {
	unsigned int recv_size = queue_free_size(connection->recv_queue);
	char recv_buffer[recv_size];
	int recieved_size = read(connection->socket, (void *) recv_buffer, recv_size);
	if(recieved_size < 0) {
		fprintf(stderr, "ERROR reading from socket\n");
		exit(0);
	}
	queue_push(connection->recv_queue, recv_buffer, recieved_size);
	return recieved_size;
}

size_t get_message(struct stratum_connection* connection, char **message) {
	recv_data(connection);
	return queue_pop(connection->recv_queue, message, '\n');
}


void destruct_stratum_context(struct stratum_context *context) {
	if(context->session_id) free(context->session_id);
	if(context->nonce1) free(context->nonce1);
	if(context->job.coinbase) free(context->job.coinbase);
	if(context->job.job_id) free(context->job.job_id);
	for (size_t i = 0; i < context->job.merkle_count; i++)
		free(context->job.merkle[i]);
	if(context->job.merkle) free(context->job.merkle);
	free(context);
}

/*
	destruct_stratum_connection: destructs a connection created with crete_stratum_connection
*/

void destruct_stratum_connection(struct stratum_connection *connection) {
	queue_free(connection->recv_queue);
	queue_free(connection->send_queue);
	close(connection->socket);
	free(connection);
}

void stratum_generate_new_work(struct stratum_context *context, struct work* work) {
	pthread_mutex_lock(&context->context_mutex);
	uint8_t merkle_root[64];
	/* Copy the coinbase to the work */
	if(work->job_id)
		free(work->job_id);
	work->job_id = strdup(context->job.job_id);
	work->nonce2_size = context->nonce2_size;
	work->nonce2 = realloc(work->nonce2, context->nonce2_size);
	memcpy(work->nonce2, context->job.nonce2, context->nonce2_size);

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
	sha256d(context->job.coinbase, context->job.coinbase_size, merkle_root);
	for(size_t i = 0; i < context->job.merkle_count; i++) {
		memcpy(merkle_root + 32, context->job.merkle[i], 32);
		sha256d(merkle_root, 64, merkle_root);
	}
	/* Increment extranonce2 to have different works */
	/* Assemble block header */
	memset(work->data, 0, 128);
	/* Version */
	work->data[0] = le32dec(context->job.version);
	/* hashPrevBlock */
	for(int i = 0; i < 8; i++)
		work->data[1 + i] = le32dec((uint32_t *)context->job.prevhash + i);

	/* hashMerkleRoot */
	for(int i = 0; i < 8; i++)
		// work->data[9 + i] = swap_uint32(*((uint32_t *) merkle_root + i));
		work->data[9 + i] = be32dec((uint32_t *)merkle_root + i);

	/* Little endian Time */
	work->data[17] = le32dec(context->job.ntime);
	/* Little endian Bits */
	work->data[18] = le32dec(context->job.nbits);
	/* work->data[19]; Nonce */
	/* Padding data, optimizing the transform */
	work->data[20] = 0x80000000;
	work->data[31] = 0x00000280;
	diff_to_target(work->target, context->job.diff);
	pthread_mutex_unlock(&context->context_mutex);
}

/*
	stratum_set_next_job_difficulty: gets the json object regarding to the difficulty
	change and store it as the next difficulty for the next job.
*/

void stratum_set_next_job_difficulty(struct stratum_context *context, json_t *json_message) {
	json_t *params = json_object_get(json_message, "params");
	json_t *difficulty = json_array_get(params, 0);
	context->next_diff = json_number_value(difficulty);
}

/*
	stratum_subscribe: creates and sends subscription.
*/

void stratum_subscribe(struct stratum_connection *connection) {
	/* Lock send_queue until the message is added and the send_id is incremented */
	pthread_mutex_lock(&connection->send_queue_mutex);
	json_t *root = json_object();
	json_object_set_new(root, "id", json_integer(connection->send_id));
	json_object_set_new(root, "method", json_string("mining.subscribe"));
	json_t *params_array = json_array();
	json_array_append_new(params_array, json_string("Pminer/0.1"));
	json_object_set_new(root, "params", params_array);
	char *res_string = json_dumps(root, JSON_ENCODE_ANY | JSON_PRESERVE_ORDER);
	size_t res_string_length = strlen(res_string);
	//Converts endstring character to newline to be read by the stratum server
	res_string[res_string_length] = '\n';
	add_to_send_queue(connection, res_string, res_string_length + 1);
	connection->send_id++;
	/* Unlock send_queue */
	pthread_mutex_unlock(&connection->send_queue_mutex);
	free(res_string);
	json_decref(root);
	/* Not thread safe */
}

void stratum_client_version(struct stratum_connection *connection, json_t *json_obj) {
	json_t *root = json_object();
	json_object_set(root, "id", json_object_get(json_obj, "id"));
	json_object_set_new(root, "error", json_null());
	json_object_set_new(root, "result", json_string("Pminer/0.1"));
	char *res_string = json_dumps(root, JSON_ENCODE_ANY | JSON_PRESERVE_ORDER);
	size_t message_len = strlen(res_string);
	res_string[message_len] = '\n';
	pthread_mutex_lock(&connection->send_queue_mutex);
	add_to_send_queue(connection, res_string, message_len + 1);
	pthread_mutex_unlock(&connection->send_queue_mutex);
	json_decref(root);
	free(res_string);
	/* Ver que hacer con el ID connection->send_id++; */
}

void stratum_load_subscription(struct stratum_context *context, json_t *json_message) {
	json_t *result = json_object_get(json_message, "result");
	//json_t *error = json_object_get(json_message, "error");
	/* Stores subscription details */
	if(context->session_id) {
		free(context->session_id);
		context->session_id = NULL;
	}
	/* Gets result first element */
	size_t index_one, index_two;
	json_t *value_one, *value_two, *notify;
	json_array_foreach(result, index_one, value_one) {
		if(json_is_string(value_one)) {
			if(!strcmp("mining.notify", json_string_value(value_one))) {
				notify = json_array_get(result, index_one + 1);
				context->session_id = strdup(json_string_value(notify));
				break;
			}
		}
		else if(json_is_array(value_one)) {
			json_array_foreach(value_one, index_two, value_two) {
				if(json_is_string(value_two)) {
					if(!strcmp("mining.notify", json_string_value(value_two))) {
						notify = json_array_get(value_one, index_two + 1);
						context->session_id = strdup(json_string_value(notify));
						break;
					}
				}
			}
		}
	}
	if(!context->session_id) exit(1);

	/* Getting the extranonce1 size */
	context->nonce1_size = strlen(json_string_value(json_array_get(result, 1)))/2;

	/* Storing the extranonce1 */
	context->nonce1 = realloc(context->nonce1, context->nonce1_size * sizeof(char));
	hex2bin(context->nonce1, json_string_value(json_array_get(result, 1)), context->nonce1_size);

	/* Getting the extranonce2 size from the subscription */
	context->nonce2_size = json_integer_value(json_array_get(result, 2));
}

void stratum_authorize(struct stratum_connection *connection,
	const char *user, const char *password) {
	//Send authorize request
	/* Lock send_queue until the message is added and the send_id is incremented */
	pthread_mutex_lock(&connection->send_queue_mutex);
	json_t *root = json_object();
	json_object_set_new(root, "id", json_integer(connection->send_id));
	json_object_set_new(root, "method", json_string("mining.authorize"));
	json_t *array = json_array();
	json_array_append_new(array, json_string(user));
	json_array_append_new(array, json_string(password));
	json_object_set_new(root, "params", array);
	char *res_string = json_dumps(root, JSON_ENCODE_ANY | JSON_PRESERVE_ORDER);
	size_t message_len = strlen(res_string);
	res_string[message_len] = '\n';
	add_to_send_queue(connection, res_string, message_len + 1);
	connection->send_id++;
	/* Unlock send_queue */
	pthread_mutex_unlock(&connection->send_queue_mutex);
	json_decref(root);
	free(res_string);
}

void stratum_submit_share(struct stratum_connection *connection, 
	struct stratum_context *context,  struct work *work) {
	uint32_t ntime, nonce;
	char time_string[9], nonce_string[9], nonce2_string[(context->nonce2_size * 2) + 1];
	le32enc(&ntime, work->data[17]);
	le32enc(&nonce, work->data[19]);
	bin2hex(time_string, (const unsigned char *) (&ntime), 4);
	bin2hex(nonce_string, (const unsigned char *) (&nonce), 4);
	bin2hex(nonce2_string,(const unsigned char *) work->nonce2, work->nonce2_size);

	json_t *root = json_object();
	json_t *params = json_array();
	json_object_set_new(root, "params", params);
	/* Lock send_queue until the message is added and the send_id is incremented */
	pthread_mutex_lock(&connection->send_queue_mutex);
	json_object_set_new(root, "id", json_integer(connection->send_id));
	json_object_set_new(root, "method", json_string("mining.submit"));
	// User, previously authorized
	json_array_append_new(params, json_string(context->user));
	// Job id
	json_array_append_new(params, json_string(context->job.job_id));
	// extra nonce2
	json_array_append_new(params, json_string(nonce2_string));
	json_array_append_new(params, json_string(time_string));
	json_array_append_new(params, json_string(nonce_string));
	char *res_string = json_dumps(root, JSON_ENCODE_ANY | JSON_PRESERVE_ORDER);
	res_string[strlen(res_string)] = '\n';
	add_to_send_queue(connection, res_string, strlen(res_string) + 1);
	connection->send_id++;
	/* Unlock send_queue */
	pthread_mutex_unlock(&connection->send_queue_mutex);
	// El incremento deberia estar en el send?
	json_decref(root);
	free(res_string);
}

void stratum_notify(struct stratum_context *context, json_t *json_obj) {
	pthread_mutex_lock(&context->context_mutex);
	json_t *params = json_object_get(json_obj, "params");
	/* Stores prevhash - Hash of previous block. */
	json_t *prev_hash = json_array_get(params, 1);
	hex2bin(context->job.prevhash, json_string_value(prev_hash), 32);

	////////////////////////////////////////////////
	// 			Start coinbase building			  //
	////////////////////////////////////////////////
	/* coinb1 - Initial part of coinbase transaction. */
	json_t *coinb1 = json_array_get(params, 2);
	/* coinb2 - Final part of coinbase transaction. */
	json_t *coinb2 = json_array_get(params, 3);
	size_t coinb1_size = strlen(json_string_value(coinb1)) / 2;
	size_t coinb2_size = strlen(json_string_value(coinb2)) / 2;
	/* Total coinbase size */
	context->job.coinbase_size = coinb1_size + context->nonce1_size + 
		context->nonce2_size + coinb2_size;
	/* Get memory for the coinbase */
	context->job.coinbase = realloc(context->job.coinbase, context->job.coinbase_size);
	/* Copies the first part of the coinbase (coinbase1) */
	hex2bin(context->job.coinbase, json_string_value(coinb1), coinb1_size);
	/* Copies the nonce1 next to the coinbase1 */
	memcpy(context->job.coinbase + coinb1_size, context->nonce1, context->nonce1_size);
	/* Sets the nonce2 pointer where it is in the coinbase array */
	context->job.nonce2 = context->job.coinbase + coinb1_size + context->nonce1_size;
	/* ID of the job - Used to submit shares */
	json_t *job_id = json_array_get(params, 0);
	/* If job_id changed or never setted, reset nonce2 */
	if(context->job.job_id) {
		if(strcmp(context->job.job_id, json_string_value(job_id))) {
			memset(context->job.nonce2, 0, context->nonce2_size);
			free(context->job.job_id);
			/* Copies new job_id */
			context->job.job_id = strdup(json_string_value(job_id));
		}
	}
	else {
		/* Copies new job_id */
		memset(context->job.nonce2, 0, context->nonce2_size);
		context->job.job_id = strdup(json_string_value(job_id));
	}

	/* Copies the coinbase2 next to the nonce2 */
	hex2bin(context->job.nonce2 + context->nonce2_size, 
		json_string_value(coinb2), coinb2_size);

	////////////////////////////////////////////////
	// 		   End coinbase building			  //
	////////////////////////////////////////////////

	////////////////////////////////////////////////
	//         Start merkle tree building         //
	////////////////////////////////////////////////

	/* Deletes old merkle hashes */
	for (size_t i = 0; i < context->job.merkle_count; i++)
		free(context->job.merkle[i]);
	/* Stores hashes used to compute the merkle root */
	json_t *merkle_branch = json_array_get(params, 4);
	context->job.merkle_count = json_array_size(merkle_branch);
	/* Creates the array of pointers that will hold the hashes */
	context->job.merkle = realloc(context->job.merkle, 
		context->job.merkle_count * sizeof(unsigned char *));
	/* For each merkle hash, store the hashes */
	for(size_t i=0;i < context->job.merkle_count; i++) {
		context->job.merkle[i] = malloc(sizeof(unsigned char) * 32);
		json_t *merkle_hash = json_array_get(merkle_branch, i);
		hex2bin(context->job.merkle[i], json_string_value(merkle_hash), 32);
	}

	////////////////////////////////////////////////
	//          End merkle tree building          //
	////////////////////////////////////////////////

	/* version - Bitcoin block version.*/
	json_t *version = json_array_get(params, 5);
	/* nbits - Encoded current network difficulty */
	json_t *nbits = json_array_get(params, 6);
	/* ntime - Current ntime */
	json_t *ntime = json_array_get(params, 7);
	hex2bin(context->job.version, (const char *) json_string_value(version), 4);
	hex2bin(context->job.nbits, (const char *) json_string_value(nbits), 4);
	hex2bin(context->job.ntime, (const char *) json_string_value(ntime), 4);
	/*
		clean_jobs - When true, server indicates that submitting shares 
		from previous jobs don't have a sense and such shares will be rejected.
		When this flag is set, miner should also drop all previous jobs, 
		so job_ids can be eventually rotated.
	*/
	json_t *clean_jobs = json_array_get(params, 8);
	if(json_is_true(clean_jobs) || context->next_diff < context->job.diff) {
		context->job.clean = true;
		if(context->next_diff < context->job.diff)
			printf("Resetting jobs due to better difficulty \n");
		else
			printf("Server requested to clean jobs \n");
	}
	context->job.diff = context->next_diff;
	pthread_mutex_unlock(&context->context_mutex);
}
