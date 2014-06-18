#include <stdlib.h>
#include <stdio.h>

uint32_t x = (uint32_t)__DATE__;
uint32_t y = (uint32_t)__TIME__;
uint32_t z = &__heap_start;
uint32_t w = &__data_start;
 
uint32_t rand(void) {
    uint32_t t;
 
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) * 3120909123 ^ t ^ (t >> 8);
}
