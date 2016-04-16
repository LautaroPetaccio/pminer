#ifndef _WORKER_H
#define _WORKER_H
#include <pthread.h>
#include <time.h>
#include "work.h"
#include "protocol.h"
#include "sha256.h"
#include "util.h"

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
void generate_new_work(struct stratum_context *context, struct work* work);
struct work *create_work();
void destruct_work(struct work *work);

extern void asm_sha256d_scan(uint32_t *fst_state, uint32_t *snd_state, const uint32_t *data);

#endif /* worker.h */
