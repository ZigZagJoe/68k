#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <flash.h>

ISR(address_err)  { HALT_CODE(0xEE,0xAE); }
ISR(bus_error)    { HALT_CODE(0xEE,0xBE); }

#define PROGRAM_VECTOR FLASH_BASE[4096]

int main() {
    TIL311 = 0x0B;

    __vectors.address_error = &address_err;
    __vectors.bus_error = &bus_error;
    
    serial_start();
    
    TIL311 = 0xDD;
    
    bool enter_loader = true;

    for (uint32_t i = 0; i < 100000; i++)
        if(!GPIO(0)) {
            enter_loader = true;
            break;
        }

    if (!enter_loader) {
        cli();
        serial_stop();
        TIL311 = 0xBB;
        _JMP(PROGRAM_VECTOR);
    }
    
    TIL311 = 0xB1;
    
    putc(0xDE);
    putc(0xAD);
    putc(0xC0);
    putc(0xDE);

}
