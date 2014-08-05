#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

typedef volatile uint8_t sem_t;

extern void sem_acquire(sem_t * const sem_var);
extern void sem_release(sem_t * const sem_var);
extern void sem_init(sem_t * const sem_var);

#endif
