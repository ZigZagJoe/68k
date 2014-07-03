#include <stdio.h>
#include <string.h>
#include <random.h>
#include <time.h>

#define MEM16(X) (*((volatile uint16_t *)(X))

uint32_t x = (uint32_t)__DATE__;
uint32_t y = (uint32_t)__TIME__;
uint32_t z = &__heap_start;
uint32_t w = &__data_start;
 
// PRNG function
uint32_t rand(void) {
    uint32_t t;
 
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) * 3120909123 ^ t ^ (t >> 8);
}

// PRNG seed with random data from sram
void srand(void) {
    x = get_random_seed();
    y = get_random_seed();
    z = get_random_seed();
    w = get_random_seed();
}

// make a random seed from some random data in ram
uint32_t get_random_seed() {

    uint32_t crc = *(uint32_t*)RAND_BASE;
    uint16_t seedA = *(uint16_t*)RAND_BASE;
    uint16_t seedB = *(uint16_t*)(RAND_BASE+2);

    uint8_t count = seedA & 0xF | ((seedB&0xF) << 4);
    
    if (!count)
        count = 255;
        
    if (!seedA)
        seedA = __heap_start;
        
   if (!seedB)
        seedB = __data_start;
        
    while (count < 50)
        count = (count << 1) + 7;
    
    uint16_t addr = ((seedA >> 4) | ((seedB & 0xF0) << 4)) & (~1);
    
    crc += 321849570;  
    
    uint16_t num;
    uint8_t tmp;
    
    while (count) {
        num = *(volatile uint16_t*)(RAND_BASE+addr);
        addr = (num >> 4) & (~1);
        
        if (tmp) {
            tmp |= num & 0xF;
            crc = qcrc_update(crc, tmp) + seedA;
            count--;     
            tmp = 0;
        } else {
            tmp = (num & 0xF) << 4;
        }  
    }
    
    *(uint32_t*)RAND_BASE = crc ^ millis();
    
    return crc;
}

