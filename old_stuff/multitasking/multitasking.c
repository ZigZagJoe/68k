#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <printf.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <flash.h>
#include <lcd.h>
#include <time.h>
#include <kernel.h>

void putc_lcd(void*p, char ch) {
	lcd_data(ch);
}

void lcd_printf(char *fmt, ...)
{
    va_list va;
    va_start(va,fmt);
    tfp_format(0,&putc_lcd,fmt,va);
    va_end(va);
}

void play_rtttl(char *p);

char * hexchar = "0123456789ABCDEF";

void putHexChar(uint8_t ch) {
	putc(hexchar[ch >> 4]);
	putc(hexchar[ch & 0xF]);
}

int main();

uint32_t x = (uint32_t)__DATE__;
uint32_t y = (uint32_t)__TIME__;
uint32_t z = (uint32_t)&main;
uint32_t w = (uint32_t)__LINE__;
 
uint32_t rand(void) {
    uint32_t t;
 
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) * 3120909123 ^ t ^ (t >> 8);
}

/*char * songs[];
#define songcount 24*/

void task_song() {
	printf("SONG TASK (ID %d) started.\n", CURRENT_TASK_ID);
	yield();
	while(true) {
		/*for (int i = 0; i < songcount; i++) {
			play_rtttl(songs[i]);
			sleep_for(1000);
		}*/
		
		play_rtttl("korobyeyniki:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p,d6,8f6,a6,8g6,8f6,e6,8e6,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,a");
		sleep_for(5000);
	}
}

void task_echo() {
	printf("ECHO TASK (ID %d) started.\n", CURRENT_TASK_ID);
	yield();
	while(true) {
		while (!serial_available())
			yield();
		putc(getc());
	}
}

volatile uint8_t exit = 0;

void task_quiet() {
	printf("QUIET TASK (ID %d) started.\n", CURRENT_TASK_ID);
	yield();
	puts("A wild QUIET TASK has appeared.\n");
	
	while(!exit) {
		yield();
	}
	
	puts("It's super effective! QUIET TASK has fainted!\n");
}


void task_time() {
	printf("TIME TASK (ID %d) started.\n",CURRENT_TASK_ID);
	yield();
	while(true) {
		lcd_cursor(0,0);
        lcd_printf("Runtime: %d.%d    ",millis()/1000, (millis()%1000)/100);
 		sleep_for(100);
    }
}



void task_a_random() {
	puts("Go, random task!\n");
	if (exit == 0) {
		yield();
		puts("Random task uses HYPER BEAM.\n");
		exit = 1;
	} else {
		puts("Random task loafs around.\n");
		sleep_for(1000);
		if (((rand() % 4) == 2)) {
			exit = 0;
			create_task(&task_quiet);
		} else
			puts("Come back, random task!\n");
			
	}
}

void task_random() {
	printf("RAND TASK (ID %d) started.\n",CURRENT_TASK_ID);
	yield();

	while(true) {
		lcd_cursor(0,1);
		if (((rand() % 64) == 27)) 
			create_task(&task_a_random);
			
		lcd_printf("%x%x",rand(),rand());
		sleep_for(200);
	}
}

void crashy_task() {
	puts("Crash-o-clock!\n");
	
	MEM(IO_DEV5) = 1;
	
}


int main() {
 	TIL311 = 0x98;

	lcd_init();
	serial_start(SERIAL_SAFE);
	
	puts("Hello! Starting tasks.\n");

    create_task(&task_echo);
    create_task(&task_song);
    create_task(&task_quiet);
  	create_task(&task_time);
  	create_task(&task_random);
  	//create_task(&crashy_task);
  	yield(); // allow other tasks to run
  	
  	puts("Tasks started, main() returning.\n");	
  	
  	return 0;
}