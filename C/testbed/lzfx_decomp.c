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
                          void* obuf, unsigned int olen){

    uint8_t const *in_ptr = (const uint8_t *)ibuf;
    uint8_t *out_ptr = (uint8_t *)obuf;
    
    uint8_t const *const in_end = in_ptr + ilen;
    uint8_t const *const out_end = out_ptr + olen;
    
    do {
        uint16_t ctrl = *in_ptr++;

        /* Format 000LLLLL: a literal byte string follows, of length L+1 */
        if(ctrl < (1 << 5)) {

            ctrl++;

         /*   if((out_ptr + ctrl) > out_end)
                return LZFX_ESIZE; // out of space
            
            if((in_ptr + ctrl) > in_end)
                return LZFX_ECORRUPT; // missing input bytes
*/
            do {
                *out_ptr++ = *in_ptr++;
            } while(--ctrl);

        /*  Format #1 [LLLooooo oooooooo]: backref of length L+1+2
                          ^^^^^ ^^^^^^^^
                            A      B
                   #2 [111ooooo LLLLLLLL oooooooo] backref of length L+7+2
                          ^^^^^          ^^^^^^^^
                            A               B
            In both cases the location of the backref is computed from the
            remaining part of the data as follows:

                location = out_ptr - A*256 - B - 1
        */
        } else {
            uint16_t len = (ctrl >> 5);
            uint8_t *ref = out_ptr - ((ctrl & 0x1f) << 8) - 1;

            if(len == 7) /* format #2 */
                len += *in_ptr++; 

            len += 2;    /* len is now #octets */

            /*if((out_ptr + len) > out_end)
                return LZFX_ESIZE; // out of space
            
            if(in_ptr >= in_end)
                return LZFX_ECORRUPT; // missing input bytes
*/
            ref -= *in_ptr++;

          /*  if(ref < (uint8_t*)obuf)
                return LZFX_ECORRUPT;
*/
            do {
                *out_ptr++ = *ref++;
            } while (--len);
        }

    } while (in_ptr < in_end);

    return out_ptr - (uint8_t *)obuf;
}

