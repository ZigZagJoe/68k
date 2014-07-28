#include <time.h>
#include <interrupts.h>
#include <io.h>

volatile extern uint32_t millis_counter;
extern void millis_count(void);

//ISR(millis_count) {	
//	millis_counter ++;
//}

#define INTR_TIMR_D (1 << 4)

volatile uint32_t millis() {
	return millis_counter * 1;
}

void millis_start() {
	__vectors.user[MFP_INT + MFP_TIMERD - USER_ISR_START] = &millis_count;
	
	VR = MFP_INT;
	TCDCR &= 0xF0; // disable timer D
	
	//TDDR = 184;	 // ~100 hz
	//TCDCR |= 0x7;  // set prescaler of 200
	
	TDDR = 230;      // ~1000 hz
	TCDCR |= 0x3;    // set prescaler of 50
	
	IERB |= INTR_TIMR_D;
	IMRB |= INTR_TIMR_D;
	
	sei();	
}

void millis_stop() {
	IMRB &= ~INTR_TIMR_D; // mask the interrupt
}
