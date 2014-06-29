#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>

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

int main() {
   // TIL311 = 0x01;

	default_interrupts();
    serial_start(SERIAL_SAFE);
   // millis_start();
    sei();
    
    memset(0x4000,0,0x30000);
    memset(0x34000,0,0x30000);
    
    //mem_dump(0x4000, 2500);
    
  //  uint32_t i = 0;
 //   DDR = 2;
  //  GPDR = 0;
    
    /*while(true) {
    	if (serial_available()) 
    		putc_sync(getc());*
    	//TIL311 = 0xAA;
   
       // for (char ch = 'a'; ch <= 'z'; ch++)
        //	putc(ch);
       /* puts("Hello world.\n");
        DELAY_MS(1000);
        continue;
        printf("%d\n",i);
       // TIL311 = 0xBB;
   		i++;
        putc('A');
        putc('\n');*/
        
       /* for (uint8_t i = 'a'; i <= 'z'; i++) 
        	putc(i);
        putc('\n');*/
        
        //TIL311 = 0xCC;
   
   // }
    
    
        /* TIL311 = 0xCC;
        DELAY_MS(1000);

        TIL311 = 0xC0;
        DELAY_MS(1000);
        
        TIL311 = 0xDE;
        DELAY_MS(1000);
        
        TIL311 = 0x00;
        DELAY_MS(1000);*/
        
}
