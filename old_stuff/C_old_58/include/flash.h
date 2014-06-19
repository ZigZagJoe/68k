#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>
#include <stdlib.h>

#define ADDR_TO_SECTOR(X) (((X) & 0x3FFFF) >> 12)
#define FLASH_BASE  ((uint8_t*)0x80000)

void flash_erase_sector(uint8_t sector);
void flash_write_byte(uint8_t *ptr, uint8_t ch);

#endif
