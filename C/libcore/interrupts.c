#include <stdint.h>
#include <stdlib.h>
#include <interrupts.h>

__attribute__ ((deprecated)) void default_interrupts() { }

void soft_reset() {
	_JMP(0x80008);
}
