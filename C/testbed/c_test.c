#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>



//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    millis_start();
    sei();
    
    uint32_t start, end;
    
    lcd_init();
	
	end = millis();
    while(true) {
       // lcd_cursor(0,0);
       // lcd_printf("Runtime: %d.%03d    ",millis()/1000, (millis()%1000));
        start = millis();
        if (start < end) 
            printf("Warning backtick detected: %d vs %d\n",start, end);
            
        end = start;
       // printf("%d\n",end);
    }

    start = millis();
    mem_dump(0x1000,27);
    end = millis();
    
        
}
