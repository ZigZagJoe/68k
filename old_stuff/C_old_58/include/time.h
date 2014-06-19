#ifndef __TIME_H__
#define __TIME_H__

#include <stdint.h>

volatile uint32_t millis();
void millis_start();
void millis_stop();

#endif
