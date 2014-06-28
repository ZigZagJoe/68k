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
#include <beep.h>

ISR(address_err)  { HALT_CODE(0xEE,0xAE); }
ISR(bus_error)    { HALT_CODE(0xEE,0xBE); }
ISR(illegal_inst) { HALT_CODE(0xEE,0x11); }
ISR(divby0)       { HALT_CODE(0xEE,0x70); }
ISR(bad_isr)      { HALT_CODE(0xEE,0x1B); }
ISR(spurious)     { HALT_CODE(0xEE,0x1F); }
ISR(auto_lvl2)    { HALT_CODE(0xEE,0x12); }
ISR(priv_vio)     { HALT_CODE(0xEE,0x77); }

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


char* byte_to_bin_str(uint8_t arg) {
	static char zero[] = "0";
	
	if (arg == 0) 
		return (char*)&zero;

	// number of bits in argument
	const int bits = sizeof(arg) * 8;

	// static is not thread safe - remove if threading
	// max number of bits to be displayed + null char
	static char buff[bits + 1];
	// pointer to end of buffer 
	char *pos = (char*)&buff + bits;

	*(pos--) = 0; // null termination

	while (arg) {
		// set char, decrement pointer
		*(pos--) = '0' + (arg & 1);
		arg = arg >> 1;
	}

	return pos + 1;
}

extern char * songs[];
void play_rtttl(char *p);

void putc_lcd(void*p, char ch) {
	lcd_data(ch);
}

void lcd_printf(char *fmt, ...)
{
    va_list va;
    va_start(va,fmt);
    tfp_format(0,&putc_lcd,fmt,va);
    va_end(va);
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
    
    serial_start(SERIAL_SAFE);
    millis_start();

	lcd_init();
	lcd_puts("Hello from C,\non the 68008!");

    beep(700,250);
    DELAY_MS(50);
 	beep(500,300);
  	DELAY_MS(100);
  	beep(700,500);
  	DELAY_MS(500);
  	
  	play_rtttl(songs[0]);
   
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
        	lcd_printf("%3d.%02d seconds",millis()/1000, (millis()%1000)/10);
        }
    }
}
