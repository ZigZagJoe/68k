#include <ctype.h>

uint8_t isdigit(char ch) {
	return (ch >= '0') && (ch <= '9');
}
