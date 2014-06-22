#include <flash.h>

void printf(char * fmt, ...);

uint8_t parse_srec(uint8_t * start, uint32_t len, uint8_t flags) {  
    printf("start=0x%x, len=0x%x, flags=0x%x\n", start, len, flags);
    return len;
}
