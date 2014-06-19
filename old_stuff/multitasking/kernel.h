#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stdint.h>
#include <interrupts.h>

typedef void(*__task)(void);

// starts in memory from 0x460
typedef struct __attribute__((packed)) {
    uint16_t ID;
    uint16_t runnable;
    uint32_t next_task;
    uint32_t sleep_time;
    uint8_t  __reserved[14];
    uint32_t D[8];
    uint32_t A[7];
    uint32_t SP;
    uint32_t PC;
    uint16_t FLAGS;
} task_struct; // 96 bytes

extern task_struct * _active_task;

#define CURRENT_TASK_ID (_active_task->ID)

// create a new task. returns 0 on failure or a task_struct
task_struct * create_task(__task function);

// voluntarily yield control to next task
void yield(void);

// sleep for at least time milliseconds and no more than time+(task_count-1)*time_slice
void sleep_for(uint32_t time);

// exit the current task
void exit_task(void);

// return the current time since boot in milliseconds
volatile uint32_t millis();

#endif
