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

int main() {
    TIL311 = 0x01;

	default_interrupts();
    serial_start(SERIAL_SAFE);
    sei();
    
    char * str2 = "embedded str";
   
	simp_printf("hello world\nhex= 0x%x\nstring= %s\nbinary= %b\npercent %%\nchar= '%c'\n", 0xDEADBEEF, str2, 0xCAFEBABE, 'h');
	
 	while(true);
}
