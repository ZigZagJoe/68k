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

extern void syscall_trap();

#define SYSCALL(ret, name, ...)   \
    ret name (__VA_ARGS__); \
    ret _ ## name (__VA_ARGS__); \
    __asm( #name ": \nmove.l #_"  # name ", %a1\ntrap #14\nrts\n"); \
    ret _ ## name (__VA_ARGS__)  

SYSCALL(uint8_t *, test_syscall, int arg1) {

    sei();
    printf("hello world %d\n", arg1);
}


//#define test_syscall(...)  syscall_helper(&_test_syscall, __VA_ARGS__)

//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;
    DDR |= 2;
    
    __vectors.trap[14] = &syscall_trap;
    
    serial_start(SERIAL_SAFE);
    sei();
    
    // become user
    __asm volatile("andi #0b1101111111111111, %SR\n");
    
    
    int i = 0;
    while(true) {
        bset_a(GPDR, 1);
        test_syscall(i++);
       //  syscall_helper(1,2,3);
        bclr_a(GPDR, 1);
    }

    /*DDR |= 2;
    while(true) {
        bset_a(GPDR, 1);
        DELAY_US(40);
        bclr_a(GPDR, 1);
        DELAY_US(40);
    }*/
    
}

void enter_critical() {}
void leave_critical() {}
