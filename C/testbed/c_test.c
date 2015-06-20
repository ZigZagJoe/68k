#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>
#include <kernel.h>


//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;
    serial_start(SERIAL_SAFE);
    sei();

    DDR |= 2;
    while(true) {
        bset_a(GPDR, 1);
        DELAY_US(40);
        bclr_a(GPDR, 1);
        DELAY_US(40);
    }
    
}

void enter_critical() {}
void leave_critical() {}
