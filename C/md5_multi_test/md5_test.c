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

char * hexchar = "0123456789ABCDEF";

void putHexChar(uint8_t ch) {
	putc(hexchar[ch >> 4]);
	putc(hexchar[ch & 0xF]);
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

#include <md5.h>

#define MD5_START 0x10000
#define MD5_END 0x60000

void task_md5() {
	printf("MD5 TASK (ID %d) started.\n",CURRENT_TASK_ID);
	
	md5_state_t state;
	uint8_t digest[16];
	
	while (true) {
		md5_init(&state);
        
		for (uint32_t i = MD5_START; i < MD5_END; i += 64)
            md5_append(&state, (uint8_t*)i, 64);
	
		md5_finish(&state, digest);
	
		yield();
		putHexChar(CURRENT_TASK_ID);
		putc(' ');
		
		TIL311 = digest[0];
		
		for (uint8_t i = 0; i < 16; i++) 
			putHexChar(digest[i]);
		
		putc('\n');
		
		yield();
	}
}

uint32_t hash( uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

int main() {
 	TIL311 = 0x98;

	lcd_init();
	serial_start(SERIAL_SAFE);
	
	puts("Hello! Starting tasks.\n");

	create_task(&task_time,0);
  	
  	puts("Filling memory\n");
  	
  	uint32_t ha = 124912341;
        
	TIL311 = 0x1A;
	
	for (uint32_t i = MD5_START; i < MD5_END; i += 1) {
		*((uint8_t*)(i)) = ha&0xFF;
		ha = hash(ha + i);
	}
	
	for (int i = 0; i < 29; i++)
		create_task(&task_md5,0);
	
	yield();
  	puts("Tasks started, main() returning.\n");	

  	return 0;
}
