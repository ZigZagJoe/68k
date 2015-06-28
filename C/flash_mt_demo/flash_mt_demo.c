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
		lcd_cursor(0,0);
        lcd_printf("Runtime: %d.%d    ",millis()/1000, (millis()%1000)/100);
        TIL311 = millis()/1000;
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
		    lcd_cursor(i,1);
            lcd_data(mode?0xFF:' ');
		    sleep_for(20);
		}
	}
}

////////////

int main() {
 	TIL311 = 0x98;

	lcd_init();
	serial_start(SERIAL_SAFE);
	
	puts("Hello! Starting tasks.\n");

	create_task(&task_time,0);
    create_task(&task_echo,0);
    create_task(&task_scroller,0);
  	
  	puts("Tasks started, main() returning.\n");	
	
  	return 0;
}
