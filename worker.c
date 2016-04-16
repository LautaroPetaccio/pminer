#include "worker.h"

struct work *create_work() {
	struct work *work = malloc(sizeof(struct work));
	memset(work->target, 0xff, 32);
	work->job_id = NULL;
	work->nonce2 = NULL;
	memset(&(work->block_header), 0, 128);
	work->block_header.padding[0] = 0x80000000;
	work->block_header.padding[11] = 0x00000280;
	return work;
}

void destruct_work(struct work *work) {
	if(work->job_id)
		free(work->job_id);
	free(work);
}

void generate_new_work(struct stratum_context *context, struct work* work) {
	pthread_mutex_lock(&context->context_mutex);
	uint8_t merkle_root[64];
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

	/* Fills block header */

	/* Version */
	work->block_header.version = le32dec(context->job.version);
	/* hashPrevBlock */
	for(int i = 0; i < 8; i++)
		work->block_header.prev_block[i] = le32dec((uint32_t *) context->job.prevhash + i);
	/* hashMerkleRoot */
	for(int i = 0; i < 8; i++)
		work->block_header.merkle_root[i] = be32dec((uint32_t *) merkle_root + i);
	/* Little endian Time */
	work->block_header.timestamp = le32dec(context->job.ntime);
	/* Little endian Bits */
	work->block_header.bits = le32dec(context->job.nbits);
	/* Nonce */
	work->block_header.nonce = 0;

	diff_to_target(work->target, context->job.diff);
	pthread_mutex_unlock(&context->context_mutex);
}

void *worker_thread(void *arg) {
	/* Store worker's data */
	struct worker_data *worker_data = (struct worker_data *) arg;
	/* Calculates end_nonce */
	uint32_t end_nonce = 0xffffffffU;
	// /* Bind to cpu */
	struct work *work = create_work();
	/* Setting sha256_scan algorithm data */
	uint32_t w[64];
	uint32_t fst_state[16];
	uint32_t hash_result[8];
	/* Initialize hash_result to be sure we don't have any bugs */
	memset(hash_result, 0xff, 32);
	/* Setting fst_state padding */
	fst_state[8] = 0x80000000;
	memset(fst_state + 9, 0, 24);
	fst_state[15] = 0x00000100;

	printf("Worker %d started \n", worker_data->thr_id);
 	clock_t begin, end;
 	unsigned int hashes_done = 0;
 	double elapsed_seconds;
 	begin = clock();
	while(true) {
		end = clock();
		elapsed_seconds = (double)(end - begin) / CLOCKS_PER_SEC;
		if(elapsed_seconds > 160) {
			if(hashes_done > 1000) {
				hashes_done /= 1000;
				printf("Hashes per second in worker %d: %.1f kh/s \n", worker_data->thr_id, hashes_done / elapsed_seconds);
			}
			else
				printf("Hashes per second in worker %d: %f \n", worker_data->thr_id, hashes_done / elapsed_seconds);
			hashes_done = 0;
			elapsed_seconds = 0;
			begin = clock();
		}
		if(worker_data->stats.running == RESET || work->block_header.nonce >= end_nonce) {
			worker_data->stats.running = RUN;
			printf("Thread %d generating new work \n", worker_data->thr_id);
			// stratum_generate_new_work(worker_data->context, work);
			generate_new_work(worker_data->context, work);
		}
		// sha256d_scan(fst_state, hash_result, work->data, w);
		asm_sha256d_scan(fst_state, hash_result, (uint32_t *) &work->block_header);
		if(hash_result[7] < work->target[7]) {
			printf("Share found, queueing for send \n");
			if(fulltest(hash_result, work->target))
				stratum_submit_share(worker_data->connection, worker_data->context, work);
		}
		++work->block_header.nonce;
		++hashes_done;
	}
	printf("Killing process %d \n", worker_data->thr_id);
	destruct_work(work);
	pthread_exit(NULL);
	return NULL;
}

void reset_threads(struct worker_data *worker_data, uint32_t thr_ammount) {
	for (uint32_t i = 0; i < thr_ammount; ++i) {
		pthread_mutex_lock(&worker_data[i].stats_mutex);
		worker_data[i].stats.running = RESET;
		pthread_mutex_unlock(&worker_data[i].stats_mutex);
	}
}

void kill_threads(pthread_t *thr, struct worker_data *worker_data, uint32_t thr_ammount) {
	/* Tell the threads to die */
	for (uint32_t i = 0; i < thr_ammount; ++i) {
		pthread_mutex_lock(&(worker_data[i].stats_mutex));
		worker_data[i].stats.running = KILL;
		pthread_mutex_unlock(&(worker_data[i].stats_mutex));
	}
	/* block until all threads complete */
	for (uint32_t i = 0; i < thr_ammount; ++i) {
		pthread_join(thr[i], NULL);
	}
	free(thr);
	free(worker_data);
}

bool create_threads(pthread_t **thr, struct worker_data **worker_data, 
	struct stratum_connection *connection, 
	struct stratum_context *context, uint32_t thr_ammount) {
	int rc;
	(*thr) = malloc(sizeof(pthread_t) * thr_ammount);
	(*worker_data) = malloc(sizeof(struct worker_data) * thr_ammount);
	for (uint32_t i = 0; i < thr_ammount; ++i) {
		(*worker_data)[i].thr_id = i;
		(*worker_data)[i].stats.running = RESET;
		(*worker_data)[i].thr_ammount = thr_ammount;
		(*worker_data)[i].connection = connection;
		(*worker_data)[i].context = context;
		if((rc = pthread_mutex_init(&((*worker_data)[i].stats_mutex), NULL))) {
			printf("Could not create mutex ! %d \n", rc);
		}
		/* Add things to thread data */
		if((rc = pthread_create(&(*thr)[i], NULL, worker_thread, &(*worker_data)[i]))) {
			fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
			return false;
		}
	}
	return true;
}
