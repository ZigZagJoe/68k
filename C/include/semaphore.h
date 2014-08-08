#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

typedef volatile uint8_t sem_t;

// try to acquire semaphore: yields until semaphore available
extern void sem_acquire(sem_t * const sem_var);

// try to acquire semaphore and return nonzero on success
extern bool sem_try(sem_t * const sem_var);

// release semaphore
extern void sem_release(sem_t * const sem_var);

// initialize semaphore
extern void sem_init(sem_t * const sem_var);

#endif
