#ifndef __ASM_FUNCS_H__
#define __ASM_FUNCS_H__

// prototypes for ASM functions

// synchronous read byte from serial
uint8_t getb();

// read big-endian word/long from serial
uint16_t getw();
uint32_t getl();

// synchronous put byte over serial
void putb();

// put big endian word/long to serial
void putw();
void putl();

// put null terminated string to serial
void puts(char * str);

// write decimal number to serial; return num digits
// max printable value: 655359
int print_dec(uint32_t d);

// write 0-padded hex values to serial
void puthexlong(uint32_t l);
void puthexword(uint32_t w);
void puthexbyte(uint32_t b);

// computes qcrc of count bytes, starting from addr
uint32_t compute_crc(uint8_t *addr, uint32_t count);

// check for reset command. return 1 if reset received, otherwise 0.
// WARNING: consumes serial characters!
uint8_t check_reset_cmd();

#endif
