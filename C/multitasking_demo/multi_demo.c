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
#include <beep.h>
#include <semaphore.h>

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

/*char * songs[];
#define songcount 24*/

/*for (int i = 0; i < songcount; i++) {
			play_rtttl(songs[i]);
			sleep_for(1000);
		}*/

void task_song() {
	printf("SONG TASK (ID %d) started.\n", CURRENT_TASK_ID);
	yield();
	while(true) {
		
		play_rtttl("korobyeyniki:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p,d6,8f6,a6,8g6,8f6,e6,8e6,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,a");
		beep_stop();
		
		sleep_for(5000);
	}
}


void task_echo() {
	printf("ECHO TASK (ID %d) started.\n", CURRENT_TASK_ID);

	yield();
	while(true) {
		while (!serial_available()) 
			 yield();
		
		while (serial_available())
			putc(getc());
			
	}
}

volatile uint8_t kill = 0;

void task_quiet() {
	printf("QUIET TASK (ID %d) started.\n", CURRENT_TASK_ID);
	yield();
	puts("A wild QUIET TASK has appeared.\n");
	
	while(!kill)
		yield();
	
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
	printf("Go, random task! (%d)\n",CURRENT_TASK_ID);
	yield();
	if (kill == 0) {
		yield();
		puts("Random task uses HYPER BEAM.\n");
		kill = 1;
	} else {
		puts("Random task loafs around.\n");
		sleep_for(1000);
		if (((rand() % 4) == 2)) {
			kill = 0;
			create_task(&task_quiet,0);
		} else
			puts("Come back, random task!\n");
			
	}
}

void task_random() {
	printf("RAND TASK (ID %d) started.\n",CURRENT_TASK_ID);
	yield();

	while(true) {
		lcd_cursor(0,1);
		if (((rand() % 128) == 27)) 
			create_task(&task_a_random,0);
			
		lcd_printf("%x%x",rand(),rand());
		sleep_for(200);
	}
}

////////////


void breeder_task() {
	yield();
	printf("Task %x\n",CURRENT_TASK_ID);
	create_task(&breeder_task,0);
}

void test_task(int arg1, int arg2, int arg3) {
	printf("Test task: %x %x %x\n", arg1, arg2, arg3);
}

uint32_t var = 0;
sem_t semaphore_1;

void task_sem_test1() {
    while(true) {
        sem_acquire(&semaphore_1);
        var++;
        sleep_for(1000);
        sem_release(&semaphore_1);
        yield();
    }
}

void task_sem_test2() {
    while(true) {
        sem_acquire(&semaphore_1);
        printf("%d\n",var);
        sem_release(&semaphore_1);
        yield();
    }
}

int main() {
 	TIL311 = 0x98;
 	
 	sem_init(&semaphore_1);

	lcd_init();
	serial_start(SERIAL_SAFE);
	
	puts("Hello! Starting tasks.\n");

	create_task(&test_task, 3, 0x8BADC0DE, 0xDEADBEEF, 0x12345678);	
	create_task(&task_time,0);
    create_task(&task_echo,0);
    create_task(&task_quiet,0);
  	//create_task(&task_random,0);
  	create_task(&task_song,0);
  	
  	create_task(&task_sem_test1,0);
    create_task(&task_sem_test2,0);
  	
  /*	for (int i = 0; i < 16; i++)
  		create_task(&breeder_task,0);*/
  	   
  	puts("Tasks started, main() returning.\n");	
	
  	return 0;
}
