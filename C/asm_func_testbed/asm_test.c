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


uint16_t _strlen(char* str);

int main() {
    TIL311 = 0x01;

	serial_start(SERIAL_SAFE);
    millis_start();
    sei();
    char * test_str = "hello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lolhello world what what in the butt lol";
    
    uint32_t start, end;
    int ret;
    
    start = millis();
    for (int i = 0; i < 100; i++)
        ret = strlen(test_str);
        
    end = millis();
        
    printf("Str len: %d\n",ret);
     printf("Time: %d\n", end-start);

    start = millis();
    for (int i = 0; i < 100; i++)
        ret = _strlen(test_str);
    end = millis();
    
    printf("Str len2: %d\n", ret);
    printf("Time: %d\n", end-start);
    
    
    printf("Null test\n");
    start = millis();
    for (int i = 0; i < 100; i++)
        ret = _nulltest(test_str);
    end = millis();
    
    printf("Time: %d\n", end-start);
     
    return_to_loader();
    //srand();
    
    
  /*  uint32_t t = do_test1();
    printf("Cpy 1: %d\n", t * 10);
    
    
    t = do_test2();
    printf("Cpy 2: %d\n", t * 10);*/
}
