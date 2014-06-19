#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>

ISR(address_err)  { HALT_CODE(0xEE,0xAE); }
ISR(bus_error)    { HALT_CODE(0xEE,0xBE); }
ISR(illegal_inst) { HALT_CODE(0xEE,0x11); }
ISR(divby0)       { HALT_CODE(0xEE,0x70); }
ISR(bad_isr)      { HALT_CODE(0xEE,0x1B); }
ISR(spurious)     { HALT_CODE(0xEE,0x1F); }
ISR(auto_lvl2)    { HALT_CODE(0xEE,0x12); }
ISR(priv_vio)     { HALT_CODE(0xEE,0x77); }


/*
#include <printf.h>
void printf_putc(void*c, char ch) {
    putc(ch);
}

ISR(trap13) {
    printf("Hi from trap #13!\r\n");
}
 /* __vectors.trap[13] = &trap13;
 init_printf(null, &printf_putc);
 printf("Hi!\r\n");
 __TRAP(13);
 printf("Back in C!\r\n");*/



int main() {
    TIL311 = 0x70;
    DELAY_S(1);
    
    __vectors.address_error = &address_err;
    __vectors.bus_error = &bus_error;
    __vectors.illegal_instr = &illegal_inst;
    __vectors.divide_by_0 = &divby0;
    __vectors.uninitialized_isr = &bad_isr;
    __vectors.int_spurious = &spurious;
    __vectors.auto_level2 = &auto_lvl2;
    __vectors.priv_violation = &priv_vio;
    
    enable_serial();
    sei();
    
    //__Vectors.user[MFP_INT +
    
    //UCR = B10001000
    
    while(true) {
        TIL311 = 0xCC;
        DELAY_HALF_SECOND;
        TIL311 = 0xC0;
        DELAY_HALF_SECOND;
        TIL311 = 0xDE;
        DELAY_HALF_SECOND;
        TIL311 = 0x11;
        DELAY_HALF_SECOND;
       
        int16_t ch;
        while ((ch = getc()) != -1)
            putc(ch);
    }
}
