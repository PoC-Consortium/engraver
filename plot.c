/* For Emacs: -*- mode:c; eval: (folding-mode 1) -*-
   Usage: ./plot64 -k <public key> -x <core> -s <start nonce> -n <nonces> -m <stagger size> -t <threads>
*/

/* {{{ include, define, vars */

#define USE_MULTI_SHABAL
#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <byteswap.h>

#include "apex_memmove.h"
#include "shabal.h"
#include "mshabal256.h"
#include "mshabal.h"
#include "helper.h"

// Leave 5GB free space
#define FREE_SPACE      (uint64_t)5 * 1000 * 1000 * 1000
#define DEFAULTDIR      "plots/"

// Not to be changed below this
#define SCOOP_SIZE      64
#define NUM_SCOOPS      4096
#define NONCE_SIZE      (NUM_SCOOPS * SCOOP_SIZE)

#define HASH_SIZE       32
#define HASH_CAP        4096

uint64_t addr        = 0;
uint64_t startnonce  = 0;
uint32_t nonces      = 0;
uint32_t staggersize = 0;
uint32_t threads     = 0;
uint32_t noncesperthread;
uint32_t selecttype  = 0;
uint32_t asyncmode   = 0;
double createtime    = 0.0;
uint64_t starttime;
uint64_t run, lastrun, thisrun;
int ofd;

char *cache, *wcache, *acache[2];
char *outputdir = DEFAULTDIR;

#define SET_NONCE(gendata, nonce, offset)      \
    *(uint64_t *)(gendata + NONCE_SIZE + offset) = bswap_64(nonce)

/* }}} */

/* {{{ nonce             original algorithm */

void nonce(uint64_t addr, uint64_t nonce, uint64_t cachepos) {
    char final[32];
    char gendata[16 + NONCE_SIZE];

    SET_NONCE(gendata, addr,  0);
    SET_NONCE(gendata, nonce, 8);

    shabal_context init_x, x;
    uint32_t len = NONCE_SIZE + 16;

    shabal_init(&init_x, 256);
    for (uint32_t i = NONCE_SIZE; i > 0; i -= HASH_SIZE) {
        apex_memmove(&x, &init_x, sizeof(init_x));
        len -= i;
        if (len > HASH_CAP)
            len = HASH_CAP;
        shabal(&x, &gendata[i], len);
        shabal_close(&x, 0, 0, &gendata[i - HASH_SIZE]);
    }
        
    shabal_init(&x, 256);
    shabal(&x, gendata, 16 + NONCE_SIZE);
    shabal_close(&x, 0, 0, final);

    // XOR with final
    uint64_t *start = (uint64_t*)gendata;
    uint64_t *fint  = (uint64_t*)&final;

    for (uint32_t i = 0; i < NONCE_SIZE; i += 32) {
        *start ^= fint[0]; start++;
        *start ^= fint[1]; start++;
        *start ^= fint[2]; start++;
        *start ^= fint[3]; start++;
    }

    // Sort them:
    for (uint32_t i = 0; i < NONCE_SIZE; i += SCOOP_SIZE)
        apex_memmove(&cache[cachepos * SCOOP_SIZE + (uint64_t)i * staggersize], &gendata[i], SCOOP_SIZE);
}

/* }}} */
/* {{{ mnonce            SSE4 version       */

int
mnonce(uint64_t addr,
       uint64_t nonce1, uint64_t nonce2, uint64_t nonce3, uint64_t nonce4,
       uint64_t cachepos1, uint64_t cachepos2, uint64_t cachepos3, uint64_t cachepos4) {
    char final1[32], final2[32], final3[32], final4[32];
    char gendata1[16 + NONCE_SIZE], gendata2[16 + NONCE_SIZE], gendata3[16 + NONCE_SIZE], gendata4[16 + NONCE_SIZE];

    SET_NONCE(gendata1, addr,  0);

    for (int i = NONCE_SIZE; i <= NONCE_SIZE + 7; ++i) {
        gendata2[i] = gendata1[i];
        gendata3[i] = gendata1[i];
        gendata4[i] = gendata1[i];
    }

    SET_NONCE(gendata1, nonce1, 8);
    SET_NONCE(gendata2, nonce2, 8);
    SET_NONCE(gendata3, nonce3, 8);
    SET_NONCE(gendata4, nonce4, 8);

    mshabal_context x;
    int len;

    for (int i = NONCE_SIZE; i > 0; i -= HASH_SIZE) {
      sse4_mshabal_init(&x, 256);

      len = NONCE_SIZE + 16 - i;
      if (len > HASH_CAP)
          len = HASH_CAP;

      sse4_mshabal(&x, &gendata1[i], &gendata2[i], &gendata3[i], &gendata4[i], len);
      sse4_mshabal_close(&x, 0, 0, 0, 0, 0, &gendata1[i - HASH_SIZE], &gendata2[i - HASH_SIZE], &gendata3[i - HASH_SIZE], &gendata4[i - HASH_SIZE]);
    }

    sse4_mshabal_init(&x, 256);
    sse4_mshabal(&x, gendata1, gendata2, gendata3, gendata4, 16 + NONCE_SIZE);
    sse4_mshabal_close(&x, 0, 0, 0, 0, 0, final1, final2, final3, final4);

    // XOR with final
    for (int i = 0; i < NONCE_SIZE; i++) {
        gendata1[i] ^= (final1[i % 32]);
        gendata2[i] ^= (final2[i % 32]);
        gendata3[i] ^= (final3[i % 32]);
        gendata4[i] ^= (final4[i % 32]);
    }

    // Sort them:
    for (int i = 0; i < NONCE_SIZE; i += 64) {
        apex_memmove(&cache[cachepos1 * 64 + (uint64_t)i * staggersize], &gendata1[i], 64);
        apex_memmove(&cache[cachepos2 * 64 + (uint64_t)i * staggersize], &gendata2[i], 64);
        apex_memmove(&cache[cachepos3 * 64 + (uint64_t)i * staggersize], &gendata3[i], 64);
        apex_memmove(&cache[cachepos4 * 64 + (uint64_t)i * staggersize], &gendata4[i], 64);
    }

    return 0;
}

// }}}
// {{{ m256nonce         AVX2 version

int
m256nonce(uint64_t addr,
          uint64_t nonce1, uint64_t nonce2, uint64_t nonce3, uint64_t nonce4,
          uint64_t nonce5, uint64_t nonce6, uint64_t nonce7, uint64_t nonce8,
          uint64_t cachepos) {
    char final1[32], final2[32], final3[32], final4[32];
    char final5[32], final6[32], final7[32], final8[32];
    char gendata1[16 + NONCE_SIZE], gendata2[16 + NONCE_SIZE], gendata3[16 + NONCE_SIZE], gendata4[16 + NONCE_SIZE];
    char gendata5[16 + NONCE_SIZE], gendata6[16 + NONCE_SIZE], gendata7[16 + NONCE_SIZE], gendata8[16 + NONCE_SIZE];

    SET_NONCE(gendata1, addr,  0);

    for (int i = NONCE_SIZE; i <= NONCE_SIZE + 7; ++i) {
      gendata2[i] = gendata1[i];
      gendata3[i] = gendata1[i];
      gendata4[i] = gendata1[i];
      gendata5[i] = gendata1[i];
      gendata6[i] = gendata1[i];
      gendata7[i] = gendata1[i];
      gendata8[i] = gendata1[i];
    }

    SET_NONCE(gendata1, nonce1, 8);
    SET_NONCE(gendata2, nonce2, 8);
    SET_NONCE(gendata3, nonce3, 8);
    SET_NONCE(gendata4, nonce4, 8);
    SET_NONCE(gendata5, nonce5, 8);
    SET_NONCE(gendata6, nonce6, 8);
    SET_NONCE(gendata7, nonce7, 8);
    SET_NONCE(gendata8, nonce8, 8);

    mshabal256_context x;
    int len;

    for (int i = NONCE_SIZE; i;) {
      mshabal256_init(&x);

      len = NONCE_SIZE + 16 - i;
      if (len > HASH_CAP)
        len = HASH_CAP;

      mshabal256(&x, &gendata1[i], &gendata2[i], &gendata3[i], &gendata4[i], &gendata5[i], &gendata6[i], &gendata7[i], &gendata8[i], len);

      i -= HASH_SIZE;
      mshabal256_close(&x,
                       (uint32_t *)&gendata1[i], (uint32_t *)&gendata2[i], (uint32_t *)&gendata3[i], (uint32_t *)&gendata4[i],
                       (uint32_t *)&gendata5[i], (uint32_t *)&gendata6[i], (uint32_t *)&gendata7[i], (uint32_t *)&gendata8[i]);

    }

    mshabal256_init(&x);
    mshabal256(&x, gendata1, gendata2, gendata3, gendata4, gendata5, gendata6, gendata7, gendata8, 16 + NONCE_SIZE);
    mshabal256_close(&x,
                     (uint32_t *)final1, (uint32_t *)final2, (uint32_t *)final3, (uint32_t *)final4,
                     (uint32_t *)final5, (uint32_t *)final6, (uint32_t *)final7, (uint32_t *)final8);

    // XOR with final
    for (int i = 0; i < NONCE_SIZE; i++) {
      gendata1[i] ^= final1[i % 32];
      gendata2[i] ^= final2[i % 32];
      gendata3[i] ^= final3[i % 32];
      gendata4[i] ^= final4[i % 32];
      gendata5[i] ^= final5[i % 32];
      gendata6[i] ^= final6[i % 32];
      gendata7[i] ^= final7[i % 32];
      gendata8[i] ^= final8[i % 32];
    }

    // Sort them:
    for (int i = 0; i < NONCE_SIZE; i += 64) {
      apex_memmove(&cache[cachepos * 64 +       (uint64_t)i * staggersize], &gendata1[i], 64);
      apex_memmove(&cache[cachepos * 64 +  64 + (uint64_t)i * staggersize], &gendata2[i], 64);
      apex_memmove(&cache[cachepos * 64 + 128 + (uint64_t)i * staggersize], &gendata3[i], 64);
      apex_memmove(&cache[cachepos * 64 + 192 + (uint64_t)i * staggersize], &gendata4[i], 64);
      apex_memmove(&cache[cachepos * 64 + 256 + (uint64_t)i * staggersize], &gendata5[i], 64);
      apex_memmove(&cache[cachepos * 64 + 320 + (uint64_t)i * staggersize], &gendata6[i], 64);
      apex_memmove(&cache[cachepos * 64 + 384 + (uint64_t)i * staggersize], &gendata7[i], 64);
      apex_memmove(&cache[cachepos * 64 + 448 + (uint64_t)i * staggersize], &gendata8[i], 64);
    }

    return 0;
}
// }}}
// {{{ work_i

void *
work_i(void *x_void_ptr) {
    uint64_t i = *(uint64_t *)x_void_ptr;

    uint32_t n, o;

    if (selecttype == 2) { // AVX2
        for (n = 0; n < noncesperthread; n += 8) {
            o = i + n;
            m256nonce(addr,
                      (o + 0), (o + 1), (o + 2), (o + 3),
                      (o + 4), (o + 5), (o + 6), (o + 7),
                      (o - startnonce));
        }
    }
    else {
        for (n = 0; n < noncesperthread; n++) {
            if (selecttype == 1) { // SSE4
                if (n + 4 <= noncesperthread) {
                    mnonce(addr,
                           (i + n), (i + n + 1), (i + n + 2), (i + n + 3),
                           (uint64_t)(i - startnonce + n),
                           (uint64_t)(i - startnonce + n + 1),
                           (uint64_t)(i - startnonce + n + 2),
                           (uint64_t)(i - startnonce + n + 3));
                    n += 3;
                }
                else {
                    printf("SSE4 inefficiency\n");
                    nonce(addr,(i + n), (uint64_t)(i - startnonce + n));
                }
            }
            else { // STANDARD
                nonce(addr, (i + n), (uint64_t)(i - startnonce + n));
            }
        }
    }

    return NULL;
}

/* }}} */
/* {{{ getMS */

uint64_t
getMS() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((uint64_t)time.tv_sec * 1000000) + time.tv_usec;
}

/* }}} */
/* {{{ usage */

void usage(char **argv) {
    printf("Usage: %s -k KEY [ -x CORE ] [-d DIRECTORY] [-s STARTNONCE] [-n NONCES] [-m STAGGERSIZE] [-t THREADS] -a\n\n", argv[0]);
    printf("   see README.md\n");
    exit(-1);
}

/* }}} */
/* {{{ writecache */

void *
writecache(void *arguments) {
    uint64_t cacheblocksize = staggersize * SCOOP_SIZE;
    uint64_t thisnonce;
    double percent;

    percent = 100.0 * (double)lastrun / (double)nonces;

    if (asyncmode == 1) {
        printf("\33[2K\r%.1f Percent done. %d nonces created in %.1f seconds. (ASYNC write)", percent, (int)staggersize, createtime);
        fflush(stdout);
    }
    else {
        printf("\33[2K\r%.1f Percent done. %d nonces created in %.1f seconds. (write)", percent, (int)staggersize, createtime);
        fflush(stdout);
    }

    for (thisnonce = 0; thisnonce < NUM_SCOOPS; thisnonce++ ) {
        uint64_t cacheposition = thisnonce * cacheblocksize;
        uint64_t fileposition  = (uint64_t)(thisnonce * (uint64_t)nonces * (uint64_t)SCOOP_SIZE + thisrun * (uint64_t)SCOOP_SIZE);
        if ( lseek64(ofd, fileposition, SEEK_SET) < 0 ) {
            printf("\n\nError while lseek()ing in file: %d\n\n", errno);
            exit(1);
        }
        if ( write(ofd, &wcache[cacheposition], cacheblocksize) < 0 ) {
            printf("\n\nError while writing to file: %d\n\n", errno);
            exit(1);
        }
    }

    uint64_t ms = getMS() - starttime;

    percent = 100.0 * (double)lastrun / (double)nonces;
    double minutes = (double)ms / (1000000 * 60);
    int    speed   = (int)(staggersize / minutes);
    int    m       = (int)(nonces - run) / speed;
    int    h       = (int)(m / 60);
    m -= h * 60;

    printf("\33[2K\r%.1f Percent done. %i nonces/minute, %i:%02i left", percent, speed, h, m);
    fflush(stdout);

    return NULL;
}

/* }}} */

/* {{{ main */

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv);
    }
    
    int i;
    int startgiven = 0;
    for (uint8_t i = 1; i < argc; i++) {
        // Ignore unknown argument
        if(argv[i][0] != '-')
            continue;

        if (!strcmp(argv[i],"-a")) {
            asyncmode = 1;
            printf("Async mode set.\n");
            continue;
        }

        char *parse = NULL;
        uint64_t parsed;
        char param = argv[i][1];
        int modified, ds;

        if (argv[i][2] == 0) {
            if (i < argc - 1)
                parse = argv[++i];
        }
        else {
            parse = &(argv[i][2]);
        }
        if (parse != NULL) {
            modified = 0;
            parsed = strtoull(parse, 0, 10);
            switch(parse[strlen(parse) - 1]) {
            case 't':
            case 'T':
                parsed *= 1024;
            case 'g':
            case 'G':
                parsed *= 1024;
            case 'm':
            case 'M':
                parsed *= 1024;
            case 'k':
            case 'K':
                parsed *= 1024;
                modified = 1;
            }
            switch(param) {
            case 'k':
                addr = parsed;
                break;
            case 's':
                startnonce = parsed;
                startgiven = 1;
                break;
            case 'n':
                if (modified == 1) {
                    nonces = (uint64_t)(parsed / NONCE_SIZE);
                }
                else {
                    nonces = parsed;
                }       
                break;
            case 'm':
                if (modified == 1) {
                    staggersize = (uint64_t)(parsed / NONCE_SIZE);
                }
                else {
                    staggersize = parsed;
                }       
                break;
            case 't':
                threads = parsed;
                break;
            case 'x':
                selecttype = parsed;
                break;
            case 'd':
                ds = strlen(parse);
                outputdir = (char*) malloc(ds + 2);
                apex_memmove(outputdir, parse, ds);
                // Add final slash?
                if (outputdir[ds - 1] != '/') {
                    outputdir[ds] = '/';
                    outputdir[ds + 1] = 0;
                }
                else {
                    outputdir[ds] = 0;
                }
                                        
            }                       
        }
    }

    if (selecttype == 1)      {
        printf("Using SSE4 core.\n");
    }
    else if (selecttype == 2) {
        printf("Using AVX2 core.\n");
        if (nonces % (threads * 8)) {
            printf("Number of nonces not divisible by threads * 8\n");
            exit(-1);
        }
    }
    else {                    printf("Using ORIG core.\n");
        selecttype = 0;
    }

    if (addr == 0)
        usage(argv);

    // Autodetect threads
    if (threads == 0)
        threads = getNumberOfCores();

    // No startnonce given: Just pick random one
    if (startgiven == 0) {
        // Just some randomness
        srand(time(NULL));
        startnonce = (uint64_t)rand() * (1 << 30) + rand();
    }

    // No nonces given: use whole disk
    if (nonces == 0) {
      uint64_t fs = freespace(outputdir);
        if (fs <= FREE_SPACE) {
            printf("Not enough free space on device\n");
            exit(-1);
        }
        fs -= FREE_SPACE;
                                
        nonces = (uint64_t)(fs / NONCE_SIZE);
    }

    // Autodetect stagger size
    if (staggersize == 0) {
      uint64_t memstag = (freemem() * 0.8) / NONCE_SIZE; // use 80% of memory


        if (nonces < memstag) {         // Small stack: all at once
            staggersize = nonces;
        }
        else {                          // Determine stagger that (almost) fits nonces
            
            for (i = memstag; i >= 1000; i--) {
                if ((nonces % i) < 1000) {
                    staggersize = i;
                    nonces-= (nonces % i);
                    i = 0;
                }
            }
        }
    }

    // Adjust according to stagger size
    if (nonces % staggersize != 0) {
        nonces -= nonces % staggersize;
        nonces += staggersize;
        printf("Adjusting total nonces to %u to match stagger size\n", nonces);
    }

    printf("Creating plots for nonces %" PRIu64 " to %" PRIu64 " (%u GB) using %u MB memory and %u threads\n",
           startnonce, (startnonce + nonces), (uint32_t)(nonces / 4 / 953), (uint32_t)(staggersize / 4 * (1 + asyncmode)), threads);

    // Comment this out/change it if you really want more than 128 Threads
    if (threads > 128) {
        printf("%u threads? Sure?\n", threads);
        exit(-1);
    }

    if (asyncmode == 1) {
        acache[0] = calloc( NONCE_SIZE, staggersize );
        acache[1] = calloc( NONCE_SIZE, staggersize );

        if (acache[0] == NULL || acache[1] == NULL) {
            printf("Error allocating memory. Try lower stagger size or removing ASYNC mode.\n");
            exit(-1);
        }
    }
    else {
        cache = calloc( NONCE_SIZE, staggersize );

        if (cache == NULL) {
            printf("Error allocating memory. Try lower stagger size.\n");
            exit(-1);
        }
    }

    mkdir(outputdir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);

    char name[100];
    char finalname[100];
    sprintf(name, "%s%"PRIu64"_%"PRIu64"_%u_%u.plotting", outputdir, addr, startnonce, nonces, nonces);
    sprintf(finalname, "%s%"PRIu64"_%"PRIu64"_%u_%u", outputdir, addr, startnonce, nonces, nonces);

    unlink(name); // no need to see if file exists: unlink can handle that

    ofd = open(name, O_CREAT | O_LARGEFILE | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (ofd < 0) {
        printf("Error opening file %s\n", name);
        exit(1);
    }

    // pre-allocate space to prevent fragmentation
    printf("Pre-allocating space for file (%ld bytes)...\n", (uint64_t)nonces * NONCE_SIZE);
    if ( posix_fallocate(ofd, 0, (uint64_t)nonces * NONCE_SIZE) != 0 ) {
        printf("File pre-allocation failed.\n");
        return 1;       
    }
    else {
        printf("Done pre-allocating space.\n");
    }

    // Threads:
    noncesperthread = (uint64_t)(staggersize / threads);

    if (noncesperthread == 0) {
        threads = staggersize;
        noncesperthread = 1;
    }

    pthread_t worker[threads], writeworker;
    uint64_t nonceoffset[threads];

    int asyncbuf = 0;
    double totalcreatetime = 0.0;
    uint64_t astarttime;
    if (asyncmode == 1) cache = acache[asyncbuf];
    else wcache = cache;

    mshabal256_init_buffers();

    for (run = 0; run < nonces; run += staggersize) {
        astarttime = getMS();

        for (i = 0; i < threads; i++) {
            nonceoffset[i] = startnonce + i * noncesperthread;

            if (pthread_create(&worker[i], NULL, work_i, &nonceoffset[i])) {
                printf("Error creating thread. Out of memory? Try lower stagger size / less threads\n");
                exit(-1);
            }
        }

        for (i = 0; i < threads; i++) {           // Wait for Threads to finish;
            pthread_join(worker[i], NULL);
        }

        for (i = threads * noncesperthread; i < staggersize; i++)     // Run leftover nonces
            nonce(addr, startnonce + i, (uint64_t)i);

        // Write plot to disk:
        createtime = ((double)getMS() - (double)astarttime) / 1000000.0;
        totalcreatetime += createtime;
        starttime = astarttime;
        if (asyncmode == 1) {
            if (run > 0) pthread_join(writeworker, NULL);
            thisrun = run;
            lastrun = run + staggersize;
            wcache = cache;
            if (pthread_create(&writeworker, NULL, writecache, (void *)NULL)) {
                printf("Error creating thread. Out of memory? Try lower stagger size / less threads / remove async mode\n");
                exit(-1);
            }       
            asyncbuf = 1 - asyncbuf;                    
            cache = acache[asyncbuf];
        }
        else {
            thisrun = run;
            lastrun = run + staggersize;
            if (pthread_create(&writeworker, NULL, writecache, (void *)NULL)) {
                printf("Error creating thread. Out of memory? Try lower stagger size / less threads\n");
                exit(-1);
            }
            pthread_join(writeworker, NULL);
        }

        startnonce += staggersize;
    }
        
    if (asyncmode == 1) pthread_join(writeworker, NULL);

    close(ofd);

    printf("\nFinished plotting. %d nonces created in %.1fs; renaming file...\n", nonces, totalcreatetime);

    unlink(finalname);

    if ( rename(name, finalname) < 0 ) {
        printf("Error while renaming file: %d\n", errno);
        return 1;
    }

    return 0;
}

/* }}} */
