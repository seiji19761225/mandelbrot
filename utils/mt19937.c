/* 
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed) or
   init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.

     3. The names of its contributors may not be used to endorse or
     promote products derived from this software without specific
     prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICUAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
 */

#include <limits.h>
#include "mt19937_internal.h"

// Period parameters
#define N		624
#define M		397
#define MATRIX_A	0x9908b0dfU	// constant vector a
#define UPPER_MASK	0x80000000U	// most significant w-r bits
#define LOWER_MASK	0x7fffffffU	// least significant r bits
#define MASK		0xffffffffU	// for WORDSIZE > 32bit
#ifdef USE_COK_OPTIMIZATION
#define MIXBITS(u,v)	(((u)&UPPER_MASK)|((v)&LOWER_MASK))
#define TWIST(u,v)	((MIXBITS(u,v)>>1)^((v)&1U?MATRIX_A:0U))
#endif

static unsigned int mt[N];	// the array for the state vector
#ifdef USE_COK_OPTIMIZATION
static unsigned int *next;
static int left = 1, initf = 0;
#else
static int mti  = N + 1;	// mti==N+1 means mt[N] is not initialized
#endif

// prototype
#ifdef USE_COK_OPTIMIZATION
static void next_state(void);
#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void init_genrand(unsigned int s)
{				// initializes mt[N] with a seed
#ifdef USE_COK_OPTIMIZATION
    int mti;
#endif

    mt[0]  = s   ;
#if UINT_MAX > MASK
    mt[0] &= MASK;
#endif

    for (mti = 1; mti < N; mti++) {
	mt[mti] =
	    (1812433253U * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
	// See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
	// In the previous versions, MSBs of the seed affect
	// only MSBs of the array mt[].
	// 2002/01/09 modified by Makoto Matsumoto
#if UINT_MAX > MASK
	mt[mti] &= MASK;
#endif
    }

#ifdef USE_COK_OPTIMIZATION
    left = initf = 1;
#endif

    return;
}

//----------------------------------------------------------------------
void init_by_array(unsigned int init_key[], int key_length)
{				// initialize by an array with array-length
				// init_key is the array for initializing keys
				// key_length is its length
				// slight change for C++, 2004/2/26
    int i, j, k;

    init_genrand(19650218U);

    i = 1;
    j = 0;

    for (k = (N > key_length ? N : key_length); k > 0; k--) {
	// non linear
	mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525U))
	    + init_key[j] + j;
#if UINT_MAX > MASK
	mt[i] &= MASK;
#endif
	if (++i >= N) {
	    mt[0] = mt[N - 1];
	    i = 1;
	}
	if (++j >= key_length)
	    j = 0;
    }

    for (k = N - 1; k > 0; k--) {
	// non linear
	mt[i] =
	    (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941U)) - i;
#if UINT_MAX > MASK
	mt[i] &= MASK;
#endif
	if (++i >= N) {
	    mt[0] = mt[N - 1];
	    i = 1;
	}
    }

    mt[0] = 0x80000000U;	// MSB is 1; assuring non-zero initial array

#ifdef USE_COK_OPTIMIZATION
    left = initf = 1;
#endif

    return;
}

//----------------------------------------------------------------------
unsigned int genrand_int32(void)
#ifdef USE_COK_OPTIMIZATION
{				// generates a random number on [0,0xffffffff]-interval
    unsigned int y;

    if (--left == 0)
	next_state();

    y = *next++;

    // Tempering
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= (y >> 18);

    return y;
}
#else				//......................................
{				// generates a random number on [0,0xffffffff]-interval
    int kk;
    unsigned int y;
    static unsigned int mag01[2] = { 0x0U, MATRIX_A };

    // mag01[x] = x * MATRIX_A  for x=0,1

    if (mti >= N) {		// generate N words at one time
	if (mti == N + 1)	// if init_genrand() has not been called,
	    init_genrand(5489U);	// a default initial seed is used
	for (kk = 0; kk < N - M; kk++) {
	    y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	    mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1U];
	}
	for (kk = N - M; kk < N - 1; kk++) {
	    y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	    mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1U];
	}
	y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
	mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1U];
	mti = 0;
    }

    y = mt[mti++];

    // Tempering
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= (y >> 18);

    return y;
}
#endif

#ifdef USE_COK_OPTIMIZATION
//......................................................................
static void next_state(void)
{
    int j;
    unsigned int *p = mt;

    // if init_genrand() has not been called,
    // a default initial seed is used
    if (initf == 0)
	init_genrand(5489U);

    left = N ;
    next = mt;

    for (j = N - M + 1; --j; p++)
	*p = p[M] ^ TWIST(p[0], p[1]);

    for (j = M; --j; p++)
	*p = p[M - N] ^ TWIST(p[0], p[1]);

    *p = p[M - N] ^ TWIST(p[0], mt[0]);

    return;
}
#endif

#ifdef TEST
//======================================================================
#include <stdio.h>

int main(void)
{
    int i;
    unsigned int init[4] = { 0x123, 0x234, 0x345, 0x456 };

    init_by_array(init, sizeof(init) / sizeof(init[0]));

    printf("1000 outputs of genrand_int32()\n");
    for (i = 0; i < 1000; i++) {
	printf("%10u ", genrand_int32());
	if (i % 5 == 4)
	    printf("\n");
    }

    printf("\n1000 outputs of genrand_real2()\n");
    for (i = 0; i < 1000; i++) {
	printf("%10.8f ", genrand_real2());
	if (i % 5 == 4)
	    printf("\n");
    }

    return 0;
}
#endif
