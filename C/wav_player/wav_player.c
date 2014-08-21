#include <stdlib.h>
#include <stdio.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>

extern volatile uint8_t *wav_ptr;
extern volatile uint8_t *wav_end;
extern void wav_isr();

#define INTR_TIMR_B (1 << 0)
#define INTR_GPI3 (1 << 3)

int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
   // millis_start();
  /* uint8_t c = 0;
   while(1) {
        TIL311 = c;
        DELAY(20);
        c++;
    }*/
 
    __vectors.user[MFP_INT + MFP_GPI3 - USER_ISR_START] = &wav_isr;

    TBDR = 29;   // approx 15.9 khz out
	TBCR = 0x1;  // prescaler of 4
     
	AER |= (1 << 2);
	IERB |= INTR_GPI3;
	IMRB |= INTR_GPI3;
	    
    wav_ptr = 0x4000;
    wav_end = wav_ptr + 428289;
    sei();
   
    DDR |= 1 << 1;
    
    /*__asm volatile ("lb: move.b #2, (0xC0001)\n"
    					 "move.b #0, (0xC0001)\n"
    					 "bra lb\n");		*/
    for (int i = 0; i < 512; i++)
        TIL311 = *wav_ptr++;	
   // wav_isr();			 
    while(1) {
        if (wav_ptr == wav_end)
            wav_ptr = 0x4000;
    }
	
}
