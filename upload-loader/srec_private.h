#ifndef UPLOADER
  #error Wrong srec_private - included header for upload-loader
#endif

/*///// parse_srec header for upload-loader /////*/

#include <stdio.h>

// simulated RAM
extern uint8_t MEMORY[0x100000];

// print all debugging messages
#define dbgprintf printf

// flash simulation
#define flash_write_byte(A,V)  MEMORY[A] = V
#define MEM(X)                 MEMORY[X]
#define ADDR_TO_SECTOR(X)      (((X) & 0x3FFFF) >> 12)  

// disable these functions
// semicolon to not break if(x) yyy(); scenarios
#define flash_erase_sector ; //
#define flash_arm ;//
