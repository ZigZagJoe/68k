#ifndef __LOADER_H__
#define __LOADER_H__

#include <stdint.h>

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
#define CMD_SET_ADDR    0xC4
#define CMD_SET_BINSR   0xC5
// unused 0xC6-C9
#define CMD_DUMP        0xCA
#define CMD_BOOT	    0xCB
#define CMD_QCRC	    0xCC
#define CMD_SREC	    0xCD
// unused 0xCE
#define CMD_RESET	    0xCF

// magic flag values
#define BOOT_MAGIC      0xD0B07CDE
#define FLWR_MAGIC      0xF1A5C0DE
#define LDWR_MAGIC      0x10ADC0DE
#define ADDR_MAGIC      0xCE110C00
#define ADDR_TAIL_MAGIC   0xACC0
#define BINSREC_MAGIC   0xB17AC5EC
#define DUMP_GREET_MAGIC  0x10AD
#define DUMP_START_MAGIC  0x1F07
#define DUMP_TAIL_MAGIC   0xEEAC

// important addresses
#define FLASH_START       0x80000
#define IO_START          0xC0000
#define LOADER_END        0x80FFF
#define LOWEST_VALID_ADDR 0x02000

#define SECTOR_COUNT     64

uint8_t parseSREC(uint8_t * buffer, uint32_t buffer_len, uint8_t fl, uint8_t armed);

//########################################################################
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
#define flash_arm ;//

#define flash_write_byte(A,V) MEMORY[A] = V;
#define MEM(X) MEMORY[X]
#define ADDR_TO_SECTOR(X) (((X) & 0x3FFFF) >> 12)  
    
#endif

#endif
