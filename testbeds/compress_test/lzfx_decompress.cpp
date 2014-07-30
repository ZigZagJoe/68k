/*
 * Copyright (c) 2009 Andrew Collette <andrew.collette at gmail.com>
 * http://lzfx.googlecode.com
 *
 * Implements an LZF-compatible compressor/decompressor based on the liblzf
 * codebase written by Marc Lehmann.  This code is released under the BSD
 * license.  License and original copyright statement follow.
 *
 * 
 * Copyright (c) 2000-2008 Marc Alexander Lehmann <schmorp@schmorp.de>
 * 
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 * 
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 * 
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "lzfx.h"

#include <stdint.h>
#include <stdio.h>

#define LZFX_HSIZE (1 << (LZFX_HLOG))

/* We need this for memset */
#ifdef __cplusplus
# include <cstring>
#else
# include <string.h>
#endif

#if __GNUC__ >= 3 && !DISABLE_EXPECT
# define fx_expect_false(expr)  __builtin_expect((expr) != 0, 0)
# define fx_expect_true(expr)   __builtin_expect((expr) != 0, 1)
#else
# define fx_expect_false(expr)  (expr)
# define fx_expect_true(expr)   (expr)
#endif

#ifndef NULL
#define NULL (0)
#endif

typedef unsigned char u8;
typedef const u8 *LZSTATE[LZFX_HSIZE];

/* Define the hash function */
#define LZFX_FRST(p)     (((p[0]) << 8) | p[1])
#define LZFX_NEXT(v,p)   (((v) << 8) | p[2])
#define LZFX_IDX(h)      ((( h >> (3*8 - LZFX_HLOG)) - h  ) & (LZFX_HSIZE - 1))

/* These cannot be changed, as they are related to the compressed format. */
#define LZFX_MAX_LIT        (1 <<  5)
#define LZFX_MAX_OFF        (1 << 13)
#define LZFX_MAX_REF        ((1 << 8) + (1 << 3))

/* Compressed format

    There are two kinds of structures in LZF/LZFX: literal runs and back
    references. The length of a literal run is encoded as L - 1, as it must
    contain at least one byte.  Literals are encoded as follows:

    000LLLLL <L+1 bytes>

    Back references are encoded as follows.  The smallest possible encoded
    length value is 1, as otherwise the control byte would be recognized as
    a literal run.  Since at least three bytes must match for a back reference
    to be inserted, the length is encoded as L - 2 instead of L - 1.  The
    offset (distance to the desired data in the output buffer) is encoded as
    o - 1, as all offsets are at least 1.  The binary format is:

    LLLooooo oooooooo           for backrefs of real length < 9   (1 <= L < 7)
    111ooooo LLLLLLLL oooooooo  for backrefs of real length >= 9  (L > 7)  
*/

/* Decompressor */
int lzfx_decompress(const void* ibuf, unsigned int ilen,
                          void* obuf, unsigned int *olen){

    u8 const *ip = (const u8 *)ibuf;
    u8 const *const in_end = ip + ilen;
    u8 *op = (u8 *)obuf;
    u8 const *const out_end = (olen == NULL ? NULL : op + *olen);
    
    unsigned int remain_len = 0;
    int rc;
    
    do {
        unsigned int ctrl = *ip++;

        /* Format 000LLLLL: a literal byte string follows, of length L+1 */
        if(ctrl < (1 << 5)) {

            ctrl++;

            if(fx_expect_false(op + ctrl > out_end)){
                 //printf("Out of space at %d in input, %d in output, requesting %d\n", ip-((uint8_t*)ibuf), ((uint64_t)op) - ((uint64_t)obuf), ctrl);
                return LZFX_ESIZE; // out of space
            }
            
            if(fx_expect_false(ip + ctrl > in_end)) 
                return LZFX_ECORRUPT; // missing input bytes

            //printf("Literal of %d bytes\n", ctrl);
            do
                *op++ = *ip++;
            while(--ctrl);

        /*  Format #1 [LLLooooo oooooooo]: backref of length L+1+2
                          ^^^^^ ^^^^^^^^
                            A      B
                   #2 [111ooooo LLLLLLLL oooooooo] backref of length L+7+2
                          ^^^^^          ^^^^^^^^
                            A               B
            In both cases the location of the backref is computed from the
            remaining part of the data as follows:

                location = op - A*256 - B - 1
        */
        } else {

            unsigned int len = (ctrl >> 5);
            u8 *ref = op - ((ctrl & 0x1f) << 8) -1;

            if(len==7) len += *ip++;    /* i.e. format #2 */

            len += 2;    /* len is now #octets */

            if(fx_expect_false(op + len > out_end)){
                 //printf("Out of space at %d in input, %d in output, requesting %d\n", ip-((uint8_t*)ibuf), ((uint64_t)op) - ((uint64_t)obuf), len);
                return LZFX_ESIZE; // out of space
            }
            
            if(fx_expect_false(ip >= in_end)) 
                return LZFX_ECORRUPT; // missing input bytes

            ref -= *ip++;

            if(fx_expect_false(ref < (u8*)obuf)) 
                return LZFX_ECORRUPT;

            //printf("Backref of %d bytes, to %d\n", len, ref-((u8*)obuf));
            do
                *op++ = *ref++;
            while (--len);
        }

    } while (ip < in_end);

    *olen = op - (u8 *)obuf;

    return 0;
}

