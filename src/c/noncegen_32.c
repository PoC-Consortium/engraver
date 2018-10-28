#include "noncegen_32.h"
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "sph_shabal.h"
sph_shabal_context global_32;

void init_shabal() { sph_shabal256_init(&global_32); }

// cache:			cache to save to
// local_num:		thread number
// numeric_id:		numeric account id
// loc_startnonce	nonce to start generation at
// local_nonces: 	number of nonces to generate
void noncegen(char *cache, const size_t cache_size, const size_t chunk_offset,
              const unsigned long long numeric_id, const unsigned long long local_startnonce,
              const unsigned long long local_nonces) {
    sph_shabal_context local_32;
    unsigned long long nonce;
    size_t len;

    unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char) * (NONCE_SIZE + 16));
    unsigned char *final = (unsigned char *)malloc(sizeof(unsigned char) * HASH_SIZE);

    for (unsigned long long n = 0; n < local_nonces;) {
        int8_t *xv = (int8_t *)&numeric_id;

        for (size_t i = 0; i < 8; i++) buffer[NONCE_SIZE + i] = xv[7 - i];

        nonce = local_startnonce + n;
        xv = (int8_t *)&nonce;

        for (size_t i = 8; i < 16; i++) buffer[NONCE_SIZE + i] = xv[15 - i];

        for (size_t i = NONCE_SIZE; i > 0; i -= HASH_SIZE) {
            memcpy(&local_32, &global_32, sizeof(global_32));

            if (i < NONCE_SIZE + 16 - HASH_CAP)
                len = HASH_CAP;
            else
                len = NONCE_SIZE + 16 - i;

            sph_shabal256(&local_32, &buffer[i], len);
            sph_shabal256_close(&local_32, &buffer[i - HASH_SIZE]);
        }
        memcpy(&local_32, &global_32, sizeof(global_32));
        sph_shabal256(&local_32, buffer, 16 + NONCE_SIZE);
        sph_shabal256_close(&local_32, final);
        
        // XOR with final
        for (size_t i = 0; i < NONCE_SIZE; i++) buffer[i] ^= (final[i % HASH_SIZE]);
        
        // Sort them PoC2:
        for (size_t i = 0; i < HASH_CAP; i++){
            memmove(&cache[i * cache_size * SCOOP_SIZE + (n + chunk_offset) * SCOOP_SIZE], &buffer[i * SCOOP_SIZE], HASH_SIZE);
            memmove(&cache[(4095-i) * cache_size * SCOOP_SIZE + (n + chunk_offset) * SCOOP_SIZE + 32], &buffer[i * SCOOP_SIZE + 32], HASH_SIZE);
        }
        n++;
    }
    free(buffer);
    free(final);
}