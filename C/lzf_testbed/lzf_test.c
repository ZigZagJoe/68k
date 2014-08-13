#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>


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

//#include "lzfx.h"
/*
void memset(uint8_t *dest, uint8_t val, uint32_t count) {
    while (--count)
        *dest = val;
}
*/
/*void lzfx_compress(int a,int b, int c, int d) {}


*/

#define ROL(num, bits) (((num) << (bits)) | ((num) >> (32 - (bits))))

uint32_t crc_update (uint32_t inp, uint8_t v) {
	return ROL(inp ^ v, 1);
}

uint32_t compute_crc(uint8_t* addr, uint32_t count);

#include "test_bin.h"
//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    millis_start();
    sei();
    
    uint32_t start, end;
    int ret;
    uint32_t crc;
    
   // out_bin_len = 16;
    
    uint8_t * dest = 0x40000;
    
    //printf("CRC_INITIAL: %08X\n\nlzf test\n",CRC_INITIAL); 
    
    start = millis();
    for (uint8_t i = 0; i < 8; i ++)
        ret = lzf_inflate(out_bin, out_bin_len, dest);
    end = millis();
    
    printf("Complete in %d ms\n", end-start);
    
    printf("ret_code: %d\n", ret);
    
    uint32_t size = ret;
   
    if (ret) {
        /*crc = CRC_INITIAL;
        for (int i = 0; i < size; i++) {
            crc = qcrc_update(crc, dest[i]);
        }*/
        crc = compute_crc(dest, size);
      
        printf("ASM QCRC: %08X\nExpect: 1C31D49B\n",crc);  
        
       /* crc = CRC_INITIAL;
        for (int i = 0; i < size; i++) {
            crc = crc_update(crc, dest[i]);
            //printf("%d: %08X\n",i,crc);
        }
      
        printf("C QCRC: %08X\n",crc);  */
    }
    
    /*
    printf("\nlzf-crc test\n");
    start = millis();
    for (uint8_t i = 0; i < 8; i ++)
        ret = lzf_inflate_crc(out_bin, out_bin_len, dest, &crc);
    end = millis();
    
    printf("Complete in %d ms\n", end-start);
    
    printf("ret_code: %d\n", ret);
    printf("CRC: %08X\n", crc);
    //mem_dump(dest, 64);
    ///////////////////////////////////////////
    
    printf("\nASM CRC test of %d bytes:\n", size);
    start = millis();
    for (uint8_t i = 0; i < 8; i ++)
        crc = compute_crc(dest, size);
    end = millis();
    
    printf("Complete in %d ms\n", end-start);
    
    
    printf("CRC: %08X\n", crc);
   
    ///////////////////////////////////////////
    printf("\nASM-C CRC test of %d bytes:\n", size);
    start = millis();
    for (uint8_t i = 0; i < 8; i ++) {
        crc = CRC_INITIAL;
        for (int i = 0; i < size; i++)
            crc = qcrc_update(crc, dest[i]);
    }
    end = millis();
    
    printf("Complete in %d ms\n", end-start);
    
    printf("CRC: %08X\n", crc);
    
    ///////////////////////////////////////////
    printf("\nC CRC test of %d bytes:\n", size);
    start = millis();
    for (uint8_t i = 0; i < 8; i ++) {
        crc = CRC_INITIAL;
        for (int i = 0; i < size; i++)
            crc = crc_update(crc, dest[i]);
    }
    end = millis();
    
    printf("Complete in %d ms\n", end-start);
    
    printf("CRC: %08X\n", crc);
    
    uint32_t compressed_sz = out_bin_len;
    
    uint32_t load_addr = (0x80000 - 4096 - compressed_sz) & ~1;
    uint32_t dec_addr = (0x80000 - 4096 - size) & ~1 - 8096; // 8096: max range of backref
    
    uint8_t * ld_ptr = load_addr;
    uint8_t * dec_ptr = dec_addr;
    
    printf("\nLoad compressed to %08X in memory\n",load_addr);
    memcpy(ld_ptr,out_bin,compressed_sz);

    printf("Decompress to %08X in memory\n",dec_addr);
    
    ret = lzf_inflate(ld_ptr, compressed_sz, dec_ptr);
    
    printf("Return: %d\n", ret);

    crc = CRC_INITIAL;
    for (int i  = 0; i < ret; i++)
        crc = qcrc_update(crc, dec_ptr[i]);
      
    printf("CRC: %08X\n",crc); 
    //memset(0x4000,0,0x30000);
   //   lzfx_compress(0,100,0,100);
//    lzfx_decompress(0,100,0,100);
    
    
  /*  serial_start(SERIAL_SAFE);
    millis_start();
    sei();
    
    memset(0x4000,0,0x30000);
    memset(0x34000,0,0x30000);
    
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
    }*/
    
  
    //mem_dump(0x4000, 2500);
    
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
