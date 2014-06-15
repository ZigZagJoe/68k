#include <string.h>

void *memcpy(void *dst, const void *src, size_t len) {
	uint8_t *ptr_s = src;
	uint8_t *ptr_d = dst;
	
	for (size_t i = 0; i < len; i++) 
		*ptr_d++ = *ptr_s++;
		
	return dst;
}
