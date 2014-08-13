#include <kernel.h>

volatile uint32_t millis_counter;

volatile uint32_t millis() {
	return millis_counter * TICK_INTERVAL;
}

task_id_t get_task_id(task_t task) {
	return (task_id_t)(task & 0xFFFF);
}

