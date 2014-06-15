#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <stdint.h>
#include <binary.h>

extern uint32_t __heap_start;
extern uint32_t __heap_end;
extern uint32_t __stack_start;
extern uint32_t __stack_end;

//48 + (x-1) * 26
// approx count of cycles for this loop on 68008
// 170665 should be approx half a second @ 6.144mhz
#define DELAY(X) __asm volatile("move.l %0,%%d0\n" \
                                "1: sub.l #1, %%d0\n" \
                                "jne 1b\n" \
                                ::"i"(X):"d0");


#define MILLISECOND_DELAY 205
#define DELAY_S(x) DELAY_MS(x*1000);
#define DELAY_MS(x) DELAY(x*MILLISECOND_DELAY)
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
#define null 0

#define TRUE 1
#define FALSE 0
#define NULL 0

#define bv(BIT) (1 << BIT)
#define bset(X,BIT) (X |= bv(BIT))
#define bclr(X,BIT) (X &= ~bv(BIT))
#define bisset(X,BIT) (X & bv(BIT))

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
  
#endif
