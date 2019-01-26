#include "noncegen_32.h"
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "sph_shabal.h"

// cache:		cache to save to
// local_num:		thread number
// numeric_id:		numeric account id
// loc_startnonce	nonce to start generation at
// local_nonces: 	number of nonces to generate
void noncegen(char *cache, const size_t cache_size, const size_t chunk_offset,
              const uint64_t numeric_id, const uint64_t local_startnonce,
              const uint64_t local_nonces) {
    uint64_t nonce;

    char seed[32];  // 64bit numeric account ID, 64bit nonce (blank), 1bit termination, 127 bits zero
    char term[32];  // 1bit 1, 255bit of zeros
    char zero[32];  // 256bit of zeros

    write_seed(seed, numeric_id);
    write_term(term);
    memset(&zero[0], 0, 32);

    uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * (NONCE_SIZE));
    uint8_t *final = (uint8_t *)malloc(sizeof(uint8_t) * HASH_SIZE);

    // prepare smart termination strings
    // creation could further be optimized, but not much in it as it only runs once per work package
    // creation could also be moved to plotter start
    union {
        sph_u32 words[16];
    } t1, t2, t3;

    for (int j = 0; j < 8; j += 1) {
        size_t o = j * 4;
        // t1
        t1.words[j + 0] = *(sph_u32 *)(seed + o);
        t1.words[j + 0 + 8] = *(sph_u32 *)(zero + o);
        // t2
        // (first 256bit skipped, will later be filled with data)
        t2.words[j + 0 + 8] = *(sph_u32 *)(seed + o);
        // t3
        t3.words[j + 0] = *(sph_u32 *)(term + o);
        t3.words[j + 0 + 8] = *(sph_u32 *)(zero + o);
    }

    for (uint64_t n = 0; n < local_nonces;) {
        // generate nonce numbers & change endianness
        nonce = bswap_64((local_startnonce + n));

        // store nonce numbers in relevant termination strings
        for (int j = 2; j < 4; j += 1) {
            size_t o = j * 4 - 8; 
            t1.words[j + 0] = *(sph_u32 *)((char *)&nonce + o);
            t2.words[j + 0 + 8] = *(sph_u32 *)((char *)&nonce + o);
        }

        // start shabal rounds

        // 3 cases: first 128 rounds uses case 1 or 2, after that case 3
        // case 1: first 128 rounds, hashes are even: use termination string 1
        // case 2: first 128 rounds, hashes are odd: use termination string 2
        // case 3: round > 128: use termination string 3
        // round 1
        sph_shabal_hash_fast(NULL, &t1, &buffer[NONCE_SIZE - HASH_SIZE], 16 >> 6);

        // store first hash into smart termination string 2
        memcpy(&t2, &buffer[NONCE_SIZE - HASH_SIZE], HASH_SIZE);

        // round 2 - 128
        for (size_t i = NONCE_SIZE - HASH_SIZE; i > (NONCE_SIZE - HASH_CAP); i -= HASH_SIZE) {
            // check if msg can be divided into 512bit packages without a
            // remainder
            if (i % 64 == 0) {
                // last msg = seed + termination
                sph_shabal_hash_fast(&buffer[i], &t1, &buffer[i - HASH_SIZE],
                                          (NONCE_SIZE + 16 - i) >> 6);
            } else {
                // last msg = 256 bit data + seed + termination
                sph_shabal_hash_fast(&buffer[i], &t2, &buffer[i - HASH_SIZE],
                                          (NONCE_SIZE + 16 - i) >> 6);
            }
        }

        // round 128-8192
        for (size_t i = NONCE_SIZE - HASH_CAP; i > 0; i -= HASH_SIZE) {
            sph_shabal_hash_fast(&buffer[i], &t3, &buffer[i - HASH_SIZE], (HASH_CAP) >> 6);
        }

        // generate final hash
        sph_shabal_hash_fast(&buffer[0], &t1, &final[0], (NONCE_SIZE + 16) >> 6);

        // XOR with final
        for (size_t i = 0; i < NONCE_SIZE; i++) buffer[i] ^= (final[i % HASH_SIZE]);

        // Sort them PoC2:
        for (size_t i = 0; i < HASH_CAP; i++) {
            memmove(&cache[i * cache_size * SCOOP_SIZE + (n + chunk_offset) * SCOOP_SIZE],
                    &buffer[i * SCOOP_SIZE], HASH_SIZE);
            memmove(
                &cache[(4095 - i) * cache_size * SCOOP_SIZE + (n + chunk_offset) * SCOOP_SIZE + 32],
                &buffer[i * SCOOP_SIZE + 32], HASH_SIZE);
        }
        n++;
    }
    free(buffer);
    free(final);
}
