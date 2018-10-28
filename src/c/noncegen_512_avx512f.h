#pragma once

#include <stdint.h>
#include <stdlib.h>

#define HASH_SIZE 32
#define HASH_CAP 4096
#define NUM_SCOOPS 4096
#define SCOOP_SIZE 64
#define NONCE_SIZE (HASH_CAP * SCOOP_SIZE)  // 4096*64

void init_shabal_avx512();
void noncegen_avx512(char *cache, const size_t cache_size, const size_t chunk_offset,
                   const unsigned long long numeric_id, const unsigned long long local_startnonce,
                   const unsigned long long local_nonces);
