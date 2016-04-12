#include "worker.h"

void *worker_thread(void *arg) {
	// /* Store worker's data */
	struct worker_data *worker_data = (struct worker_data *) arg;
	// /* Calculates end_nonce ? */
	uint32_t end_nonce = 0xffffffffU; // / worker_data->thr_ammount * (worker_data->thr_id + 1) - 0x20;
	// /* Bind to cpu */
	struct work work;
	work.job_id = NULL;
	work.nonce2 = NULL;

	/* Setting sha256_scan algorithm data */
	uint32_t w[64];
	uint32_t fst_state[16];
	uint32_t hash_result[8];
	/* Initialize hash_result to be sure we don't have any bugs */
	memset(hash_result, 0xff, 32);
	memset(work.target, 0xff, 32);
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
		pthread_mutex_lock(&worker_data->stats_mutex);
		if(worker_data->stats.running == KILL) {
			printf("Thread %d has been signaled to die \n", worker_data->thr_id);
			pthread_mutex_unlock(&worker_data->stats_mutex);
			break;
		}
		else if(worker_data->stats.running == RESET) {
			worker_data->stats.running = RUN;
			printf("Thread %d generating new work \n", worker_data->thr_id);
			stratum_generate_new_work(worker_data->context, &work);
			// work.data[19] = 0xffffffffU / worker_data->thr_ammount * worker_data->thr_id;
			work.data[19] = 0;
		}
		pthread_mutex_unlock(&worker_data->stats_mutex);
		if(work.data[NONCE] >= end_nonce) {
			printf("Se lleno la nonce!\n");
			stratum_generate_new_work(worker_data->context, &work);
		}
		// sha256d_scan(fst_state, hash_result, work.data, w);
		asm_sha256d_scan(fst_state, hash_result, work.data, w);
		if(hash_result[7] < work.target[7]) {
			printf("Encontro algo![7][7], viva la pepa -------------------------------------\n");
			if(fulltest(hash_result, work.target))
				stratum_submit_share(worker_data->connection, worker_data->context, &work);
		}
		if(hash_result[0] < work.target[7]) {
			printf("Encontro algo [0][7]!, viva la pepa -------------------------------------\n");
			if(fulltest(hash_result, work.target))
				stratum_submit_share(worker_data->connection, worker_data->context, &work);
		}
		if(hash_result[0] < work.target[0]) {
			printf("Encontro algo[0][0]!, viva la pepa -------------------------------------\n");
			if(fulltest(hash_result, work.target))
				stratum_submit_share(worker_data->connection, worker_data->context, &work);
		}
		++work.data[NONCE];
		++hashes_done;
	}
	printf("Killing process %d \n", worker_data->thr_id);
	// if(work.job_id)
	// 	free(work.job_id);
	// if(work.coinbase)
	// 	free(work.coinbase);
	pthread_exit(NULL);
	return NULL;
}

void reset_threads(struct worker_data *worker_data, unsigned int thr_ammount) {
	for (int i = 0; i < thr_ammount; ++i) {
		pthread_mutex_lock(&worker_data[i].stats_mutex);
		worker_data[i].stats.running = RESET;
		pthread_mutex_unlock(&worker_data[i].stats_mutex);
	}
}

void kill_threads(pthread_t *thr, struct worker_data *worker_data, unsigned int thr_ammount) {
	/* Tell the threads to die */
	for (int i = 0; i < thr_ammount; ++i) {
		pthread_mutex_lock(&(worker_data[i].stats_mutex));
		worker_data[i].stats.running = KILL;
		pthread_mutex_unlock(&(worker_data[i].stats_mutex));
	}
	/* block until all threads complete */
	for (int i = 0; i < thr_ammount; ++i) {
		pthread_join(thr[i], NULL);
	}
	free(thr);
	free(worker_data);
}

bool create_threads(pthread_t **thr, struct worker_data **worker_data, 
	struct stratum_connection *connection, 
	struct stratum_context *context, unsigned int thr_ammount) {
	int rc;
	(*thr) = malloc(sizeof(pthread_t) * thr_ammount);
	(*worker_data) = malloc(sizeof(struct worker_data) * thr_ammount);
	for (int i = 0; i < thr_ammount; ++i) {
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