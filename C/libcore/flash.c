#include <flash.h>

void flash_erase_sector(uint8_t sector) {
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    FLASH_BASE[0x5555] = 0x80;
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    FLASH_BASE[((uint32_t)sector) << 12] = 0x30;
    DELAY_MS(25);
}

void flash_write_byte(uint8_t *ptr, uint8_t ch) {
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    FLASH_BASE[0x5555] = 0xA0;
    *ptr = ch;
    DELAY(3); // waste approximately 20 us to wait for write to complete
}


/* 
uint32_t baseaddr = 0x80000 + (1 << 14);

printf("%d\n",ADDR_TO_SECTOR(baseaddr));
printf("%d\n",ADDR_TO_SECTOR(0x80000 + (1 << 15)));
printf("%d\n",ADDR_TO_SECTOR(0x80000 + (37 * 4096)));

do_dump(baseaddr, 256);

flash_write_byte(ADDR(baseaddr),0xDE);
flash_write_byte(ADDR(baseaddr)+1,0xAD);
flash_write_byte(ADDR(baseaddr)+2,0xBE);
flash_write_byte(ADDR(baseaddr)+3,0xEF);

do_dump(baseaddr, 256);

flash_erase_sector(4);

do_dump(baseaddr, 256);*/

