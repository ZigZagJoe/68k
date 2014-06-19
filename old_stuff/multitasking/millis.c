#include <kernel.h>

volatile uint32_t millis_counter;

volatile uint32_t millis() {
	return millis_counter * 10;
}
