#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "data.h"
#include <time.h>

#define ROL(num, bits) (((num) << (bits)) | ((num) >> (32 - (bits))))

uint8_t failures[20];


uint32_t crc_update (uint32_t inp, uint8_t v) {
	return ROL(inp ^ v, 1);
}

/*
uint16_t crc_update(uint16_t crc, uint8_t a) {
    int i;
    
    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }
    
    return crc;
}*/

uint32_t base;
uint16_t fail_ind = 0;

uint8_t check_diff(uint32_t crc) {
	uint8_t diffbits = 0;
	
	for (uint8_t i = 0; i < 31; i++) {
		if (((crc >> i) & 0x1) != ((base >> i) & 0x1))
			diffbits++;
	}
	
	if (diffbits == 0) {
		//printf("WARNING NO DIFFERENCE\n");
		failures[fail_ind] ++;
	}
	
	return diffbits;
}

#define TEST(name) { /*//printf("%2d: %s\t",fail_ind, name);*/ for (int x = 0; x < data_len; x++) { test_arr[x] = data[x]; }; crc = 0xDEADC0DE; }

#define END_TEST() {d = check_diff(crc); diff_s += d; diff_t += 32; /*//printf("0x%X\t%2d/32\n",crc, d);*/ fail_ind++; }

void crc_test() {

    
    uint32_t crc;
    uint8_t a, b;
   	uint8_t test_arr[data_len];
   	uint8_t d;
   	uint32_t diff_s = 0, diff_t = 0;
    uint16_t r,r1,r2;
    TEST("base CRC\t");
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
 	//printf("\t0x%X\n", crc);
    base = crc;
    
    ///////////////////////////////
    fail_ind=0;
    TEST("skip 10 in middle");
    r = rand() % (data_len-20);
     //printf("%d\t",r);
    for (int i = 0; i < data_len; i++) {
    	if (i > r && i < (r+10)) continue;
    	crc = crc_update(crc, test_arr[i]);
    }
    END_TEST();
   
    TEST("skip 1 in middle");
    r = rand() % (data_len/3) + (data_len/3);
    //printf("%d\t",r);
    for (int i = 0; i < data_len; i++) {
    	if (i == r) continue;
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    ///////////////////////////////
    TEST("skip 1 at end    \t");
    for (int i = 0; i < data_len-1; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    ///////////////////////////////
    TEST("skip 10 at end    \t");
    for (int i = 0; i < data_len-10; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
      END_TEST();
   
    
    ///////////////////////////////
    TEST("skip 1 at start   \t");
    for (int i = 1; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    ///////////////////////////////
    TEST("skip every 1000th byte");
    for (int i = 1; i < data_len; i++) {
    	if (i %1000 == 0) continue;
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    
     ///////////////////////////////
    TEST("skip 10 at start   \t");
    for (int i = 10; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST()
    
    ///////////////////////////////
    TEST("swap two bytes  ");
    
    while(true) {
		r1 = rand() % (data_len);
		r2 = rand() % (data_len);
		if (r1 == r2 || (test_arr[r1] == test_arr[r2])) 
			continue;

		a = test_arr[r1];
		b = test_arr[r2];
		test_arr[r1] = b;
		test_arr[r2] = a;

		//printf("%d %d\t",r1,r2);
		break;
  	}
    
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
    
    ///////////////////////////////
    TEST("zero a middle byte");
    
    while(true) {
  	   r = rand() % (data_len/3) + (data_len/3);
    	if (test_arr[r] == 0) continue;
    	test_arr[r] = 0;
    	
    	//printf("%d\t",r);
    	break;
	}
    
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    
    ///////////////////////////////
    TEST("zero an early byte");
    while(true) {
  	   r = rand() % (data_len/3);
    	if (test_arr[r] == 0) continue;
    	test_arr[r] = 0;
    	
    	//printf("%d\t",r);
    	break;
	}
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
     
     ///////////////////////////////
    TEST("zero a late byte");
    while(true) {
  	   r = rand() % (data_len/3) + (2*data_len/3)-10;
    	if (test_arr[r] == 0) continue;
    	test_arr[r] = 0;
    	
    	//printf("%d\t",r);
    	break;
	}
	
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
  ///////////////////////////////
  /*  TEST("0xFF a middle byte");
    
    while(true) {
  	   r = rand() % (data_len/3) + (data_len/3);
    	if (test_arr[r] == 0xFF) continue;
    	test_arr[r] = 0xFF;
    	
    	//printf("%d\t",r);
    	break;
	}
    
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    
    ///////////////////////////////
    TEST("0xFF an early byte");
    while(true) {
  	   r = rand() % (data_len/3);
    	if (test_arr[r] == 0xFF) continue;
    	test_arr[r] = 0xFF;
    	
    	//printf("%d\t",r);
    	break;
	}
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
     
     ///////////////////////////////
    TEST("0xFF a late byte");
    while(true) {
  	   r = rand() % (data_len/3) + (2*data_len/3)-10;
    	if (test_arr[r] == 0xFF) continue;
    	test_arr[r] = 0xFF;
    	
    	//printf("%d\t",r);
    	break;
	}
	
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
  */
    ///////////////////////////////
    TEST("lose a middle bit");
     r1 = rand() % (data_len/3) + (data_len/3);
     r2 = 1 << (rand() % 7);
     
    //printf("%d %d\t",r1,r2);
    test_arr[r1] = test_arr[r1] ^ r2;
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    ///////////////////////////////
    TEST("lose a late bit ");
     r1 = rand() % (data_len/3) + (2*data_len/3)-10;
     r2 = 1 << (rand() % 7);
     
    //printf("%d %d\t",r1,r2);
    test_arr[r1] = test_arr[r1] ^ r2;
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
   
    ///////////////////////////////
    TEST("lose an early bit");
     r1 = rand() % (data_len/3);
     r2 = 1 << (rand() % 7);
     
    //printf("%d %d\t",r1,r2);
    test_arr[r1] = test_arr[r1] ^ r2;
    for (int i = 0; i < data_len; i++) {
    	crc = crc_update(crc, test_arr[i]);
    }
    
    END_TEST();
    
    //printf("Total diff: %d / %d\n", diff_s,diff_t);
}

int main (int argc, char ** argv) {
    // vars n shit bitches
 	for (int i = 0; i < 20; i++) 
   		failures[i] = 0;
   	
   	srand(time(0));
   	
   	for (int i =0; i < 10000; i++)
   		crc_test();
   	
   	for (int i = 0; i < 20; i++) {
   		printf("%d: %d failures\n", i, failures[i]);
   	}
}

