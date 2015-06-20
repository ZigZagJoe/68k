#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <kernel.h>
#include <md5.h>

char * hexchar = "0123456789ABCDEF";

void putHexChar(uint8_t ch) {
	putc(hexchar[ch >> 4]);
	putc(hexchar[ch & 0xF]);
}

uint32_t t_a, t_b;

void task_time() {
	printf("TIME TASK (ID %d) started.\n",CURRENT_TASK_ID);
	yield();
	while(true) {
		uint32_t t = millis();
		t_a = t/1000;
		t_b = (t%1000)/100;
		TIL311 = (((uint8_t) t_a) << 4) | (t_b&0xF);
		lcd_cursor(0,0);
        lcd_printf("Runtime: %d.%d    ",t_a, t_b);
    }
}

// test 320k of ram
#define MD5_START 0x10000
#define MD5_END 0x60000

int failed = 0;
uint8_t PRINT_ID = 2;
uint8_t expect[] = {0x2A,0xB4,0xD6,0x61, 0xCC,0x16,0xC5,0xC5, 0x60,0x6F,0xBC,0x3A, 0xC5,0x61,0x15,0x3D};

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

		uint8_t pass = 1;
		for (uint8_t i = 0; i < 16; i++) 
		    if (digest[i] != expect[i]) pass = false;
			

		enter_critical();	
		printf("TASK %d ",CURRENT_TASK_ID);
		
        if (!pass) {
            failed++;	
		    puts("FAILED (");

            for (uint8_t i = 0; i < 16; i++) 
                putHexChar(digest[i]);
            
            putc(')');
            
            lcd_clear();
            lcd_cursor(0,1);
            lcd_printf("FC%d %d.%ds",failed ,t_a, t_b);
		} else 
		    puts("passed");
		    
		printf(" at %d.%ds\n",t_a, t_b);
        	
		if (CURRENT_TASK_ID == PRINT_ID && !failed) {
			lcd_cursor(0,1);
			
			for (uint8_t p = 0; p < 6; p++) 
				lcd_printf("%02X",digest[p]);
			
			lcd_printf(" #%02X",PRINT_ID);
            
			if (++PRINT_ID > 30) 
				PRINT_ID = 2;	
		}
			
		leave_critical();	
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
	
	lcd_printf("# MD5 SELFTEST #");
	lcd_cursor(0,1);
	lcd_printf("Initializing....");
	
  	puts("Initializing memory...\n");
  	
  	uint32_t ha = 124912341;
        
	TIL311 = 0x1A;
	
	for (uint32_t i = MD5_START; i < MD5_END; i += 1) {
		*((uint8_t*)(i)) = ha&0xFF;
		ha = hash(ha + i);
	}
	
	puts("Starting tasks.\n");
    puts("EXPECT: 2AB4D661CC16C5C5606FBC3AC561153D\n");
    
  	enter_critical();   // prevent the tasks from being run until they are all created
	create_task(&task_time,0);
  	
	for (int i = 0; i < 29; i++)
		create_task(&task_md5,0);
	
	lcd_cursor(0,1);
	lcd_printf("MD5 in progress.");
	
	puts("MD5 tasks started, main() returning.\n");	
	
	leave_critical();
  	
  	return 0;
}
