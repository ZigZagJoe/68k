#include <stdint.h>
#include <stdlib.h>
#include <interrupts.h>

void soft_reset() {
	_JMP(0x80008);
}
