#ifndef _SENDER_H
#define _SENDER_H

#include <pthread.h>
#include "protocol.h"

pthread_t * create_sender(struct stratum_connection* connection);

#endif /* sender.h */
