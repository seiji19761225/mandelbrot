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
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
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

#ifndef _MT19937_H_
#define _MT19937_H_

#ifdef _MT19937_INTERNEL_
#define MT19937_API
#else
#define MT19937_API	extern
#endif

// int    genrand_int31() : generates a random number on [0,0x7fffffff]-interval
#define genrand_int31()		(genrand_int32()>>1)
// These real versions are due to Isaku Wada, 2002/01/09 added
// double genrand_real1() : generates a random number on [0,1]-real-interval
// double genrand_real2() : generates a random number on [0,1)-real-interval
// double genrand_real3() : generates a random number on (0,1)-real-interval
// double genrand_real53(): generates a random number on [0,1) with 53-bit resolution
#define genrand_real1()		((double) genrand_int32()/4294967295.0)
#define genrand_real2()		((double) genrand_int32()/4294967296.0)
#define genrand_real3()		(((double) genrand_int32()+0.5)/4294967296.0)
#define genrand_real53()	(((genrand_int32()>>5)*67108864.0+(genrand_int32()>>6))/9007199254740992.0)

#ifdef __cplusplus
extern "C" {
#endif

MT19937_API void         init_genrand (unsigned int);
MT19937_API void         init_by_array(unsigned int *, int);
MT19937_API unsigned int genrand_int32(void);

#ifdef __cplusplus
}
#endif
#undef MT19937_API
#endif
