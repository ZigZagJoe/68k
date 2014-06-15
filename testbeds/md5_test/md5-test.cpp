#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "md5.h"

uint32_t hash( uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

int main(int argc, char** argv) {
	
	
	uint8_t v = 0;
	
	uint32_t * data = (uint32_t*)malloc(0x78000);
	uint8_t * bytes = (uint8_t*)data;
	
	if (!data) {
		printf("malloc failed");
	}
	
	 md5_state_t state;
	uint8_t digest[16];
	
	md5_init(&state);
	
	uint32_t ha = 124912341;
	
	uint32_t start = 0x10000;
	uint32_t end = 0x60000;
	
	for (uint32_t i = start; i < end; i += 1) {
		*((uint8_t*)(bytes + i)) = ha&0xFF;
		ha = hash(ha + i);
	}
    	
	
    for (uint32_t i = start; i < end; i += 64) {
    	md5_append(&state, bytes+i, 64);
    }
    
	md5_finish(&state, digest);
   
    printf("done\n");
    
    char *hex_chars = "0123456789ABCDEF";
    
    for (uint8_t i = 0; i < 16; i++) {
        putc(hex_chars[digest[i]>>4],stdout);
        putc(hex_chars[digest[i]&0xF],stdout);
    }
    
    putc('\n',stdout);


    return 0;
}
