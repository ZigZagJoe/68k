#include <stdlib.h>
#include <stdio.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>

#include <60hz.h>
#define song_bin __60hz_raw
#define song_len __60hz_raw_len
//#define song_bin 0x84000
//#define song_len 245449

extern uint8_t *volatile wav_start;
extern uint8_t *volatile wav_ptr;
extern uint8_t *volatile wav_end;
extern void wav_isr();

#define INTR_TIMR_B (1 << 0)
#define INTR_GPI2 (1 << 2)

int main() {
    serial_start(SERIAL_SAFE);

    // set up timer B
    TBDR = 29;   // approx 15.9 khz out
	TBCR = 0x1;  // prescaler of 4
       
    // wait for the fifo to clear of garbage
    DELAY_MS(50);
    
    wav_start = song_bin;
    wav_end = wav_start + song_len - 1;
    wav_ptr = wav_start;
    
    if ((*(uint32_t*)256754) != 0xDEADBEEF) {
        (*(uint32_t*)256754) = 0xDEADBEEF; 
    } else {
        (*(uint32_t*)256754) = 0; 
        wav_start = 0x50000;
        wav_ptr = wav_start;
        
        for (uint32_t i = 0; i < 65536; i++) 
            *(wav_ptr++) = i&0xFF;
            
        wav_end = wav_ptr-1;
        wav_ptr = wav_start;
    }
   
    // set up the reload interrupt
	__vectors.user[MFP_INT + MFP_GPI2 - USER_ISR_START] = &wav_isr;

    AER |= (1 << 2);
	IERB |= INTR_GPI2;
	IMRB |= INTR_GPI2;
	    
    // prime the fifo
    memcpy(IO_DEV3, wav_ptr, 512);
    wav_ptr += 512;
       
    sei();
	
	puts("Playback started.\n");
		 
    // wait for end of clip & loop.
    while(1) {
        TIL311 = ((wav_ptr - wav_start) * ((wav_end-wav_start)/16000) / (wav_end-wav_start));
    }
}
