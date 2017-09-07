/*
 * API for implementations of Shabal.
 *
 * The implementations all support the 16 defined Shabal variants,
 * corresponding to output sizes 32 to 512 bits (multiples of 32 only).
 * This includes the SHA-3 standard sizes 224, 256, 384 and 512.
 *
 * All implementations are reentrant and thread-safe: any two threads,
 * including signal handlers, may use these functions simultaneously,
 * as long as they use distinct context structures.
 *
 * -----------------------------------------------------------------------
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
 * -----------------------------------------------------------------------
 */

#ifndef shabal_h__
#define shabal_h__

#include <limits.h>

/*
 * We define the 'shabal_u32' type to be an unsigned integer type with
 * at least 32 bits; if possible, it will have exactly 32 bits.
 */
#if defined __STDC__ && __STDC_VERSION__ >= 199901L
#include <stdint.h>
#ifdef UINT32_MAX
typedef uint32_t shabal_u32;
#else
typedef uint_fast32_t shabal_u32;
#endif
#else
#if ((UINT_MAX >> 11) >> 11) >= 0x3FF
typedef unsigned int shabal_u32;
#else
typedef unsigned long shabal_u32;
#endif
#endif

/*
 * A 'shabal_context' instance represents the state for a Shabal
 * computation. Its contents are opaque (they are not meant to be
 * accessed directly, and their semantics are not part of the API). The
 * caller is responsible for allocating contexts, with proper alignment,
 * e.g. as local variables, global variables or on the heap.
 *
 * A context instance must be initialized before use, by calling the
 * shabal_init() function. Once initialized, the context can be used
 * to input data by chunk through zero, one or more calls to shabal().
 * The hash computation is finalized, and the hash value produced, by
 * calling shabal_close(). After that call, the context contents are
 * indeterminate; shabal_init() must be called again if the context is
 * to be reused for a new computation.
 *
 * Context instances are cloneable and moveable: the current state of
 * the hash computation can be saved by simply copying the context
 * contents into another structure instance (e.g. with memcpy()).
 * Context instances contain no pointer.
 */
typedef struct {
	unsigned char buf[64];
	size_t ptr;
	shabal_u32 state[12 + 16 + 16];
	shabal_u32 Wlow, Whigh;
	unsigned out_size;
} shabal_context;

/**
 * Initialize the provided context, for a computation with the specified
 * output size. The output size is given in bits; it MUST be one of the
 * supported output sizes (multiple of 32, between 32 and 512 inclusive).
 *
 * @param sc         pointer to the context to initialize
 * @param out_size   the intended output size (in bits)
 */
void shabal_init(shabal_context *sc, unsigned out_size);

/**
 * Process some additional bytes. If 'len' is zero, then this function
 * does nothing, and 'data' is ignored. Otherwise, 'len' consecutive
 * bytes are read, beginning with the byte pointed to by 'data', and
 * processed. There is no alignment condition on the input data.
 *
 * @param sc     pointer to the context structure
 * @param data   pointer to the input data
 * @param len    input data length (in bytes)
 */
void shabal(shabal_context *sc, const void *data, size_t len);

/**
 * Terminate the hash computation and write out the hash result in 'dst'.
 * 'dst' MUST point to a large enough buffer; the hash result length has
 * been set when the context was last initialized (with shabal_init()).
 * There is no alignment condition on the destination bufffer.
 *
 * This function allows the addition of 0 to 7 trailing bits to the hash
 * function input prior to finalization. The 'n' parameter contains the
 * number of extra bits (it MUST lie between 0 and 7 inclusive). The
 * 'ub' parameter contains the extra bits: the first extra bits is the
 * bit with numerical value 128 in 'ub', the second bit has value 64,
 * and so on. The other bits in 'ub' are ignored. In particular, if 'n'
 * is zero (which means that the hash input consists in an integral
 * number of bytes), then 'ub' is totally ignored.
 *
 * After the call to shabal_close(), the structure contents are
 * indeterminate and cannot be reused, except for a shabal_init() call
 * which begins a new hash computation.
 *
 * @param sc    pointer to the context structure
 * @param ub    the final extra input bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   pointer to the output buffer (receives the hash output)
 */
void shabal_close(shabal_context *sc, unsigned ub, unsigned n, void *dst);

#endif
