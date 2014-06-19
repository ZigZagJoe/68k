#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <printf.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <flash.h>
#include <time.h>
#include <lcd.h>

ISR(address_err)  { HALT_CODE(0xEE,0xAE); }
ISR(bus_error)    { HALT_CODE(0xEE,0xBE); }
ISR(illegal_inst) { HALT_CODE(0xEE,0x11); }
ISR(divby0)       { HALT_CODE(0xEE,0x70); }
ISR(bad_isr)      { HALT_CODE(0xEE,0x1B); }
ISR(spurious)     { HALT_CODE(0xEE,0x1F); }
ISR(auto_lvl2)    { HALT_CODE(0xEE,0x12); }
ISR(priv_vio)     { HALT_CODE(0xEE,0x77); }

void printf_putc(void*p, char ch) {
	putc(ch);
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

/*
#include <md5.h>

uint32_t hash( uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

	md5_state_t state;
	uint8_t digest[16];
	
	while (true) {
		//puts("Fill memory\n");

		md5_init(&state);
	
		uint32_t ha = 124912341;
        uint8_t tmp;
        
        TIL311 = 0x1A;
        
		for (uint32_t i = 0x6000; i < 0x78000; i += 1) {
			*((uint8_t*)(i)) = ha&0xFF;
			ha = hash(ha + i);
		}
		
		//puts("Do hash\n");
	
		for (uint32_t i = 0x6000; i < 0x78000; i += 64) {
            TIL311 = (uint8_t*)i;
			md5_append(&state, (uint8_t*)i, 64);
		}
	
		md5_finish(&state, digest);
   
	   // printf("Done\n");

		char *hex_chars = "0123456789ABCDEF";
	
		for (uint8_t i = 0; i < 16; i++) {
			putc(hex_chars[digest[i]>>4]);
			putc(hex_chars[digest[i]&0xF]);
		}
	
		putc('\n');
	}
*/
void mario();

void printf_lcd(void*p, char ch) {
	lcd_data(ch);
}

int main() {
    TIL311 = 0x01;
    
    __vectors.address_error = &address_err;
    __vectors.bus_error = &bus_error;
    __vectors.illegal_instr = &illegal_inst;
    __vectors.divide_by_0 = &divby0;
    __vectors.uninitialized_isr = &bad_isr;
    __vectors.int_spurious = &spurious;
    __vectors.auto_level2 = &auto_lvl2;
    __vectors.priv_violation = &priv_vio;
    
    serial_start();
    millis_start();
    
    init_printf(null, &printf_putc);

	lcd_init();
	lcd_puts("Hello from C,\non the 68008!");
	

    mario();
    
    init_printf(null, &printf_lcd);

/*	while(true) {
		lcd_cursor(0,0);
        printf("Runtime: %d.%02d ",millis/1000, (millis%1000)/10);
    }*/
    
    while(true) {
        TIL311 = 0xCC;
        DELAY_MS(1000);

        TIL311 = 0xC0;
        DELAY_MS(1000);
        
        TIL311 = 0xDE;
        DELAY_MS(1000);
        
        TIL311 = 0x00;
        DELAY_MS(1000);
               
        if(serial_available()) {
			int16_t ch;
			lcd_clear();
			uint8_t c = 0;
			
			while ((ch = getc()) != -1) {
				putc(ch);
				lcd_data(ch);
				if (c++ == 15)
					lcd_cursor(0,1);
			}
        } else {
        	lcd_clear();
        	lcd_puts("Time since boot:");
        	lcd_cursor(0,1);
        	printf("%3d.%02d seconds",millis()/1000, (millis()%1000)/10);
        }
    }
}
