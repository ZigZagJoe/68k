#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <printf.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>
#include <kernel.h>
#include <string.h>
#include <semaphore.h>

int main();

sem_t lcd_sem;

extern volatile uint8_t *wav_ptr;
extern volatile uint8_t *wav_end;
extern void wav_isr();

#define INTR_TIMR_B (1 << 0)
#define INTR_GPI2 (1 << 2)

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

void task_time() {
	printf("TIME TASK (ID %d) started.\n",CURRENT_TASK_ID);
	yield();
	while(true) {
	    sem_acquire(&lcd_sem);
		lcd_cursor(0,0);
        lcd_printf("Runtime: %d.%d    ",millis()/1000, (millis()%1000)/100);
        sem_release(&lcd_sem);
 		sleep_for(100);
    }
}

void task_scroller() {
	printf("SCROLLER TASK (ID %d) started.\n",CURRENT_TASK_ID);
	yield();

    uint8_t mode = 0;
	while(true) {
	    mode = !mode;
	    for(uint8_t i = 0; i < 16; i++) {
		    sem_acquire(&lcd_sem);
		    lcd_cursor(i,1);
            lcd_data(mode?0xFF:' ');
		    sem_release(&lcd_sem);
 		    sleep_for(20);
		}
	}
}

////////////

#define song_start 0x84000
#define song_len   245449

void task_wav_play() {
    printf("WAV TASK (ID %d) started.\n", CURRENT_TASK_ID);
	__vectors.user[MFP_INT + MFP_GPI2 - USER_ISR_START] = &wav_isr;

    TBDR = 29;   // approx 15.9 khz out
	TBCR = 0x1;  // prescaler of 4
       
    // wait for the fifo to clear of garbage
    DELAY_MS(50);
    
    wav_ptr = song_start;
    wav_end = wav_ptr + song_len - 1;
    
	AER |= (1 << 2);
	IERB |= INTR_GPI2;
	IMRB |= INTR_GPI2;
	    
    // prime the fifo
    memcpy(IO_DEV3, wav_ptr, 512);
    wav_ptr += 512;
       
	printf("Wav Playback started.\n");
		 
    // wait for end of clip & loop.
    while(1) {
        if (wav_ptr >= wav_end) {
            wav_ptr = song_start;
            printf("Wav looping!\n");
        } else
            TIL311 = ((int)(wav_ptr - song_start) * (song_len/16000) / song_len);
        
        yield();
    }
}

int main() {
 	TIL311 = 0x98;
 	
 	srand();
 	
 	sem_init(&lcd_sem);
	
	lcd_init();
	serial_start(SERIAL_SAFE);
	
	puts("Hello! Starting tasks.\n");
    
    enter_critical();
    
    create_task(&task_time,0);
    create_task(&task_echo,0);
  	create_task(&task_scroller,0);
    create_task(&task_wav_play,0);
    
  /*	for (int i = 0; i < 16; i++)
  		create_task(&breeder_task,0);*/
  		
  	leave_critical();
  	yield();  
  	
  	puts("Tasks started, main() returning.\n");	
  	
	return 0;
}
