#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <stdint.h>
#include <stdlib.h>

#define RAND_BASE 0x5F000

void srand(void);
uint32_t rand(void);
uint32_t get_random_seed();

#endif
