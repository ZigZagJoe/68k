#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <stdint.h>
#include <binary.h>
#include <random.h>

extern uint8_t *__heap_start;
extern uint8_t *__heap_end;
extern uint8_t *__stack_start;
extern uint8_t *__stack_end;
extern uint8_t *__data_start;

#define DELAY_BIG(X) __asm volatile("move.l %0,%%d0\n" \
                                "1: subi.l #1, %%d0\n" \
                                "bne 1b\n" \
                                ::"i"(X):"d0");

#define DELAY_SML(X) __asm volatile("move.w %0,%%d0\n" \
                                "1: dbra %%d0, 1b\n" \
                                ::"i"(X):"d0");
// @ 8mhz, on 68008
#define MILLISECOND_DELAY 218
#define MICROSECOND_DELAY .5525

#define DELAY_US(x) if (x > 50000) { DELAY_MS(x/1000); } else { DELAY_SML((short)(x*MICROSECOND_DELAY)); }
    
#define DELAY_MS(x) DELAY_BIG(x*MILLISECOND_DELAY)
#define DELAY_S(x) DELAY_MS(x*1000);
#define DELAY_HALF_SECOND DELAY_MS(500);

extern void sleep_for(uint32_t time);

// blink two values on TIl311 display forever
#define HALT_CODE(A,B)  while(1) { \
        TIL311 = A; \
        DELAY_MS(750); \
        TIL311 = B; \
        DELAY_MS(750); \
    }
    
#define _JMP(X) __asm volatile("jmp (%0)\n"::"a"(X))

#define true 1
#define false 0

#define TRUE 1
#define FALSE 0

#define bv(BIT) (1 << BIT)
#define bset(X,BIT) (X |= bv(BIT))
#define bclr(X,BIT) (X &= ~bv(BIT))
#define bisset(X,BIT) (X & bv(BIT))

#define bset_a(X,BIT)  __asm volatile("bset.b %0, (%1)\n"::"i"(BIT),"a"(&X))
#define bclr_a(X,BIT)  __asm volatile("bclr.b %0, (%1)\n"::"i"(BIT),"a"(&X))

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) (((x) > 0) ? (x) : (-(x)))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
  
#define ROL(num, bits) (((num) << (bits)) | ((num) >> (32 - (bits))))

#endif
