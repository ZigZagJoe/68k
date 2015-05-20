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
    
    
    bset (DDR, 6);
    
    
    // 1ms = 169        90
    // 252 = center
    // 2ms = 169 * 2    -90
   /* while(1) {
        bset(GPDR,6);
        DELAY(252);
        bclr(GPDR,6);
        DELAY_MS(20);
    }*/
    
    
    
   /* TCDCR |= 0x3 << 4;
    // 220 = max travel one way

    int8_t dir = 1;
    uint8_t num = 110;
    
    while(true) {
        num += dir;
        TCDR = num;
        
        DELAY_MS(20);
        
        if (num >= 220) {
            dir = -1;
        } else if (num <= 110)
            dir = 1;
    }
    */
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
