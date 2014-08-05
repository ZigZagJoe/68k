#include <stdio.h>
#include <stdlib.h>

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
            printf("  %s\n",ascii);
            if ((i+1) < cnt)
                printf("%8X   ", addr);
            c = 0;
        }
    }
    
    if (c && c < 15) {
        ascii[c] = 0;
        while (c++ < 16)
            printf("   ");
        printf("  %s\n",ascii);
    } else putc('\n');
}

void return_to_loader() {
    putc('\n');
    while (tx_busy());  // wait for any pending characters to be sent
    DELAY(100);         // wait for last character to be sent (otherwise MFP will be reset and char not sent)
    __asm ("move.w #0xB007, 0x400\n" /* cause loader to not boot from flash */
            "jmp 0x80008\n"          /* jump to loader */
    );
}
