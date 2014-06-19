#include <stdint.h>
#include <io.h>
#include <stdlib.h>

void _delay_ms(uint16_t x) {
	__asm volatile("move.l %0,%%d0\n" \
		"1: sub.l #1, %%d0\n" \
		"jne 1b\n" \
		::"g"(x*MILLISECOND_DELAY):"d0");
}

void beep(uint16_t freq, uint16_t ms) {
	uint8_t del = (3686400 / 50) / freq;
	TIL311 = del;
	
	TADR = del;  // calculated delay
	TACR = 0x4; // /50 prescaler
	
	_delay_ms(ms);
	TACR = 0;
	TADR = 0;
}

#define TEMPO_UP 1.6F
#define TEMPO_DOWN 0.6F

void mario() {
	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(510, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(770, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*550);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*575);

	beep(510, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*450);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*400);
	beep(320, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*500);
	beep(440, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(480, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*330);
	beep(450, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*200);
	beep(660, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*200);
	beep(760, TEMPO_UP*50);
	_delay_ms(TEMPO_DOWN*150);
	beep(860, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(700, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(760, TEMPO_UP*50);
	_delay_ms(TEMPO_DOWN*350);
	beep(660, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(520, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(580, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(480, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*500);

	beep(510, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*450);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*400);
	beep(320, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*500);
	beep(440, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(480, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*330);
	beep(450, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*200);
	beep(660, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*200);
	beep(760, TEMPO_UP*50);
	_delay_ms(TEMPO_DOWN*150);
	beep(860, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(700, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(760, TEMPO_UP*50);
	_delay_ms(TEMPO_DOWN*350);
	beep(660, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(520, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(580, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(480, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*500);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(760, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(720, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(680, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(620, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);

	beep(650, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(570, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*220);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(760, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(720, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(680, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(620, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);

	beep(650, TEMPO_UP*200);
	_delay_ms(TEMPO_DOWN*300);

	beep(1020, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(1020, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(1020, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);

	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(760, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(720, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(680, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(620, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);

	beep(650, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(570, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*420);

	beep(585, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*450);

	beep(550, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*420);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*360);

	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(760, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(720, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(680, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(620, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);

	beep(650, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(570, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*220);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(760, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(720, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(680, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(620, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);

	beep(650, TEMPO_UP*200);
	_delay_ms(TEMPO_DOWN*300);

	beep(1020, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(1020, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(1020, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);

	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(760, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(720, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(680, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(620, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);

	beep(650, TEMPO_UP*150);
	_delay_ms(TEMPO_DOWN*300);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(430, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(570, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*420);

	beep(585, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*450);

	beep(550, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*420);

	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*360);

	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);

	beep(500, TEMPO_UP*60);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*60);
	_delay_ms(TEMPO_DOWN*350);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(580, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*350);
	beep(660, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(430, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(380, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*600);

	beep(500, TEMPO_UP*60);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*60);
	_delay_ms(TEMPO_DOWN*350);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(580, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(660, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*550);

	beep(870, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*325);
	beep(760, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*600);

	beep(500, TEMPO_UP*60);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(500, TEMPO_UP*60);
	_delay_ms(TEMPO_DOWN*350);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(580, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*350);
	beep(660, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(500, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*300);
	beep(430, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*150);
	beep(380, TEMPO_UP*80);
	_delay_ms(TEMPO_DOWN*600);

	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*150);
	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(510, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*100);
	beep(660, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*300);
	beep(770, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*550);
	beep(380, TEMPO_UP*100);
	_delay_ms(TEMPO_DOWN*575);
}
