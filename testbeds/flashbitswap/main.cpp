#include <stdio.h>
#include <stdlib.h>
#include "strapper.h"

uint8_t swap_bits(uint8_t arg) {
    return  ((arg & 1) << 7) + 
            ((arg & 2) << 5) + 
            ((arg & 4) << 3) + 
            ((arg & 8) << 1) + 
            ((arg & 16) >> 1) +
            ((arg & 32) >> 3) + 
            ((arg & 64) >> 5) +  
            ((arg & 128) >> 7); 
}

int main(int argc, char** argv) {

    for (int i = 0; i< strapper_len; i++) {
        if (i % 16 == 0) printf("\n");
                
        printf("%02X ",swap_bits(strapper[i]));

    }
    
    printf("\n");
    return 0;
}
