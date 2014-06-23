#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>

#ifndef DELAY
#define DELAY(X) __asm volatile("move.l %0,%%d0\n" \
                                "1: subi.l #1, %%d0\n" \
                                "bne 1b\n" \
                                ::"i"(X):"d0");

// @ 8mhz, on 68008
#define MILLISECOND_DELAY 174
#define DELAY_MS(x) DELAY(x*MILLISECOND_DELAY)
#endif

#define ADDR_TO_SECTOR(X) (((X) & 0x3FFFF) >> 12)
#define FLASH_BASE  ((volatile uint8_t*)0x80000)

void flash_erase_sector(uint8_t sector);
void flash_write_byte(uint8_t *ptr, uint8_t ch);

#endif
