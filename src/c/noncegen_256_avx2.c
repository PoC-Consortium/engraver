#include "noncegen_256_avx2.h"
#include <immintrin.h>
#include <string.h>
#include "common.h"
#include "mshabal_256_avx2.h"
#include "sph_shabal.h"

sph_shabal_context global_32;
mshabal256_context global_256;
mshabal256_context_fast global_256_fast;

void init_shabal_avx2() {
    sph_shabal256_init(&global_32);
    mshabal256_init(&global_256, 256);
    global_256_fast.out_size = global_256.out_size;
    for (int i = 0; i < 352; i++) global_256_fast.state[i] = global_256.state[i];
    global_256_fast.Whigh = global_256.Whigh;
    global_256_fast.Wlow = global_256.Wlow;
}

// cache:		cache to save to
// local_num:		thread number
// numeric_id:		numeric account id
// loc_startnonce	nonce to start generation at
// local_nonces: 	number of nonces to generate
void noncegen_avx2(char *cache, const size_t cache_size, const size_t chunk_offset,
                   const uint64_t numeric_id, const uint64_t local_startnonce,
                   const uint64_t local_nonces) {
    sph_shabal_context local_32;
    uint64_t nonce;
    size_t len;

    mshabal256_context_fast local_256_fast;
    uint64_t nonce1, nonce2, nonce3, nonce4, nonce5, nonce6, nonce7, nonce8;

    char seed[32];  // 64bit numeric account ID, 64bit nonce (blank), 1bit termination, 127 bits zero
    char term[32];  // 1bit 1, 255bit of zeros
    char zero[32];  // 256bit of zeros

    write_seed(seed, numeric_id);
    write_term(term);
    memset(&zero[0], 0, 32);

    //vars shared
    uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t) * MSHABAL256_VECTOR_SIZE * NONCE_SIZE);
    uint8_t* final = (uint8_t*)malloc(sizeof(uint8_t) * MSHABAL256_VECTOR_SIZE * HASH_SIZE);

    // prepare smart SIMD aligned termination strings
    // creation could further be optimized, but not much in it as it only runs once per work package
    // creation could also be moved to plotter start
    union {
        mshabal_u32 words[64 * MSHABAL256_FACTOR];
        __m256i data[16];
    } t1, t2, t3;

    for (int j = 0; j < 64 * MSHABAL256_FACTOR / 2; j += 4 * MSHABAL256_FACTOR) {
        size_t o = j / MSHABAL256_FACTOR;
        // t1
        t1.words[j + 0] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 1] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 2] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 3] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 4] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 5] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 6] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 7] = *(mshabal_u32 *)(seed + o);
        t1.words[j + 0 + 64] = *(mshabal_u32 *)(zero + o);
        t1.words[j + 1 + 64] = *(mshabal_u32 *)(zero + o);
        t1.words[j + 2 + 64] = *(mshabal_u32 *)(zero + o);
        t1.words[j + 3 + 64] = *(mshabal_u32 *)(zero + o);
        t1.words[j + 4 + 64] = *(mshabal_u32 *)(zero + o);
        t1.words[j + 5 + 64] = *(mshabal_u32 *)(zero + o);
        t1.words[j + 6 + 64] = *(mshabal_u32 *)(zero + o);
        t1.words[j + 7 + 64] = *(mshabal_u32 *)(zero + o);
        // t2
        // (first 256bit skipped, will later be filled with data)
        t2.words[j + 0 + 64] = *(mshabal_u32 *)(seed + o);
        t2.words[j + 1 + 64] = *(mshabal_u32 *)(seed + o);
        t2.words[j + 2 + 64] = *(mshabal_u32 *)(seed + o);
        t2.words[j + 3 + 64] = *(mshabal_u32 *)(seed + o);
        t2.words[j + 4 + 64] = *(mshabal_u32 *)(seed + o);
        t2.words[j + 5 + 64] = *(mshabal_u32 *)(seed + o);
        t2.words[j + 6 + 64] = *(mshabal_u32 *)(seed + o);
        t2.words[j + 7 + 64] = *(mshabal_u32 *)(seed + o);
        // t3
        t3.words[j + 0] = *(mshabal_u32 *)(term + o);
        t3.words[j + 1] = *(mshabal_u32 *)(term + o);
        t3.words[j + 2] = *(mshabal_u32 *)(term + o);
        t3.words[j + 3] = *(mshabal_u32 *)(term + o);
        t3.words[j + 4] = *(mshabal_u32 *)(term + o);
        t3.words[j + 5] = *(mshabal_u32 *)(term + o);
        t3.words[j + 6] = *(mshabal_u32 *)(term + o);
        t3.words[j + 7] = *(mshabal_u32 *)(term + o);
        t3.words[j + 0 + 64] = *(mshabal_u32 *)(zero + o);
        t3.words[j + 1 + 64] = *(mshabal_u32 *)(zero + o);
        t3.words[j + 2 + 64] = *(mshabal_u32 *)(zero + o);
        t3.words[j + 3 + 64] = *(mshabal_u32 *)(zero + o);
        t3.words[j + 4 + 64] = *(mshabal_u32 *)(zero + o);
        t3.words[j + 5 + 64] = *(mshabal_u32 *)(zero + o);
        t3.words[j + 6 + 64] = *(mshabal_u32 *)(zero + o);
        t3.words[j + 7 + 64] = *(mshabal_u32 *)(zero + o);
    }

    for (uint64_t n = 0; n < local_nonces;) {
        // iterate nonces (8 per cycle - avx2)
        // min 8 nonces left for avx 2 processing, otherwise SISD
        if (n + 8 <= local_nonces) {
            // generate nonce numbers & change endianness
            nonce1 = bswap_64((uint64_t)(local_startnonce + n + 0));
            nonce2 = bswap_64((uint64_t)(local_startnonce + n + 1));
            nonce3 = bswap_64((uint64_t)(local_startnonce + n + 2));
            nonce4 = bswap_64((uint64_t)(local_startnonce + n + 3));
            nonce5 = bswap_64((uint64_t)(local_startnonce + n + 4));
            nonce6 = bswap_64((uint64_t)(local_startnonce + n + 5));
            nonce7 = bswap_64((uint64_t)(local_startnonce + n + 6));
            nonce8 = bswap_64((uint64_t)(local_startnonce + n + 7));

            // store nonce numbers in relevant termination strings
            for (int j = 16; j < 64 * MSHABAL256_FACTOR / 4; j += 4 * MSHABAL256_FACTOR) {
                size_t o = j / MSHABAL256_FACTOR - 8;
                // t1
                t1.words[j + 0] = *(mshabal_u32 *)((char *)&nonce1 + o);
                t1.words[j + 1] = *(mshabal_u32 *)((char *)&nonce2 + o);
                t1.words[j + 2] = *(mshabal_u32 *)((char *)&nonce3 + o);
                t1.words[j + 3] = *(mshabal_u32 *)((char *)&nonce4 + o);
                t1.words[j + 4] = *(mshabal_u32 *)((char *)&nonce5 + o);
                t1.words[j + 5] = *(mshabal_u32 *)((char *)&nonce6 + o);
                t1.words[j + 6] = *(mshabal_u32 *)((char *)&nonce7 + o);
                t1.words[j + 7] = *(mshabal_u32 *)((char *)&nonce8 + o);
                t2.words[j + 0 + 64] = *(mshabal_u32 *)((char *)&nonce1 + o);
                t2.words[j + 1 + 64] = *(mshabal_u32 *)((char *)&nonce2 + o);
                t2.words[j + 2 + 64] = *(mshabal_u32 *)((char *)&nonce3 + o);
                t2.words[j + 3 + 64] = *(mshabal_u32 *)((char *)&nonce4 + o);
                t2.words[j + 4 + 64] = *(mshabal_u32 *)((char *)&nonce5 + o);
                t2.words[j + 5 + 64] = *(mshabal_u32 *)((char *)&nonce6 + o);
                t2.words[j + 6 + 64] = *(mshabal_u32 *)((char *)&nonce7 + o);
                t2.words[j + 7 + 64] = *(mshabal_u32 *)((char *)&nonce8 + o);
            }
    
            // start shabal round    

            // 3 cases: first 128 rounds uses case 1 or 2, after that case 3
            // case 1: first 128 rounds, hashes are even: use termination string 1
            // case 2: first 128 rounds, hashes are odd: use termination string 2
            // case 3: round > 128: use termination string 3
            
            // round 1
            memcpy(&local_256_fast, &global_256_fast,
                   sizeof(global_256_fast));  // fast initialize shabal                 
            
            mshabal256_openclose_fast(
                &local_256_fast, NULL, &t1,
                &buffer[MSHABAL256_VECTOR_SIZE * (NONCE_SIZE - HASH_SIZE)], 16 >> 6);

            // store first hash into smart termination string 2 (data is vectored and SIMD aligned)
            memcpy(&t2, &buffer[MSHABAL256_VECTOR_SIZE * (NONCE_SIZE - HASH_SIZE)],
                   MSHABAL256_VECTOR_SIZE * (HASH_SIZE));

            // round 2 - 128
            for (size_t i = NONCE_SIZE - HASH_SIZE; i > (NONCE_SIZE - HASH_CAP); i -= HASH_SIZE) {
                // check if msg can be divided into 512bit packages without a
                // remainder
                if (i % 64 == 0) {
                    // last msg = seed + termination
                    mshabal256_openclose_fast(&local_256_fast, &buffer[i * MSHABAL256_VECTOR_SIZE],
                                              &t1,
                                              &buffer[(i - HASH_SIZE) * MSHABAL256_VECTOR_SIZE],
                                              (NONCE_SIZE + 16 - i) >> 6);
                } else {
                    // last msg = 256 bit data + seed + termination
                    mshabal256_openclose_fast(&local_256_fast, &buffer[i * MSHABAL256_VECTOR_SIZE],
                                              &t2,
                                              &buffer[(i - HASH_SIZE) * MSHABAL256_VECTOR_SIZE],
                                              (NONCE_SIZE + 16 - i) >> 6);
                }
            }

            // round 128-8192
            for (size_t i = NONCE_SIZE - HASH_CAP; i > 0; i -= HASH_SIZE) {
                mshabal256_openclose_fast(&local_256_fast, &buffer[i * MSHABAL256_VECTOR_SIZE], &t3,
                                          &buffer[(i - HASH_SIZE) * MSHABAL256_VECTOR_SIZE],
                                          (HASH_CAP) >> 6);
            }
           
            // generate final hash
            mshabal256_openclose_fast(&local_256_fast, &buffer[0], &t1, &final[0],
                                      (NONCE_SIZE + 16) >> 6);
            
            // XOR using SIMD
            // load final hash
            __m256i F[8];
            for (int j = 0; j < 8; j++) F[j] = _mm256_loadu_si256((__m256i *)final + j);
            // xor all hashes with final hash
            for (int j = 0; j < 8 * 2 * HASH_CAP; j++)
                _mm256_storeu_si256(
                    (__m256i *)buffer + j,
                    _mm256_xor_si256(_mm256_loadu_si256((__m256i *)buffer + j), F[j % 8]));
             
            // todo: fork SIMD aligned plot file here
            
            // simd shabal words unpack + POC Shuffle + scatter nonces into optimised cache
            for (int i = 0; i < NUM_SCOOPS * 2; i++) {
                for (int j = 0; j < 32; j += 4) {
                    for (int k = 0; k < MSHABAL256_VECTOR_SIZE; k += 1) {
                    memcpy(&cache[((i & 1) * (4095 - (i >> 1)) + ((i + 1) & 1) * (i >> 1)) *
                                      SCOOP_SIZE * cache_size +
                                  (n + k + chunk_offset) * SCOOP_SIZE + (i & 1) * 32 + j],
                           &buffer[(i * 32 + j) * MSHABAL256_VECTOR_SIZE + k * 4], 4);
                    }
                }
            }

            n += 8;
        } else {
            // if less than 8 nonces left, use 1d-shabal
            int8_t *xv = (int8_t *)&numeric_id;
 
            for (size_t i = 0; i < 8; i++) buffer[NONCE_SIZE + i] = xv[7 - i];

            nonce = local_startnonce + n;
            xv = (int8_t *)&nonce;

            for (size_t i = 8; i < 16; i++) buffer[NONCE_SIZE + i] = xv[15 - i];

            for (size_t i = NONCE_SIZE; i > 0; i -= HASH_SIZE) {
                memcpy(&local_32, &global_32, sizeof(global_32));
                ;
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
    }
    free(buffer);
    free(final);
}
