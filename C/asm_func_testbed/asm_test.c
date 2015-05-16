#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <printf.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <time.h>

uint16_t _strlen(char* str);

void return_to_loader() {
    putc('\n');
    while (tx_busy());  // wait for any pending characters to be sent
    DELAY(100);         // wait for last character to be sent (otherwise MFP will be reset and char not sent)
    __asm ("move.w #0xB007, 0x400\n" /* cause loader to not boot from flash */
            "jmp 0x80008\n"          /* jump to loader */
    );
}
int main() {
    TIL311 = 0x01;

	serial_start(SERIAL_SAFE);
    //millis_start();
    sei();
    
    uint8_t *addr = 0x40000;
    
    for (uint16_t i = 0; i < 300; i++)
        *(addr++) = i & 0xFF;
     
    return_to_loader();
}
