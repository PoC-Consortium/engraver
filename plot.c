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

#include "shabal.h"
#include "mshabal256.h"
#include "mshabal.h"
#include "helper.h"

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
uint32_t noncesperthread, noncearguments;
uint32_t selecttype  = 0;
uint32_t asyncmode   = 0;
double createtime    = 0.0;
uint64_t maxmemory   = 0;
uint64_t leavespace  = 0;
uint64_t plotfilesize;
uint64_t starttime;
uint64_t run, lastrun, thisrun;
int userleavespace;
int lastspeed, lasthours, lastminutes, lastseconds;
int ofd;

char *cache, *wcache, *acache[2];
char *outputdir = DEFAULTDIR;

#define SET_NONCE(gendata, nonce, offset)      \
    xv = (char*)&nonce;                        \
    gendata[NONCE_SIZE + offset]     = xv[7];  \
    gendata[NONCE_SIZE + offset + 1] = xv[6];  \
    gendata[NONCE_SIZE + offset + 2] = xv[5];  \
    gendata[NONCE_SIZE + offset + 3] = xv[4];  \
    gendata[NONCE_SIZE + offset + 4] = xv[3];  \
    gendata[NONCE_SIZE + offset + 5] = xv[2];  \
    gendata[NONCE_SIZE + offset + 6] = xv[1];  \
    gendata[NONCE_SIZE + offset + 7] = xv[0]

/* }}} */

/* {{{ nonce             original algorithm */

void nonce(uint64_t addr, uint64_t nonce, uint64_t cachepos) {
    char final[32];
    char gendata[16 + NONCE_SIZE];
    char *xv;

    SET_NONCE(gendata, addr,  0);
    SET_NONCE(gendata, nonce, 8);

    shabal_context init_x, x;
    uint32_t len;

    shabal_init(&init_x, 256);
    for (uint32_t i = NONCE_SIZE; i > 0; i -= HASH_SIZE) {
        memcpy(&x, &init_x, sizeof(init_x));

        len = NONCE_SIZE + 16 - i;
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
        memmove(&cache[cachepos * SCOOP_SIZE + (uint64_t)i * staggersize], &gendata[i], SCOOP_SIZE);
}

/* }}} */
/* {{{ mnonce            SSE4 version       */

int
mnonce(uint64_t addr,
       uint64_t nonce1, uint64_t nonce2, uint64_t nonce3, uint64_t nonce4,
       uint64_t cachepos1, uint64_t cachepos2, uint64_t cachepos3, uint64_t cachepos4) {
    char final1[32], final2[32], final3[32], final4[32];
    char gendata1[16 + NONCE_SIZE], gendata2[16 + NONCE_SIZE], gendata3[16 + NONCE_SIZE], gendata4[16 + NONCE_SIZE];

    char *xv;

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
        memmove(&cache[cachepos1 * 64 + (uint64_t)i * staggersize], &gendata1[i], 64);
        memmove(&cache[cachepos2 * 64 + (uint64_t)i * staggersize], &gendata2[i], 64);
        memmove(&cache[cachepos3 * 64 + (uint64_t)i * staggersize], &gendata3[i], 64);
        memmove(&cache[cachepos4 * 64 + (uint64_t)i * staggersize], &gendata4[i], 64);
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

    char *xv;

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
      memmove(&cache[cachepos * 64 +       (uint64_t)i * staggersize], &gendata1[i], 64);
      memmove(&cache[cachepos * 64 +  64 + (uint64_t)i * staggersize], &gendata2[i], 64);
      memmove(&cache[cachepos * 64 + 128 + (uint64_t)i * staggersize], &gendata3[i], 64);
      memmove(&cache[cachepos * 64 + 192 + (uint64_t)i * staggersize], &gendata4[i], 64);
      memmove(&cache[cachepos * 64 + 256 + (uint64_t)i * staggersize], &gendata5[i], 64);
      memmove(&cache[cachepos * 64 + 320 + (uint64_t)i * staggersize], &gendata6[i], 64);
      memmove(&cache[cachepos * 64 + 384 + (uint64_t)i * staggersize], &gendata7[i], 64);
      memmove(&cache[cachepos * 64 + 448 + (uint64_t)i * staggersize], &gendata8[i], 64);
    }

    return 0;
}
// }}}
// {{{ work_i

void *
work_i(void *x_void_ptr) {
    uint64_t i = *(uint64_t *)x_void_ptr;

    uint32_t n;

    for (n = 0; n < noncesperthread; n += noncearguments) {
        if (selecttype == 1) { // SSE4
            mnonce(addr,
                    (i + n), (i + n + 1), (i + n + 2), (i + n + 3),
                    (uint64_t)(i - startnonce + n),
                    (uint64_t)(i - startnonce + n + 1),
                    (uint64_t)(i - startnonce + n + 2),
                    (uint64_t)(i - startnonce + n + 3));
        }
        else if (selecttype == 2) { // AVX2
            m256nonce(addr,
                    (i + n + 0), (i + n + 1), (i + n + 2), (i + n + 3),
                    (i + n + 4), (i + n + 5), (i + n + 6), (i + n + 7),
                    (i - startnonce + n));
        }
        else { // STANDARD
            nonce(addr, (i + n), (uint64_t)(i - startnonce + n));
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
    printf("Usage: %s -k KEY [ -x CORE ] [-d DIRECTORY] [-s STARTNONCE] [-n NONCES] [-m STAGGERSIZE] [-t THREADS] [-b MAXMEMORY] [-p PLOTFILESIZE] [-a] [-R]\n\n", argv[0]);
    printf("   see README.md\n");
    exit(-1);
}

/* }}} */
/* {{{ writecache */

void *
writecache(void *arguments) {
    uint64_t cacheblocksize = staggersize * SCOOP_SIZE;
    uint64_t thisnonce;
    float percent;

    percent = ((double)100 * lastrun / nonces);

    if (lastseconds) {
        printf("\33[2K\r%5.2f%% done. %i nonces/second, %02i:%02i:%02i left [writing%s]",
                percent, lastspeed, lasthours, lastminutes, lastseconds, (asyncmode) ? " asynchronously" : "");
    } else {
        printf("\33[2K\r%5.2f%% done. [writing%s]",
                percent, (asyncmode) ? " asynchronously" : "");
    }
    fflush(stdout);

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

    percent        = ((double)100 * lastrun / nonces);
    double runsecs = (double)ms / 1000000;
    lastspeed      = (int)(staggersize / runsecs);

    int seconds    = (int)(nonces - run) / lastspeed;
    int remainder  = seconds % 3600;
    lasthours      = (int)seconds / 3600;
    lastminutes    = remainder / 60;;
    lastseconds    = remainder % 60;

    printf("\33[2K\r%5.2f%% done. %i nonces/second, %02i:%02i:%02i left", percent, lastspeed, lasthours, lastminutes, lastseconds);
    fflush(stdout);

    return NULL;
}

/* }}} */

/* {{{ writestatus */

void
writestatus(void) {
    // Write current status to the end of the file
    if ( lseek64(ofd, -32, SEEK_END) < 0 ) {
        printf("\n\nError while lseek()ing in file: %d\n\n", errno);
        exit(1);
    }
    // Write (uint64_t)run, (uint64_t)startnonce, (uint32_t)staggersize
    if ( write(ofd, &run, sizeof run) < 0 ) {
        printf("\n\nError while writing to file: %d\n\n", errno);
        exit(1);
    }
    if ( write(ofd, &startnonce, sizeof startnonce) < 0 ) {
        printf("\n\nError while writing to file: %d\n\n", errno);
        exit(1);
    }
}

/* }}} */

/* {{{ main */

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv);
    }

    int i;
    int startgiven = 0;
    int resume = 0;
    for (uint8_t i = 1; i < argc; i++) {
        // Ignore unknown argument
        if(argv[i][0] != '-')
            continue;

        if (!strcmp(argv[i],"-a")) {
            asyncmode = 1;
            printf("Async mode set.\n");
            continue;
        }

        if (!strcmp(argv[i],"-R")) {
            resume = 1;
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
            case 'b':
                maxmemory = parsed;
                break;
            case 'f':
                userleavespace = 1;
                leavespace = parsed;
                break;
            case 'p':
                plotfilesize = parsed;
                break;
            case 'x':
                selecttype = parsed;
                break;
            case 'd':
                ds = strlen(parse);
                outputdir = (char*) malloc(ds + 2);
                memcpy(outputdir, parse, ds);
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

    // Autodetect threads
    if (threads == 0)
        threads = getNumberOfCores();

    if (selecttype == 1) {
        noncearguments = 4;
        printf("Using SSE4 core.\n");
    }
    else if (selecttype == 2) {
        noncearguments = 8;
        printf("Using AVX2 core.\n");
    }
    else {
        noncearguments = 1;
        printf("Using ORIG core.\n");
        selecttype = 0;
    }
    if (noncearguments > 1 && nonces % (threads * noncearguments)) {
        if (staggersize > 0) {
            printf("Stagger size is predefined, but number of nonces is not divisible by threads * %d. Unable to use selected hashing core.\n", noncearguments);
            exit(-1);
        } else {
            printf("Number of nonces is not divisible by threads * %d, and will be adjusted when calculating stagger size.\n", noncearguments);
        }
    }

    if (addr == 0) {
        usage(argv);
    }

    // No startnonce given: Just pick random one
    if (startgiven == 0) {
        // Just some randomness
        srand(time(NULL));
        startnonce = (uint64_t)rand() * (1 << 30) + rand();
    }
    if (nonces > 0 && plotfilesize > 0) {
        printf("Both number of nonces and size of plot file is specified. Choose one, and the other will be calculated automatically.\n");
        return(1);
    }

    // No nonces specified. Calculate nonces based on disk space
    if (nonces == 0) {
        uint64_t fs = freespace(outputdir);
        if (plotfilesize == 0 && (leavespace == 0 && userleavespace != 1)) {
            // Neither plot file size nor remaining disk space is specified.
            // Leave maximum 1GB if available, or 50% of the remaining diskspace otherwise.
            leavespace = (fs > 1024*1024*1024) ? 1024 * 1024 * 1024 : fs * 0.5;
        }
        uint64_t usespace = (plotfilesize > 0) ? plotfilesize : fs - leavespace;
        if (plotfilesize > 0  && leavespace > 0 && (plotfilesize + leavespace > fs)) {
            printf("Plot file size is set to %0.2f GB and we should leave %0.2f GB of free space, but the disk only has %0.2f GB available.\n",
                    (double)plotfilesize / 1024 / 1024 / 1024, (double)leavespace / 1024 / 1024 / 1024, (double)fs / 1024 / 1024 / 1024);
            exit(1);
        }
        if ((fs < usespace) || ((usespace / NONCE_SIZE) < 1)) {
            printf("Not enough free space on device. Disk has %0.2f GB available, and we're configured to use %0.2f GB, leaving %0.2f GB.\n",
                    (double)fs / 1024 / 1024 / 1024, (double)usespace / 1024 / 1024 / 1024, (double)leavespace / 1024 / 1024 / 1024);
            exit(-1);
        }
        nonces = (uint64_t)(usespace / NONCE_SIZE);
        if (noncearguments > 1 && nonces % (threads * noncearguments)) {
            nonces -= nonces % (threads * noncearguments);
        }
        printf("Number of nonces not specified. Attempting to create %d nonces (%0.2f GB), leaving %0.2f GB remaining free space.\n",
                nonces, ((double)nonces * NONCE_SIZE / 1024 / 1024 / 1024), ((double)(fs - usespace) / 1024 / 1024 / 1024));
    }

    // Autodetect stagger size
    if (staggersize == 0) {
        // Use max 80% (40% if async mode) of total available memory, unless the user has specified a limit
        uint64_t usememory = (maxmemory > 0) ? maxmemory : freemem() * 0.8;
        if (asyncmode) {
            usememory = (uint64_t)usememory / 2;
        }
        if (usememory < NONCE_SIZE) {
            printf("Unable to plot any nonces (%d bytes) with only %" PRIu64 " bytes of memory available.\n", NONCE_SIZE, usememory);
            exit(1);
        }

        uint64_t memstag = usememory / NONCE_SIZE;
        int staggerdiff = (memstag > 1000) ? 1000 : 1;
        if (nonces < memstag) {
            // Small stack: all at once
            if (noncearguments > 1 && nonces % (threads * noncearguments)) {
                printf("All nonces would fit in memory, but number of nonces is not divisible by threads * %d. Adjusting nonces from %d to %d.\n",
                        noncearguments, nonces, (nonces - (nonces % (threads * noncearguments))));
                nonces -= (nonces % (threads * noncearguments));
            } else {
                printf("All nonces will fit in memory. Setting stagger size to %d\n", nonces);
            }
            staggersize = nonces;
        }
        else {
            // Determine stagger that (almost) fits nonces
            for (i = memstag; i >= staggerdiff; i--) {
                if (i - (i % (threads * noncearguments)) <=  0) {
                    printf("Unable to find suitable stagger size for selected hashing core based on %d nonces and %d thread(s). Could indicate lack of memory (%0.2f GB).\n",
                            nonces, threads, (double)usememory / 1024 / 1024 / 1024);
                    return(1);
                }
                if (nonces % (i - (i % (threads * noncearguments))) <= staggerdiff) {
                    if (selecttype > 0) {
                        // Optimize stagger sizes for nonces processed per hashing core
                        i = i - (i % (threads * noncearguments));
                    }
                    staggersize = i;
                    printf("Stagger size was set to %u, based on available memory and selected hashing algorithm.\n", staggersize);
                    if ((nonces % staggersize) > 0) {
                        printf("Adjusting nonces from %u to %u to comply with stagger size.\n",
                                nonces, (nonces - (nonces % i)));
                        nonces -= (nonces % i);
                    }
                    i = 0;
                }
            }
        }
    }
    if (nonces == 0 || staggersize == 0) {
        printf("Ended up with %d nonces and a stagger size of %d. Unable to proceed.", nonces, staggersize);
        return(1);
    }

    // Adjust according to stagger size
    if (nonces % staggersize != 0) {
        nonces -= nonces % staggersize;
        nonces += staggersize;
        printf("Adjusting total nonces to %u to match stagger size\n", nonces);
    }

    printf("Creating plots for %u nonces (%" PRIu64 " to %" PRIu64 ", %0.2f GB) with stagger size %u, using %0.2f MB memory and %u threads\n",
           nonces, startnonce, (startnonce + nonces), ((double)nonces * NONCE_SIZE / 1024 / 1024 / 1024), staggersize, ((double)staggersize / 4 * (1 + asyncmode)), threads);

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

    int readconfig = 0;
    if ( !resume ) {
        unlink(name); // no need to see if file exists: unlink can handle that
    } else if( access( name, F_OK ) != -1 ) {
        readconfig = 1;
    }

    ofd = open(name, O_CREAT | O_LARGEFILE | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (ofd < 0) {
        printf("Error opening file %s\n", name);
        exit(1);
    }

    if ( readconfig ) {
        // Read config and initial status to the end of the file
        if ( lseek64(ofd, -32, SEEK_END) < 0 ) {
            printf("\n\nError while lseek()ing in file: %d\n\n", errno);
            exit(1);
        }
        // Read (uint64_t)run, (uint64_t)startnonce, (uint32_t)staggersize
        if ( read(ofd, &run, sizeof run) < sizeof run ) {
            printf("\n\nError while reading from file: %d\n\n", errno);
            exit(1);
        }
        if ( read(ofd, &startnonce, sizeof startnonce) < sizeof startnonce ) {
            printf("\n\nError while reading from file: %d\n\n", errno);
            exit(1);
        }
        if ( read(ofd, &staggersize, sizeof staggersize) < sizeof staggersize ) {
            printf("\n\nError while reading from file: %d\n\n", errno);
            exit(1);
        }
        printf("Resuming at nonce %ld with staggersize %d...\n", startnonce, staggersize);
    }
    else {
        // pre-allocate space to prevent fragmentation
        uint64_t filesize = (uint64_t)nonces * NONCE_SIZE;
        printf("Pre-allocating space for file (%ld bytes)...\n", filesize);
        if ( posix_fallocate(ofd, 0, filesize) != 0 ) {
            printf("File pre-allocation failed.\n");
            return 1;
        }
        else {
            printf("Done pre-allocating space.\n");
        }

        writestatus();
        if ( write(ofd, &staggersize, sizeof staggersize) < 0 ) {
            printf("\n\nError while writing to file: %d\n\n", errno);
            exit(1);
        }
    }

    // Threads:
    noncesperthread = (uint64_t)(staggersize / threads);

    if (noncesperthread == 0) {
        threads = staggersize;
        noncesperthread = 1;
    }

    pthread_t worker[threads], writeworker;
    int workerrunning = 0;
    uint64_t nonceoffset[threads];

    int asyncbuf = 0;
    double totalcreatetime = 0.0;
    uint64_t astarttime;
    if (asyncmode == 1) cache = acache[asyncbuf];
    else wcache = cache;

    for (; run < nonces; run += staggersize) {
        writestatus();
        astarttime = getMS();

        for (i = 0; i < threads; i++) {
            nonceoffset[i] = startnonce + i * noncesperthread;

            if (pthread_create(&worker[i], NULL, work_i, &nonceoffset[i])) {
                printf("Error creating thread. Out of memory? Try lower stagger size / fewer threads\n");
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
            if (workerrunning) pthread_join(writeworker, NULL);
            else workerrunning = 1;
            thisrun = run;
            lastrun = run + staggersize;
            wcache = cache;
            if (pthread_create(&writeworker, NULL, writecache, (void *)NULL)) {
                printf("Error creating thread. Out of memory? Try lower stagger size / fewer threads / remove async mode\n");
                exit(-1);
            }
            asyncbuf = 1 - asyncbuf;
            cache = acache[asyncbuf];
        }
        else {
            thisrun = run;
            lastrun = run + staggersize;
            if (pthread_create(&writeworker, NULL, writecache, (void *)NULL)) {
                printf("Error creating thread. Out of memory? Try lower stagger size / fewer threads\n");
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
