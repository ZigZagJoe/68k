#include <stdint.h>
#include <io.h>
#include <stdlib.h>

void _delay_ms(uint16_t x) {
	__asm volatile("move.l %0,%%d0\n" \
		"1: sub.l #1, %%d0\n" \
		"jne 1b\n" \
		::"g"(x*MILLISECOND_DELAY):"d0");
}

void beep_stop() {
	TACR = B10000; // reset output and stop timer
}

void beep_start(uint16_t freq) {
	uint8_t psc = 4;
	uint8_t val = 0x1;
	
	if (freq < 3650 && freq >= 1450) {
		psc = 10;
		val = 0x2;
	} else if (freq < 1450 && freq >= 950) {
		psc = 16;
		val = 0x3;
	} else if (freq < 950 && freq >= 300) {
		psc = 50;
		val = 0x4;
	} else if (freq < 300) {
		psc = 100;
		val = 0x6;
	} 
	
	uint8_t del = (4000000 / psc) / freq;
	
	 // there's no point using the /200 prescaler to hit values below this, they sound like shit. so, prevent an overflow if they are presented.
	if (freq < 150)
		del = 0xFF;

	TIL311 = del;
	
	TADR = del;  // calculated delay
	TACR = val;  // prescaler
}

void beep(uint16_t freq, uint16_t ms) {
	beep_start(freq);
	_delay_ms(ms);
	beep_stop();
}
