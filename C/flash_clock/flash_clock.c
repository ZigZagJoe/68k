#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <time.h>
#include <lcd.h>
#include <beep.h>

void putc_lcd(void*p, char ch) {
	lcd_data(ch);
}

void lcd_printf(char *fmt, ...) {
    va_list va;
    va_start(va,fmt);
    tfp_format(0,&putc_lcd,fmt,va);
    va_end(va);
}

int main() {
    
    serial_start(SERIAL_SAFE);
    millis_start();
    sei();

	lcd_init();
  	lcd_puts("Time since boot:");
        
    while(true) {
        if (serial_available())
            putc(getc());
            
        lcd_cursor(0,1);
        lcd_printf("%3d.%02d seconds",millis()/1000, (millis()%1000)/10);
        TIL311 = millis() / 1000;
    }
}
