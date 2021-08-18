#ifndef  __MEMORY_STACK_H__
#define  __MEMORY_STACK_H__

//#include "switch.h"

#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>
#include<malloc.h>
#include <sched.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define CACHE_LINE_SIZE         64 
typedef struct memqueue_t
{

    uint32_t cap;
    char pad1[CACHE_LINE_SIZE - sizeof(uint32_t)];

    volatile uint32_t head;
    char pad2[CACHE_LINE_SIZE - sizeof(uint32_t)];

    volatile int32_t lock;
    char pad3[CACHE_LINE_SIZE - sizeof(int32_t)];

    void* msgs[1];
}memqueue_s;


//__attribute((always_inline)) 
static inline void s_lock(volatile int32_t* lock) 
{
    if (__sync_lock_test_and_set(lock, 1)) 
    {
        uint32_t spins = 0;
        while (*lock != 0 || __sync_lock_test_and_set(lock, 1)) 
        {
            if ((++spins & 1023) == 0) 
            {
                sched_yield();
            }            
            _mm_pause();
        }                                                                                          
    }                                                                                            
}


//__attribute((always_inline)) 
static inline void s_unlock(volatile int32_t* lock) 
{
    __sync_lock_release(lock);                                                      
}

//__attribute((always_inline)) 
static inline memqueue_s* memQinit(uint32_t capacity)
{       
    memqueue_s* q = NULL;
    uint32_t cap = 0;
    if(capacity > 31)
    {   
        return NULL;
    }   
    cap = (1<<capacity);                                                                       
    q = (memqueue_s*)memalign(64,sizeof(memqueue_s) + cap * sizeof(q->msgs[0]));
    memset(q, 0, sizeof(memqueue_s) + cap * sizeof(q->msgs[0]));
    q->cap = cap;
    q->head = 1;   
    return q;
}   
 
//__attribute((always_inline)) 
static inline int mempush(memqueue_s* q, void* memory)
{
    s_lock(&q->lock);
    if(q->head < q->cap)
    {
        q->msgs[q->head] = memory;
        q->head++;
        s_unlock(&q->lock);
    }
    else
    {
        s_unlock(&q->lock);
        return 1;
    }
    return 0;
}

//__attribute((always_inline)) 
static inline void* mempop(memqueue_s* q)
{
    uint32_t pos; 
    void* ret = NULL;
   
    //if(q->head == 1)
    //{
    //     return NULL;
    //}


    s_lock(&q->lock);

    pos = q->head-1; 
    if(pos > 0)
    {
        ret = q->msgs[pos];
        q->head--;   
        s_unlock(&q->lock);
    }
    else
    {
        s_unlock(&q->lock);
        return NULL;
    }
    return ret;
}


#ifdef __cplusplus
}
#endif
#endif
