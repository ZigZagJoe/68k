#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>

int main() {
   // TIL311 = 0x01;

	default_interrupts();
    serial_start(SERIAL_SAFE);
   // millis_start();
    sei();
    
    uint32_t i = 0;
    DDR = 2;
    GPDR = 0;
    
    while(true) {
    	//TIL311 = 0xAA;
   
       // for (char ch = 'a'; ch <= 'z'; ch++)
        //	putc(ch);
        puts("Hello world.\n");
        DELAY_MS(1000);
        continue;
        printf("%d\n",i);
       // TIL311 = 0xBB;
   		i++;
        putc('A');
        putc('\n');
        //TIL311 = 0xCC;
   
    }
    
    
        /* TIL311 = 0xCC;
        DELAY_MS(1000);

        TIL311 = 0xC0;
        DELAY_MS(1000);
        
        TIL311 = 0xDE;
        DELAY_MS(1000);
        
        TIL311 = 0x00;
        DELAY_MS(1000);*/
        
}
