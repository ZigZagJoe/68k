#include <time.h>
#include <interrupts.h>
#include <io.h>

volatile uint32_t millis_counter = 0;

ISR(millis_count) {	
	millis_counter ++;
}

#define INTR_TIMR_D (1 << 4)

volatile uint32_t millis() {
	return millis_counter * 10;
}

void millis_start() {
	__vectors.user[MFP_INT + MFP_TIMERD - USER_ISR_START] = &millis_count;
	
	TDDR = 184;
	TCDCR |= 0x7; // prescaler of 200 for timer D
	IERB |= INTR_TIMR_D;
	IMRB |= INTR_TIMR_D;
	sei();	
}

void millis_stop() {
	IMRB &= ~INTR_TIMR_D; // mask it
}
