/*
 * Parallel implementation of Shabal, using the AVX2 unit. This code
 * compiles and runs on x86 architectures, in 32-bit or 64-bit mode,
 * which possess a AVX2-compatible SIMD unit.
 *
 *
 * (c) 2010 SAPHIR project. This software is provided 'as-is', without
 * any epxress or implied warranty. In no event will the authors be held
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to no restriction.
 *
 * Technical remarks and questions can be addressed to:
 * <thomas.pornin@cryptolog.com>
 */

#include <stddef.h>
#include <string.h>

#include <stdint.h>
#include <immintrin.h>
#include "mshabal256.h"

#ifdef  __cplusplus
    extern "C" {
#endif

#define C32(x)         ((uint32_t)x ## UL)
#define T32(x)         ((x) & C32(0xFFFFFFFF))
#define ROTL32(x, n)   T32(((x) << (n)) | ((x) >> (32 - (n))))

static void
mshabal256_compress(mshabal256_context *sc,
                    const uint8_t *buf0, const uint8_t *buf1,
                    const uint8_t *buf2, const uint8_t *buf3,
                    const uint8_t *buf4, const uint8_t *buf5,
                    const uint8_t *buf6, const uint8_t *buf7,
                    size_t num) {
    union {
        uint32_t words[128];
        __m256i data[16];
    } u;
    __m256i A[12], B[16], C[16];
    __m256i one = _mm256_set1_epi32(C32(0xFFFFFFFF));

    for (uint8_t j = 0; j < 12; j++) {
        A[j] = _mm256_loadu_si256((__m256i *)sc->state + j);
        B[j] = _mm256_loadu_si256((__m256i *)sc->state + j + 12);
        C[j] = _mm256_loadu_si256((__m256i *)sc->state + j + 28);
    }
    B[12] = _mm256_loadu_si256((__m256i *)sc->state + 24);
    B[13] = _mm256_loadu_si256((__m256i *)sc->state + 25);
    B[14] = _mm256_loadu_si256((__m256i *)sc->state + 26);
    B[15] = _mm256_loadu_si256((__m256i *)sc->state + 27);

    C[12] = _mm256_loadu_si256((__m256i *)sc->state + 40);
    C[13] = _mm256_loadu_si256((__m256i *)sc->state + 41);
    C[14] = _mm256_loadu_si256((__m256i *)sc->state + 42);
    C[15] = _mm256_loadu_si256((__m256i *)sc->state + 43);

#define M(i)   _mm256_load_si256(u.data + (i))

    while (num--) {
        uint8_t o = 0;
        for (uint8_t j = 0; j < 128; j += 8, o += 4) {
            u.words[j + 0] = *(uint32_t *)(buf0 + o);
            u.words[j + 1] = *(uint32_t *)(buf1 + o);
            u.words[j + 2] = *(uint32_t *)(buf2 + o);
            u.words[j + 3] = *(uint32_t *)(buf3 + o);
            u.words[j + 4] = *(uint32_t *)(buf4 + o);
            u.words[j + 5] = *(uint32_t *)(buf5 + o);
            u.words[j + 6] = *(uint32_t *)(buf6 + o);
            u.words[j + 7] = *(uint32_t *)(buf7 + o);
        }

        for (uint8_t j = 0; j < 16; j++)
            B[j] = _mm256_add_epi32(B[j], M(j));

        A[0] = _mm256_xor_si256(A[0], _mm256_set1_epi32(sc->Wlow));
        A[1] = _mm256_xor_si256(A[1], _mm256_set1_epi32(sc->Whigh));

        for (uint8_t j = 0; j < 16; j++)
            B[j] = _mm256_or_si256(_mm256_slli_epi32(B[j], 17),
                                   _mm256_srli_epi32(B[j], 15));

#define PP(xa0, xa1, xb0, xb1, xb2, xb3, xc, xm)   do {                 \
            __m256i tt;                                                 \
            tt = _mm256_or_si256(_mm256_slli_epi32(xa1, 15),            \
                                 _mm256_srli_epi32(xa1, 17));           \
            tt = _mm256_add_epi32(_mm256_slli_epi32(tt, 2), tt);        \
            tt = _mm256_xor_si256(_mm256_xor_si256(xa0, tt), xc);       \
            tt = _mm256_add_epi32(_mm256_slli_epi32(tt, 1), tt);        \
            tt = _mm256_xor_si256(_mm256_xor_si256(tt, xb1),            \
                                  _mm256_xor_si256(_mm256_andnot_si256(xb3, xb2), xm)); \
            xa0 = tt;                                                   \
            tt = xb0;                                                   \
            tt = _mm256_or_si256(_mm256_slli_epi32(tt, 1),              \
                                 _mm256_srli_epi32(tt, 31));            \
            xb0 = _mm256_xor_si256(tt, _mm256_xor_si256(xa0, one));     \
        } while (0)

        PP(A[0x0], A[0xB], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x1], A[0x0], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0x2], A[0x1], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0x3], A[0x2], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x4], A[0x3], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x5], A[0x4], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0x6], A[0x5], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0x7], A[0x6], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x8], A[0x7], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x9], A[0x8], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0xA], A[0x9], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0xB], A[0xA], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x0], A[0xB], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x1], A[0x0], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0x2], A[0x1], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0x3], A[0x2], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        PP(A[0x4], A[0x3], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x5], A[0x4], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0x6], A[0x5], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0x7], A[0x6], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x8], A[0x7], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x9], A[0x8], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0xA], A[0x9], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0xB], A[0xA], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x0], A[0xB], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x1], A[0x0], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0x2], A[0x1], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0x3], A[0x2], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x4], A[0x3], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x5], A[0x4], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0x6], A[0x5], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0x7], A[0x6], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        PP(A[0x8], A[0x7], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x9], A[0x8], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0xA], A[0x9], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0xB], A[0xA], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x0], A[0xB], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x1], A[0x0], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0x2], A[0x1], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0x3], A[0x2], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x4], A[0x3], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x5], A[0x4], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0x6], A[0x5], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0x7], A[0x6], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x8], A[0x7], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x9], A[0x8], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0xA], A[0x9], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0xB], A[0xA], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        A[0xB] = _mm256_add_epi32(A[0xB], C[0x6]);
        A[0xA] = _mm256_add_epi32(A[0xA], C[0x5]);
        A[0x9] = _mm256_add_epi32(A[0x9], C[0x4]);
        A[0x8] = _mm256_add_epi32(A[0x8], C[0x3]);
        A[0x7] = _mm256_add_epi32(A[0x7], C[0x2]);
        A[0x6] = _mm256_add_epi32(A[0x6], C[0x1]);
        A[0x5] = _mm256_add_epi32(A[0x5], C[0x0]);
        A[0x4] = _mm256_add_epi32(A[0x4], C[0xF]);
        A[0x3] = _mm256_add_epi32(A[0x3], C[0xE]);
        A[0x2] = _mm256_add_epi32(A[0x2], C[0xD]);
        A[0x1] = _mm256_add_epi32(A[0x1], C[0xC]);
        A[0x0] = _mm256_add_epi32(A[0x0], C[0xB]);
        A[0xB] = _mm256_add_epi32(A[0xB], C[0xA]);
        A[0xA] = _mm256_add_epi32(A[0xA], C[0x9]);
        A[0x9] = _mm256_add_epi32(A[0x9], C[0x8]);
        A[0x8] = _mm256_add_epi32(A[0x8], C[0x7]);
        A[0x7] = _mm256_add_epi32(A[0x7], C[0x6]);
        A[0x6] = _mm256_add_epi32(A[0x6], C[0x5]);
        A[0x5] = _mm256_add_epi32(A[0x5], C[0x4]);
        A[0x4] = _mm256_add_epi32(A[0x4], C[0x3]);
        A[0x3] = _mm256_add_epi32(A[0x3], C[0x2]);
        A[0x2] = _mm256_add_epi32(A[0x2], C[0x1]);
        A[0x1] = _mm256_add_epi32(A[0x1], C[0x0]);
        A[0x0] = _mm256_add_epi32(A[0x0], C[0xF]);
        A[0xB] = _mm256_add_epi32(A[0xB], C[0xE]);
        A[0xA] = _mm256_add_epi32(A[0xA], C[0xD]);
        A[0x9] = _mm256_add_epi32(A[0x9], C[0xC]);
        A[0x8] = _mm256_add_epi32(A[0x8], C[0xB]);
        A[0x7] = _mm256_add_epi32(A[0x7], C[0xA]);
        A[0x6] = _mm256_add_epi32(A[0x6], C[0x9]);
        A[0x5] = _mm256_add_epi32(A[0x5], C[0x8]);
        A[0x4] = _mm256_add_epi32(A[0x4], C[0x7]);
        A[0x3] = _mm256_add_epi32(A[0x3], C[0x6]);
        A[0x2] = _mm256_add_epi32(A[0x2], C[0x5]);
        A[0x1] = _mm256_add_epi32(A[0x1], C[0x4]);
        A[0x0] = _mm256_add_epi32(A[0x0], C[0x3]);

#define SWAP_AND_SUB(xb, xc, xm)   do {         \
            __m256i tmp;                        \
            tmp = xb;                           \
            xb = _mm256_sub_epi32(xc, xm);      \
            xc = tmp;                           \
        } while (0)

        SWAP_AND_SUB(B[0x0], C[0x0], M(0x0));
        SWAP_AND_SUB(B[0x1], C[0x1], M(0x1));
        SWAP_AND_SUB(B[0x2], C[0x2], M(0x2));
        SWAP_AND_SUB(B[0x3], C[0x3], M(0x3));
        SWAP_AND_SUB(B[0x4], C[0x4], M(0x4));
        SWAP_AND_SUB(B[0x5], C[0x5], M(0x5));
        SWAP_AND_SUB(B[0x6], C[0x6], M(0x6));
        SWAP_AND_SUB(B[0x7], C[0x7], M(0x7));
        SWAP_AND_SUB(B[0x8], C[0x8], M(0x8));
        SWAP_AND_SUB(B[0x9], C[0x9], M(0x9));
        SWAP_AND_SUB(B[0xA], C[0xA], M(0xA));
        SWAP_AND_SUB(B[0xB], C[0xB], M(0xB));
        SWAP_AND_SUB(B[0xC], C[0xC], M(0xC));
        SWAP_AND_SUB(B[0xD], C[0xD], M(0xD));
        SWAP_AND_SUB(B[0xE], C[0xE], M(0xE));
        SWAP_AND_SUB(B[0xF], C[0xF], M(0xF));
        
        buf0 += 64;
        buf1 += 64;
        buf2 += 64;
        buf3 += 64;
        buf4 += 64;
        buf5 += 64;
        buf6 += 64;
        buf7 += 64;
        
        if (++sc->Wlow == 0)
            sc->Whigh++;

    }

    for (uint8_t j = 0; j < 12; j++) {
        _mm256_storeu_si256((__m256i *)sc->state + j,      A[j]);
        _mm256_storeu_si256((__m256i *)sc->state + j + 12, B[j]);
        _mm256_storeu_si256((__m256i *)sc->state + j + 28, C[j]);
    }
    _mm256_storeu_si256((__m256i *)sc->state + 24, B[12]);
    _mm256_storeu_si256((__m256i *)sc->state + 25, B[13]);
    _mm256_storeu_si256((__m256i *)sc->state + 26, B[14]);
    _mm256_storeu_si256((__m256i *)sc->state + 27, B[15]);

    _mm256_storeu_si256((__m256i *)sc->state + 40, C[12]);
    _mm256_storeu_si256((__m256i *)sc->state + 41, C[13]);
    _mm256_storeu_si256((__m256i *)sc->state + 42, C[14]);
    _mm256_storeu_si256((__m256i *)sc->state + 43, C[15]);
    
#undef M
}

void
mshabal256_init(mshabal256_context *sc) {
    memset(sc->state, 0, 1408);
    
    memset(sc->buf0, 0, sizeof sc->buf0);
    memset(sc->buf1, 0, sizeof sc->buf1);
    memset(sc->buf2, 0, sizeof sc->buf2);
    memset(sc->buf3, 0, sizeof sc->buf3);
    memset(sc->buf4, 0, sizeof sc->buf4);
    memset(sc->buf5, 0, sizeof sc->buf5);
    memset(sc->buf6, 0, sizeof sc->buf6);
    memset(sc->buf7, 0, sizeof sc->buf7);
    for (uint8_t u = 0; u < 16; u++) {
        uint8_t idx = u * 4;
        sc->buf0[idx] = 256 + u;
        sc->buf1[idx] = 256 + u;
        sc->buf2[idx] = 256 + u;
        sc->buf3[idx] = 256 + u;
        sc->buf4[idx] = 256 + u;
        sc->buf5[idx] = 256 + u;
        sc->buf6[idx] = 256 + u;
        sc->buf7[idx] = 256 + u;
        idx++;
        sc->buf0[idx] = 1;
        sc->buf1[idx] = 1;
        sc->buf2[idx] = 1;
        sc->buf3[idx] = 1;
        sc->buf4[idx] = 1;
        sc->buf5[idx] = 1;
        sc->buf6[idx] = 1;
        sc->buf7[idx] = 1;
    }
    sc->Whigh = sc->Wlow = C32(0xFFFFFFFF);
    mshabal256_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, sc->buf4, sc->buf5, sc->buf6, sc->buf7, 1);
    for (uint8_t u = 0; u < 16; u++) {
        uint8_t idx = u * 4;
        sc->buf0[idx] = 272 + u;
        sc->buf1[idx] = 272 + u;
        sc->buf2[idx] = 272 + u;
        sc->buf3[idx] = 272 + u;
        sc->buf4[idx] = 272 + u;
        sc->buf5[idx] = 272 + u;
        sc->buf6[idx] = 272 + u;
        sc->buf7[idx] = 272 + u;
        idx++;
        sc->buf0[idx] = 1;
        sc->buf1[idx] = 1;
        sc->buf2[idx] = 1;
        sc->buf3[idx] = 1;
        sc->buf4[idx] = 1;
        sc->buf5[idx] = 1;
        sc->buf6[idx] = 1;
        sc->buf7[idx] = 1;
    }
    mshabal256_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, sc->buf4, sc->buf5, sc->buf6, sc->buf7, 1);
    sc->ptr = 0;
}

void
mshabal256(mshabal256_context *sc,
           const void *data0, const void *data1, const void *data2, const void *data3,
           const void *data4, const void *data5, const void *data6, const void *data7,
           size_t len) {
    size_t num;
    size_t ptr = sc->ptr;

    if (ptr != 0) {
        size_t clen;

        clen = (sizeof sc->buf0 - ptr);
        if (clen > len) {
            memcpy(sc->buf0 + ptr, data0, len);
            memcpy(sc->buf1 + ptr, data1, len);
            memcpy(sc->buf2 + ptr, data2, len);
            memcpy(sc->buf3 + ptr, data3, len);
            memcpy(sc->buf4 + ptr, data4, len);
            memcpy(sc->buf5 + ptr, data5, len);
            memcpy(sc->buf6 + ptr, data6, len);
            memcpy(sc->buf7 + ptr, data7, len);
            sc->ptr = ptr + len;
            return;
        }
        else {
            memcpy(sc->buf0 + ptr, data0, clen);
            memcpy(sc->buf1 + ptr, data1, clen);
            memcpy(sc->buf2 + ptr, data2, clen);
            memcpy(sc->buf3 + ptr, data3, clen);
            memcpy(sc->buf4 + ptr, data4, clen);
            memcpy(sc->buf5 + ptr, data5, clen);
            memcpy(sc->buf6 + ptr, data6, clen);
            memcpy(sc->buf7 + ptr, data7, clen);
            mshabal256_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, sc->buf4, sc->buf5, sc->buf6, sc->buf7, 1);
            data0 = (const uint8_t *)data0 + clen;
            data1 = (const uint8_t *)data1 + clen;
            data2 = (const uint8_t *)data2 + clen;
            data3 = (const uint8_t *)data3 + clen;
            data4 = (const uint8_t *)data4 + clen;
            data5 = (const uint8_t *)data5 + clen;
            data6 = (const uint8_t *)data6 + clen;
            data7 = (const uint8_t *)data7 + clen;
            len -= clen;
        }
    }

    num = len >> 6;
    if (num) {
        mshabal256_compress(sc, data0, data1, data2, data3, data4, data5, data6, data7, num);
        num <<= 6;
        data0 = (const uint8_t *)data0 + num;
        data1 = (const uint8_t *)data1 + num;
        data2 = (const uint8_t *)data2 + num;
        data3 = (const uint8_t *)data3 + num;
        data4 = (const uint8_t *)data4 + num;
        data5 = (const uint8_t *)data5 + num;
        data6 = (const uint8_t *)data6 + num;
        data7 = (const uint8_t *)data7 + num;
    }
    len &= (size_t)63;
    memcpy(sc->buf0, data0, len);
    memcpy(sc->buf1, data1, len);
    memcpy(sc->buf2, data2, len);
    memcpy(sc->buf3, data3, len);
    memcpy(sc->buf4, data4, len);
    memcpy(sc->buf5, data5, len);
    memcpy(sc->buf6, data6, len);
    memcpy(sc->buf7, data7, len);
    sc->ptr = len;
}

void
mshabal256_close(mshabal256_context *sc,
                 uint32_t *dst0, uint32_t *dst1, uint32_t *dst2, uint32_t *dst3,
                 uint32_t *dst4, uint32_t *dst5, uint32_t *dst6, uint32_t *dst7) {
    size_t ptr = sc->ptr;

    sc->buf0[ptr] = 0x80;
    sc->buf1[ptr] = 0x80;
    sc->buf2[ptr] = 0x80;
    sc->buf3[ptr] = 0x80;
    sc->buf4[ptr] = 0x80;
    sc->buf5[ptr] = 0x80;
    sc->buf6[ptr] = 0x80;
    sc->buf7[ptr] = 0x80;
    
    ptr++;
    memset(sc->buf0 + ptr, 0, (sizeof sc->buf0) - ptr);
    memset(sc->buf1 + ptr, 0, (sizeof sc->buf1) - ptr);
    memset(sc->buf2 + ptr, 0, (sizeof sc->buf2) - ptr);
    memset(sc->buf3 + ptr, 0, (sizeof sc->buf3) - ptr);
    memset(sc->buf4 + ptr, 0, (sizeof sc->buf4) - ptr);
    memset(sc->buf5 + ptr, 0, (sizeof sc->buf5) - ptr);
    memset(sc->buf6 + ptr, 0, (sizeof sc->buf6) - ptr);
    memset(sc->buf7 + ptr, 0, (sizeof sc->buf7) - ptr);

    mshabal256_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, sc->buf4, sc->buf5, sc->buf6, sc->buf7, 1);
    if (!sc->Wlow--) sc->Whigh--;
    mshabal256_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, sc->buf4, sc->buf5, sc->buf6, sc->buf7, 1);
    if (!sc->Wlow--) sc->Whigh--;
    mshabal256_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, sc->buf4, sc->buf5, sc->buf6, sc->buf7, 1);
    if (!sc->Wlow--) sc->Whigh--;
    mshabal256_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, sc->buf4, sc->buf5, sc->buf6, sc->buf7, 1);
    if (!sc->Wlow--) sc->Whigh--;

    uint8_t i = 0;
    for (uint16_t z = 288; z < 352;) {
        dst0[i] = sc->state[z++];
        dst1[i] = sc->state[z++];
        dst2[i] = sc->state[z++];
        dst3[i] = sc->state[z++];
        dst4[i] = sc->state[z++];
        dst5[i] = sc->state[z++];
        dst6[i] = sc->state[z++];
        dst7[i] = sc->state[z++];
        i++;
    }
}

#ifdef  __cplusplus
    extern "C" {
#endif
