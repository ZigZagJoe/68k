#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <printf.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>

void mem_dump(uint8_t *addr, uint32_t cnt);
uint32_t hash( uint32_t a);

uint32_t crc_region (uint8_t * start, uint32_t sz) {
	uint32_t crc = 0xDEADC0DE;
	
	for (uint32_t i = 0; i < sz; i++)
		crc = qcrc_update(crc, start[i]);
		
	printf("QCRC of %d to %d: 0x%X\n", start, sz, crc);
	return crc;
}

int main() {
    TIL311 = 0x01;

	default_interrupts();
    serial_start(SERIAL_SAFE);
    millis_start();
    uint32_t start, end;
	
   	char * str1 = "hello world!";
   	
   	mem_dump(str1, 16);
   	printf("strlen str1: %d\n",strlen(str1));
   	
   	char str2[128];
   	
   	memset(str2, '#', 16);
   	mem_dump(str2, 16);
   
    strcpy(str2, str1);
   	
   	mem_dump(str2, 16);
   
	
 	while(true);
}

void mem_dump(uint8_t *addr, uint32_t cnt) {
    int c = 0;
    char ascii[17];
    ascii[16] = 0;
    
    putc('\n');
    
    printf("%8X   ", addr);
    for (uint32_t i = 0; i < cnt; i++) {
        uint8_t b = *addr;
        
        ascii[c] = (b > 31 & b < 127) ? b : '.';
        
        printf("%02X ",b);
        
        addr++;
        
        if (++c == 16) {
            puts("  ");
            puts(ascii);
            putc('\n');
            if ((i+1) < cnt)
                printf("%8X   ", addr);
            c = 0;
        }
    }
    
    if (c < 15) putc('\n');
}
