#ifndef  __MEMORY_LOCK_FREE_STACK_H__
#define  __MEMORY_LOCK_FREE_STACK_H__

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
typedef struct memstack_t
{

    volatile uint64_t top;
    char pad1[CACHE_LINE_SIZE - sizeof(uint64_t)];

    struct
    {
        uint64_t mask;
        void* m;
    }msgs[1];
}memstack_s;



//__attribute((always_inline)) 
static inline memstack_s* memQinit(uint32_t capacity)
{
    memstack_s* q = NULL;
    uint64_t cap = 0;
    if(capacity > 31)
    {   
        return NULL;
    }   
    cap = (1<<capacity);                                                                       
    q = (memstack_s*)memalign(64,sizeof(memstack_s) + cap * sizeof(q->msgs[0]));
    memset(q, 0, sizeof(memstack_s) + cap * sizeof(q->msgs[0]));
    q->top = 0x01;
    q->top |= (cap<<32);
    return q;
}   
 
__attribute((always_inline)) static inline 
int mempush(memstack_s* q, void* memory)
{
    uint64_t pos, top, ok;
    uint64_t mask, mask_lock; // push_mask 4byte popmask 4byte
    do
    {
        top = q->top;
        pos = top & 0xFFFFFFFF;
        if(pos >= (top >> 32))
        {
            return 1; //full
        }
        mask = q->msgs[pos].mask & 0xFFFFFFFF00000000;
        mask_lock = mask + 0x100000000;
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].mask, mask, mask_lock);  // no pop on this slot, lock this slot, can't pop, but don't lock push
        if(!ok)
        {
            continue;
        }
        else
        {
            ok = __sync_bool_compare_and_swap(&q->top, top, top+1);
            if(ok)
            {
                break;
            }
            else
            {
                do
                {
                    mask = q->msgs[pos].mask & 0xFFFFFFFF00000000;
                    mask_lock = mask - 0x100000000;
                    ok = __sync_bool_compare_and_swap(&q->msgs[pos].mask, mask, mask_lock);
                }
                while(!ok);
                continue;
            }
        }
    }while(1);

    q->msgs[pos].m = memory;
    asm volatile ("":::"memory");
    do
    {
        mask = q->msgs[pos].mask & 0xFFFFFFFF00000000;
        mask_lock = mask - 0x100000000;
	ok =  __sync_bool_compare_and_swap(&q->msgs[pos].mask, mask, mask_lock);
    }
    while(!ok);

    return 0;
}

__attribute((always_inline)) 
static inline void* mempop(memstack_s* q)
{
    uint64_t top, pos, ok;
    uint64_t mask, mask_lock; 
    void* ret = NULL;
 
    do 
    {
        top = q->top;
        pos = (top & 0xFFFFFFFF)-1;
        if(pos == 0)
        {
            return NULL;
        }
        mask = q->msgs[pos].mask & 0xFFFFFFFF;
        mask_lock = mask + 0x01;
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].mask, mask, mask_lock);
        if(ok)
        {
            ok = __sync_bool_compare_and_swap(&q->top, top, top-1);
            if(ok)
            {
                break;
            }
            else
            {
                do
                {
                    mask = q->msgs[pos].mask & 0xFFFFFFFF;
                    mask_lock = mask - 0x01;
                    ok = __sync_bool_compare_and_swap(&q->msgs[pos].mask, mask, mask_lock);
                    
                }while(!ok);
                continue;
            }
        }
        else
        {
            continue;
        }
    }while(1);
    ret = q->msgs[pos].m;
    asm volatile ("":::"memory");
    do
    {
        mask = q->msgs[pos].mask & 0xFFFFFFFF;
        mask_lock = mask - 0x01;
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].mask, mask, mask_lock);
        
    }while(!ok);
    return ret;
}


#ifdef __cplusplus
}
#endif
#endif
