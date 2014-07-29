#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>
#include <kernel.h>

void mem_dump(uint8_t *addr, uint32_t cnt) {
    int c = 0;
    char ascii[17];
    ascii[16] = 0;
    
    putc('\n');
    
    printf("%8X   ", addr);
    for (uint32_t i = 0; i < cnt; i++) {
        uint8_t b = *addr;
        
        ascii[c] = (b > 31 & b < 127) ? b : '.';
        
        printf("%02X ",b);
        
        addr++;
        
        if (++c == 16) {
            puts("  ");
            puts(ascii);
            putc('\n');
            if ((i+1) < cnt)
                printf("%8X   ", addr);
            c = 0;
        }
    }
    
    if (c < 15) putc('\n');
}

#include <md5.h>

uint32_t hash( uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

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

//#include "lzfx.h"
/*
void memset(uint8_t *dest, uint8_t val, uint32_t count) {
    while (--count)
        *dest = val;
}
*/
/*void lzfx_compress(int a,int b, int c, int d) {}


*/

#include "test_bin.h"
//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    //millis_start();
    sei();
    
    create_task(&test_task, 3, 0x8BADC0DE, 0xDEADBEEF, 0x12345678);	
	create_task(&task_time,0);
    create_task(&task_echo,0);
    create_task(&task_quiet,0);
  	create_task(&task_random,0);
  	create_task(&task_song,0);
    
    uint32_t out_size = 30000;
    uint32_t start, end;
    int ret_code;
    
    uint8_t * dest = 0x40000;
    
    start = millis();
    for (uint8_t i = 0; i < 8; i ++)
        ret_code = lzf_decompress(out_bin, out_bin_len, dest, out_size);
    end = millis();
    
    printf("Complete in %d ms\n", end-start);
    
    printf("ret_code: %d\n", ret_code);
    
    if (ret_code) {
        uint32_t crc = CRC_INITIAL;
        for (int i = 0; i < ret_code; i++)
            crc = qcrc_update(crc, dest[i]);
      
        printf("QCRC: %08X\n",crc);  
    }
    
    md5_state_t state;
	uint8_t digest[16];
	
	while (true) {
		//puts("Fill memory\n");

		md5_init(&state);
	
		uint32_t ha = 124912341;
        uint8_t tmp;
        
        TIL311 = 0x1A;
        
		for (uint32_t i = 0x6000; i < 0x78000; i += 1) {
			*((uint8_t*)(i)) = ha&0xFF;
			ha = hash(ha + i);
		}
		
		//puts("Do hash\n");
	
		for (uint32_t i = 0x6000; i < 0x78000; i += 64) {
            TIL311 = (uint8_t*)i;
			md5_append(&state, (uint8_t*)i, 64);
		}
	
		md5_finish(&state, digest);
   
	   // printf("Done\n");

		char *hex_chars = "0123456789ABCDEF";
	
		for (uint8_t i = 0; i < 16; i++) {
			putc(hex_chars[digest[i]>>4]);
			putc(hex_chars[digest[i]&0xF]);
		}
	
		putc('\n');
	}
memset(0x4000,0,0x30000);
      lzfx_compress(0,100,0,100);
//    lzfx_decompress(0,100,0,100);
    
    
   serial_start(SERIAL_SAFE);
    millis_start();
    sei();
    
    memset(0x4000,0,0x30000);
    memset(0x34000,0,0x30000);
    memcpy(1,2,3);
    switch (getc()) {
        case 1:
          __asm volatile ("nop\n");
        case 2:
          __asm volatile ("nop\n");
        case 3:
          __asm volatile ("nop\n");
        case 4:
          __asm volatile ("nop\n");
        case 5:
              __asm volatile ("nop\n");
        case 6:
            __asm volatile ("nop\n");
            break;
    }
    
  
    mem_dump(0x4000, 2500);
    
  //  uint32_t i = 0;
 //   DDR = 2;
  //  GPDR = 0;
    
    /*while(true) {
    	if (serial_available()) 
    		putc_sync(getc());*
    	//TIL311 = 0xAA;
   
       // for (char ch = 'a'; ch <= 'z'; ch++)
        //	putc(ch);
       /* puts("Hello world.\n");
        DELAY_MS(1000);
        continue;
        printf("%d\n",i);
       // TIL311 = 0xBB;
   		i++;
        putc('A');
        putc('\n');*/
        
       /* for (uint8_t i = 'a'; i <= 'z'; i++) 
        	putc(i);
        putc('\n');*/
        
        //TIL311 = 0xCC;
   
   // }
    
    
        /* TIL311 = 0xCC;
        DELAY_MS(1000);

        TIL311 = 0xC0;
        DELAY_MS(1000);
        
        TIL311 = 0xDE;
        DELAY_MS(1000);
        
        TIL311 = 0x00;
        DELAY_MS(1000);*/
        
}
