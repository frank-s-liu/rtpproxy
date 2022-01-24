#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <sched.h>
#define CPU_PAUSE __asm__ ("pause") 
 
typedef struct spinlock 
{
  volatile int m_isLocked;
} spinLock;


static inline void spinlock_init(spinLock* lock) 
{
    lock->m_isLocked = 0;
}
                                                                                                          
static inline void spinlock_lock(spinLock* lock) 
{
    int spins = 0;
    while ((lock->m_isLocked != 0) || __sync_lock_test_and_set(&lock->m_isLocked, 1)) 
    {
        if ((++spins & 8191) == 0) 
        {
            sched_yield();
        }
        CPU_PAUSE;
    }
}
                                                                                                
static inline void spinlock_unlock(spinLock *lock) 
{
  __sync_lock_release(&lock->m_isLocked);
}
#endif                                                                                                                                               
