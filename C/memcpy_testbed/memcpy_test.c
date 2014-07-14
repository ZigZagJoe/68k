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

uint32_t crc_region (uint8_t * start, uint32_t sz) ;
void test(uint32_t src, uint32_t dest, uint32_t len) ;
uint32_t hash( uint32_t a);
void mem_dump(uint8_t *addr, uint32_t cnt);

int main() {
    TIL311 = 0x01;

	serial_start(SERIAL_SAFE);
    millis_start();
    sei();
    
    
    uint32_t ha = 124912341;
    uint8_t c = 0;
	for (uint32_t i = 0x10000; i < 0x20100; i += 4) {
		*((uint32_t*)(i)) = ha;
		ha = hash(ha + i);
		//*((uint8_t*)(i)) = c++;
		//*((uint8_t*)(i + 0x20000)) = c++;
	}
	
	uint32_t start, end;
	
	puts("Source\n");
	mem_dump(0x10000, 256);
	mem_dump(0x1FF00, 384);
	
	puts("Dest org\n");
	mem_dump(0x30000, 256);
	mem_dump(0x3FF00, 384);
	
	start = millis();
	memset(0x30000, 'C', 0x10000);
	end = millis();
	
	memset(0x40000, '#', 128);
	
	printf("Clear done in %d\n", end-start);
	
	puts("Dest clr\n");
	mem_dump(0x30000, 256);
	mem_dump(0x3FF00, 384);
	
	start = millis();
	memcpy(0x30000,0x10000,0x10000);
	end = millis();
	
	printf("Copy done in %d\n", end-start);
	
    puts("After copy\n");
	mem_dump(0x30000, 256);
	mem_dump(0x3FF00, 384);
	
	test(0x10000, 0x30000, 10);
    test(0x10000, 0x31000, 100);
	test(0x10000, 0x30300, 1337);
	test(0x100e1, 0x30100, 1337);
	test(0x100e0, 0x30001, 213);
	test(0x100e1, 0x30001, 123);
	test(0x10000, 0x30000, 3);
	test(0x10011, 0x30010, 1);
	test(0x10000, 0x30011, 1);
	test(0x10009, 0x30011, 4);
	test(0x10000, 0x30000, 0x1000);
	test(0x10000, 0x30000, 0x10000);
	test(0x10000, 0x30000, 127);
	test(0x10000, 0x30000, 128);
	
    while(true);
    
   /*
   char * str2 = "embedded str";
	simp_printf("hello world\nhex= 0x%x\nstring= %s\nbinary= %b\npercent %%\nchar= '%c'\nu_dec= '%u'\nu_dec_oor= '%u'\n", 0xDEADBEEF, str2, 0xCAFEBABE, 'h',132874,655369);
	
	simp_printf("s_dec= '%d'\n-s_dec= '%d'\ns_dec_oor= '%d'\n-s_dec_oor= '%d'\n", 21345,-10982,695369,-655600);
	*/
	
	while(true)
	    printf("%08x\n",rand());
	
 	
}	
	

void mem_dump(uint8_t *addr, uint32_t cnt) {
    int c = 0;
    char ascii[17];
    ascii[16] = 0;
    
    printf("\n%8X   ", addr);
    for (uint32_t i = 0; i < cnt; i++) {
        uint8_t b = *addr;
        
        ascii[c] = (b > 31 & b < 127) ? b : '.';
        
        printf("%02X ",b);
        
        addr++;
        
        if (++c == 16) {
            printf("  %s\n",ascii);
            if ((i+1) < cnt)
                printf("%8X   ", addr);
            c = 0;
        }
    }
    
    if (c && c < 15) {
        ascii[c] = 0; // null terminate
        while (c++ < 16)
            printf("   ");
        printf("  %s\n",ascii);
    } else printf("\n");
}

uint32_t hash( uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}


uint32_t crc_region (uint8_t * start, uint32_t sz) {
	uint32_t crc = 0xDEADC0DE;
	
	for (uint32_t i = 0; i < sz; i++)
		crc = qcrc_update(crc, start[i]);
		
	printf("QCRC of %d bytes from 0x%x: 0x%X\n", sz,start, crc);
	return crc;
}

	
void test(uint32_t src, uint32_t dest, uint32_t len) {
	uint32_t start, end;
	
	start = millis();
	memset(dest, 0, len);
	end = millis();
	printf("Clear: %d elapsed\n", end-start);
	
	start = millis();
	memcpy(dest, src, len);
	end = millis();
	
	printf("Copy: %d elapsed\n", end-start);
	
	if (crc_region(src, len) != crc_region(dest, len)) 
		printf("ERROR: CRC MISMATCH!\n");

    putc('\n');
}

