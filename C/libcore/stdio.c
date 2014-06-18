#include <stdlib.h>
#include <stdio.h>


void puts(char *str) {
    while (*str != 0)
        putc(*str++);
}


