#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <md5.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>

ISR(address_err)  { HALT_CODE(0xEE,0xAE); }
ISR(bus_error)    { HALT_CODE(0xEE,0xBE); }
ISR(illegal_inst) { HALT_CODE(0xEE,0x11); }
ISR(divby0)       { HALT_CODE(0xEE,0x70); }
ISR(bad_isr)      { HALT_CODE(0xEE,0x1B); }
ISR(spurious)     { HALT_CODE(0xEE,0x1F); }
ISR(auto_lvl2)    { HALT_CODE(0xEE,0x12); }
ISR(priv_vio)     { HALT_CODE(0xEE,0x77); }

char *hex_chars = "0123456789ABCDEF";


uint32_t hash( uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}


int main() {
    TIL311 = 0x70;
    DELAY_S(1);
    
    __vectors.address_error = &address_err;
    __vectors.bus_error = &bus_error;
    __vectors.illegal_instr = &illegal_inst;
    __vectors.divide_by_0 = &divby0;
    __vectors.uninitialized_isr = &bad_isr;
    __vectors.int_spurious = &spurious;
    __vectors.auto_level2 = &auto_lvl2;
    __vectors.priv_violation = &priv_vio;
    
    serial_setup();
    sei();
    
    
    md5_state_t state;
	uint8_t digest[16];
	
    
    md5_init(&state);
	uint32_t ha = 124912341;
	
    for (uint32_t i = 0x6000; i < 0x78000; i++) {
    	uint8_t v = ha & 0xFF;
        md5_append(&state, &v, 1);
        ha = hash(ha);
    }
    
	md5_finish(&state, digest);
    
    puts("done\n");
    
   
    for (uint8_t i = 0; i < 16; i++) {
        putc(hex_chars[digest[i]>>4]);
        putc(hex_chars[digest[i]&0xF]);
    }
    
    putc('\n');
    
    /////////////////////////////////////////
    puts("Setting RAM\n");
    
    
    ha = 124912341;
    for (uint32_t i = 0x6000; i < 0x78000; i++) {
        volatile uint8_t *ptr = ((volatile uint8_t *)(i));
        
        *ptr = ha & 0xFF;
        ha = hash(ha);
    }
    
    puts("Do md5\n");

	md5_init(&state);
	
    for (uint32_t i = 0x6000; i < 0x78000; i++) {
        volatile uint8_t *ptr = ((volatile uint8_t *)(i));
        md5_append(&state, ptr, 1);
    }
	
    
	md5_finish(&state, digest);
   
    puts("done\n");

    for (uint8_t i = 0; i < 16; i++) {
        putc(hex_chars[digest[i]>>4]);
        putc(hex_chars[digest[i]&0xF]);
    }
    
    putc('\n');
    
    while(true) {
        TIL311 = 0xCC;
        DELAY_HALF_SECOND;
        TIL311 = 0xC0;
        DELAY_HALF_SECOND;
        TIL311 = 0xDE;
        DELAY_HALF_SECOND;
        TIL311 = 0x11;
        DELAY_HALF_SECOND;
       
        int16_t ch;
        while ((ch = getc()) != -1)
            putc(ch);
    }
}
