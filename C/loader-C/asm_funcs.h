#ifndef __ASM_FUNCS_H__
#define __ASM_FUNCS_H__

// prototypes for ASM functions
uint8_t getb();
uint16_t getw();
uint32_t getl();

void putw();
void putb();
void putl();

void puts(char * str);

void print_dec(uint32_t d);
void puthexlong(uint32_t l);
void puthexword(uint32_t w);
void puthexbyte(uint32_t b);

uint8_t check_reset_cmd();

#endif
