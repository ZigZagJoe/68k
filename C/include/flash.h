#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>

#ifndef DELAY
  #include <stdlib.h>
#endif

#define FLASH_ARM  0xF1AC0DE0

#define ADDR_TO_SECTOR(X) (((X) & 0x3FFFF) >> 12)
#define FLASH_BASE  ((volatile uint8_t*)0x80000)

__attribute__ ((noinline)) void flash_erase_sector(uint8_t sector);
__attribute__ ((noinline)) void flash_write_byte(uint8_t *ptr, uint8_t ch);
__attribute__ ((noinline)) void flash_arm(uint32_t magic);

#endif
