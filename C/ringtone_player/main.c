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

extern char * songs[];
void play_rtttl(char *p);

#define songcount 24

int main() {
    TIL311 = 0xDE;
    
    __vectors.address_error = &address_err;
    __vectors.bus_error = &bus_error;
    __vectors.illegal_instr = &illegal_inst;

    serial_start(SERIAL_SAFE);
  
	lcd_init();
	lcd_puts("Hello from C,\non the 68008!");
	
	for (int i = 0; i < songcount; i++) {
		play_rtttl(songs[i]);
		DELAY_S(1);
	}
		
    while(true) {
        TIL311 = 0xCC;
        DELAY_MS(1000);

        TIL311 = 0xC0;
        DELAY_MS(1000);
        
        TIL311 = 0xDE;
        DELAY_MS(1000);
    }
}
