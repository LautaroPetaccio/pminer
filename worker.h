#ifndef _WORKER_H
#define _WORKER_H
#include "protocol.h"
#include "sha256.h"
#include "util.h"
#include <pthread.h>
#include <time.h>

#define NONCE 19

typedef enum {RUN, RESET, KILL} running_stats;

struct worker_stats {
	double hashrate;
	running_stats running;
};

struct worker_data {
  int thr_id;
  unsigned int thr_ammount;
  unsigned int coinbase2_increment;
  pthread_mutex_t stats_mutex;
  struct worker_stats stats;
  struct stratum_connection *connection;
  struct stratum_context *context;
};

void kill_threads(pthread_t *thr, struct worker_data *worker_data, unsigned int thr_ammount);
bool create_threads(pthread_t **thr, struct worker_data **worker_data, 
	struct stratum_connection *connection, 
	struct stratum_context *context, unsigned int thr_ammount);
void reset_threads(struct worker_data *worker_data, unsigned int thr_ammount);

extern void asm_sha256d_scan(uint32_t *fst_state, uint32_t *snd_state, const uint32_t *data);

#endif /* worker.h */
