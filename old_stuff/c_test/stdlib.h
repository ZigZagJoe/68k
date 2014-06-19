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

#define DELAY_HALF_SECOND DELAY(170665)
#define DELAY_S(x) DELAY(170665*2*x)
#define DELAY_MS(x) DELAY(x*340)

// blink two values on TIl311 display forever
#define HALT_CODE(A,B)  while(1) { \
        TIL311 = A; \
        DELAY_HALF_SECOND; \
        TIL311 = B; \
        DELAY_HALF_SECOND; \
    }
    
#endif
