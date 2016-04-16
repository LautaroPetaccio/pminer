#ifndef _WORK_H
#define _WORK_H
#include <stdint.h>

typedef struct block_header {
        unsigned int    version;
        uint32_t prev_block[8];
        uint32_t merkle_root[8];
        unsigned int    timestamp;
        unsigned int    bits;
        unsigned int    nonce;
        uint32_t padding[12];
} block_header;
 
struct work {
  block_header block_header;
  uint32_t target[8];
  char *job_id;
  size_t nonce2_size;
  unsigned char *nonce2;
};

#endif
