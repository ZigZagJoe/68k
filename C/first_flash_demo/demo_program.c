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

extern char * songs[];
void play_rtttl(char *p);


int main() {
    TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    millis_start();
    sei();

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
