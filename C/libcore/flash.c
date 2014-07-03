#include <flash.h>

// global var to contain a magic arm variable
volatile uint32_t flash_protect_var = 0x12345678;

void flash_erase_sector(uint8_t sector) {
    if (flash_protect_var != FLASH_ARM) 
        __asm("illegal");
        
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    
    if (flash_protect_var != FLASH_ARM) 
        __asm("illegal");
        
    FLASH_BASE[0x5555] = 0x80;
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    
    FLASH_BASE[((uint32_t)sector) << 12] = 0x30;
    DELAY_MS(25);          // wait for erase
}

void flash_write_byte(uint8_t *ptr, uint8_t ch) {
    if (flash_protect_var != FLASH_ARM) 
        __asm("illegal");
        
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    
    if (flash_protect_var != FLASH_ARM) 
        __asm("illegal");
        
    FLASH_BASE[0x5555] = 0xA0;

    *ptr = ch;
    DELAY(3);              // waste approximately 20 us to wait for write to complete
}

// this function is last, so that if execution rolls through this region, 
// the flash protect var will not be set when the above code runs.

void flash_arm(uint32_t magic) {
    flash_protect_var = magic;
}


