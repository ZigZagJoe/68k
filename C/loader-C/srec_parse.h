#ifndef __SREC_PARSE_H__
#define __SREC_PARSE_H__

#include <srec_private.h>

// flag bits
#define ALLOW_FLASH          1
#define ALLOW_LOADER         2
#define BINARY_SREC          4

// errno bits
#define BAD_HEX_CHAR         1
#define INVALID_WRITE        2
#define FAILED_WRITE         4
#define FORMAT_ERROR         8
#define EARLY_EOF           16
#define __UNUSED_ERROR      32
#define CRC_ERROR           64
#define PROG_FAILURE       128

// 256kb / 4kb sectors
#define SECTOR_COUNT      64

// srec parsing
// provided by parse_srec.c
uint8_t parseSREC(uint8_t * buffer, uint32_t buffer_len, uint8_t fl, uint8_t armed);

#endif
