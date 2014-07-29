#ifndef __LOADER_H__
#define __LOADER_H__

#include <stdint.h>

// command codes
// unused 0xC0
#define CMD_SET_FLWR	  0xC1
#define CMD_SET_LDWR	  0xC2
#define CMD_SET_BINSR     0xC3
// unused 0xC4-C8
#define CMD_INFLATE       0xC9
#define CMD_DUMP          0xCA
#define CMD_BOOT	      0xCB
#define CMD_QCRC	      0xCC
#define CMD_SREC	      0xCD
#define CMD_SET_ADDR      0xCE
#define CMD_RESET	      0xCF

// magic values
#define FLWR_MAGIC        0xF1A5C0DE
#define LDWR_MAGIC        0x10ADC0DE
#define BINSREC_MAGIC     0xB17AC5EC

#define ADDR_MAGIC        0xCE110C00
#define ADDR_TAIL_MAGIC   0xACC0

#define DUMP_GREET_MAGIC  0x10AD
#define DUMP_START_MAGIC  0x1F07
#define DUMP_TAIL_MAGIC   0xEEAC

#define SREC_GREET_MAGIC  0xD0E881CC
#define SREC_CODE_MAGIC   0xC0DE
#define SREC_TAIL_MAGIC   0xEF00

#define INFL_GREET_MAGIC  0xD14F1A7E
#define INFL_CODE_MAGIC   0xC0DE
#define INFL_TAIL_MAGIC   0x1F00

// important addresses
#define FLASH_START       0x80000
#define IO_START          0xC0000
#define LOADER_END        0x80FFF
#define LOWEST_VALID_ADDR 0x02000

#include <srec_parse.h>

#endif

