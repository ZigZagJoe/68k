#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stdint.h>
#include <interrupts.h>

typedef uint16_t task_id_t;
typedef uint32_t task_t;

// 31 of these start in memory from 0x460
typedef volatile struct __attribute__((packed)) {
    task_id_t ID;			 // 0
    uint8_t __reserved1;	 // 2
    uint8_t runnable;		 // 3
    uint32_t next_task;		 // 4
    uint32_t sleep_time;	 // 8
    uint8_t __reserved2[14]; // 12
    uint32_t D[8];			 // 26
    uint32_t A[7];			 // 58
    uint32_t SP;			 // 86
    uint32_t PC;			 // 90
    uint16_t FLAGS;			 // 94
} task_struct_t;             // total: 96

extern task_struct_t * _active_task;

#define CURRENT_TASK_ID (_active_task->ID)
#define abort() exit(0xAB)

// create a new task. returns 0 on failure or a task_struct
task_t create_task(void *task, uint16_t argc, ...);

// voluntarily yield control to next task
void yield(void);

// sleep for at least time milliseconds and no more than time+(task_count-1)*time_slice
void sleep_for(uint32_t time);

// exit the current task
void exit_task(void);

// return the current time since boot, in milliseconds.
volatile uint32_t millis();

// yield until task exits
void wait_for_exit(task_t task);

// simple critical section support (disables interrupts)
void enter_critical();
void leave_critical();

// stops execution with code 
void exit(uint8_t code);

// utility functions
task_id_t get_task_id(task_t task);

#endif
