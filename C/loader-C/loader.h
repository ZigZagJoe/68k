#ifndef __LOADER_H__
#define __LOADER_H__

// flag bits
#define BOOT_SREC          1
#define ALLOW_FLASH        2
#define ALLOW_LOADER       4
#define BINARY_SREC        8

// errno bits
#define BAD_HEX_CHAR       1
#define INVALID_WRITE      2
#define FAILED_WRITE       4
#define FORMAT_ERROR       8
#define EARLY_EOF         16
#define INVALID_CHAR      32
#define CRC_ERROR         64
#define PROG_FAILURE     128

// command codes
// unused 0xC0
#define CMD_SET_BOOT	0xC1
#define CMD_SET_FLWR	0xC2
#define CMD_SET_LDWR	0xC3
#define CMD_SET_HIRAM   0xC4
#define CMD_SET_BINSR   0xC5
// unused 0xC6-CA
#define CMD_BOOT	    0xCB
#define CMD_QCRC	    0xCC
#define CMD_SREC	    0xCD
// unused 0xCE
#define CMD_RESET	    0xCF

// magic flag values
#define BOOT_MAGIC      0xD0B07CDE
#define FLWR_MAGIC      0xF1A5C0DE
#define LDWR_MAGIC      0x10ADC0DE
#define HIRAM_MAGIC     0xCE110C00
#define BINSREC_MAGIC   0xB17AC5EC

// important addresses
#define FLASH_START 0x80000
#define IO_START    0xC0000
#define LOADER_END  0x80FFF
#define LOWEST_VALID_ADDR 0x2000

#define SECTOR_COUNT     64

// conditional defines for parse_srec.c
#ifndef UPLOADER    /* ############ code for loader-C ################# */

#include <io.h>

#define dbgprintf ; //
// comment out the function call 
// have ; so that if (x) dbgprintf(...) does not break stuff

#else               /* ############ code for upload-loader ############ */
          
#include <stdio.h>
#define dbgprintf printf

extern uint8_t MEMORY[0x100000];

// don't use...
#define flash_erase_sector ; //

#define flash_write_byte(A,V) MEMORY[A] = V;
#define MEM(X) MEMORY[X]
#define ADDR_TO_SECTOR(X) (((X) & 0x3FFFF) >> 12)  
    
#endif

#endif
