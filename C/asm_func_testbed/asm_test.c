#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <printf.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>

void simp_printf(char * str, ...);

uint32_t do_test1();
uint32_t do_test2();


int main() {
    TIL311 = 0x01;

	serial_start(SERIAL_SAFE);
    millis_start();
    sei();
    
    //srand();
    
    
    uint32_t t = do_test1();
    printf("Cpy 1: %d\n", t * 10);
    
    
    t = do_test2();
    printf("Cpy 2: %d\n", t * 10);
    
    while(true);
    
   /*
   char * str2 = "embedded str";
	simp_printf("hello world\nhex= 0x%x\nstring= %s\nbinary= %b\npercent %%\nchar= '%c'\nu_dec= '%u'\nu_dec_oor= '%u'\n", 0xDEADBEEF, str2, 0xCAFEBABE, 'h',132874,655369);
	
	simp_printf("s_dec= '%d'\n-s_dec= '%d'\ns_dec_oor= '%d'\n-s_dec_oor= '%d'\n", 21345,-10982,695369,-655600);
	*/
	
	while(true)
	    printf("%08x\n",rand());
	
 	
}
