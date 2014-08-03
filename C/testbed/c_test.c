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
    

    start = millis();
    mem_dump(0x1000,27);
    end = millis();
    
        
}
