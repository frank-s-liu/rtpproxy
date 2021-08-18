#include "memqueue.h"

#include <stdint.h>
#include <stdlib.h>
#include <emmintrin.h>
#include <string.h> 
#include <assert.h>
#include <sched.h>

#ifndef likely
#define likely(x)       __builtin_expect((x), 1)
#endif
 
#ifndef unlikely
#define unlikely(x)     __builtin_expect((x), 0)
#endif
 
 
 
 
#define CACHE_LINE_SIZE         64
 
extern void* __real_malloc(size_t size);

struct memqueue_t 
{
    union
    {
        struct 
        {
            volatile uint32_t head;
            volatile uint32_t tail;
            uint32_t mask;
        }s;
        char pad1[CACHE_LINE_SIZE];
    }up;
    union
    {
        struct 
        {
            volatile uint32_t head;
            volatile uint32_t tail;
            uint32_t mask;
        }s;
        char pad2[CACHE_LINE_SIZE];
    }uc;
    void* msgs[CACHE_LINE_SIZE/sizeof(void*)];
};
 
struct memqueue_t* memQinit(uint32_t capacity)
{
    struct memqueue_t* q = NULL;
    uint32_t cap = 0;
    if(capacity > 31)
    {
        return NULL;
    }
    cap = (1<<capacity);
    q = (struct memqueue_t*)malloc(sizeof(struct memqueue_t) + cap * sizeof(void *));
    memset(q, 0, sizeof(struct memqueue_t) + cap * sizeof(void *));
    q->uc.s.mask = cap-1;
    q->up.s.mask = cap-1;

    return q;
}
 
 
int mempush(struct memqueue_t *q, void *m)
{
    uint32_t head, tail, mask, next;
    int ok;
    int loopcount = 1024;
    mask = q->up.s.mask;

    do 
    {
        // we don't need mem fence here on x86 platform 
        head = q->up.s.head;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        tail = q->uc.s.tail;
        /////////////////

        if ((mask + tail - head) < 1U)
        {
            assert(NULL);
            continue;
        }
        next = head + 1;
        ok = __sync_bool_compare_and_swap(&q->up.s.head, head, next);
    } while (!ok);

    q->msgs[head & mask] = m;
    asm volatile ("":::"memory");

    while (unlikely((q->up.s.tail != head)))
    {
        if(loopcount == 0)
        {
            loopcount = 1024;
            sched_yield();
        }
        loopcount--;    
        _mm_pause();
    }
    q->up.s.tail = next;

    return 0;
}
 
void* mempop(struct memqueue_t *q)
{
    uint32_t head, tail, mask, next;
    int ok;
    void *ret;
    int loopcount = 1024;
    mask = q->uc.s.mask;

    do 
    {
        head = q->uc.s.head;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        tail = q->up.s.tail;
        if ((tail - head) < 1U)
        {
                return NULL;
        }
        next = head + 1;
        ok = __sync_bool_compare_and_swap(&q->uc.s.head, head, next);
    } while (!ok);

    ret = q->msgs[head & mask];
    asm volatile ("":::"memory");

    while (unlikely((q->uc.s.tail != head)))
    {
        if(loopcount == 0)
        {
            loopcount = 1024;
            sched_yield();
        }
        loopcount--;    
        _mm_pause();
    }
    q->uc.s.tail = next;

    return ret;
}
