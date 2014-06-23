#ifndef __LOADER_H__
#define __LOADER_H__

// flag bits
#define BOOT_SREC          1
#define ALLOW_FLASH        2
#define ALLOW_LOADER       4

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
#define CMD_BOOT	    0xCB
#define CMD_RESET	    0xCF
#define CMD_QCRC	    0xCC
#define CMD_HIRAM	    0xCE
#define CMD_SREC	    0xCD
#define CMD_SET_BOOT	0xC1
#define CMD_SET_FLWR	0xC2
#define CMD_SET_LDWR	0xC3

#endif
