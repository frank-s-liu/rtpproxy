#include "cqueue.h"
#include "commonError.h"

#include <stdint.h>
#include <stdlib.h>
#include <emmintrin.h>
#include <string.h> 


#ifndef likely
#define likely(x)       __builtin_expect((x), 1)
#endif
 
#ifndef unlikely
#define unlikely(x)     __builtin_expect((x), 0)
#endif
 
 
 
 
#define CACHE_LINE_SIZE         64
 
struct cqueue_t 
{
    struct 
    {
        volatile uint32_t head;
        char pad1[CACHE_LINE_SIZE - sizeof(uint32_t)];
        volatile uint32_t tail;
        char pad2[CACHE_LINE_SIZE - sizeof(uint32_t)];
    } header;
 
    struct 
    {
        volatile uint32_t head;
        char pad1[CACHE_LINE_SIZE - sizeof(uint32_t)];
        volatile uint32_t tail;
        char pad2[CACHE_LINE_SIZE - sizeof(uint32_t)];
    } tail;
    uint32_t mask;
    char pad[CACHE_LINE_SIZE - sizeof(uint32_t)];
 
    void    *msgs[0];
};
 
struct cqueue_t* cqinit(uint32_t capacity)
{
    struct cqueue_t* q = NULL;
    uint32_t cap = 0;
    if(capacity > 31)
    {
        return NULL;
    }
    cap = (1<<capacity);
    q = (struct cqueue_t*)malloc(sizeof(struct cqueue_t) + cap * sizeof(void *));
    memset(q, 0, sizeof(struct cqueue_t) + cap * sizeof(void *));
    q->mask = cap-1;

    return q;
}
 
 
int cpush(struct cqueue_t *q, void *m)
{
    uint32_t head, tail, mask, next;
    int ok;

    mask = q->mask;

    do 
    {
        // we don't need mem fence here on x86_64 platform 
        // only allowed "store can be reorder after load"  on x86_64 
        head = q->header.head;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        tail = q->tail.tail;
        /////////////////

        if ((mask + tail - head) < 1U)
        {
            return QUEUE_FULL;
        }
        next = head + 1;
        ok = __sync_bool_compare_and_swap(&q->header.head, head, next);
    } while (!ok);

    q->msgs[head & mask] = m;

    // make sure get variable from memory while not register and cache 
    // used for GCC not runtime
    asm volatile ("":::"memory");
    
    // guarantee the push is finished, 
    // if this push thread is hold before (q->msgs[head & mask] = m;) is executed,
    // this element can't be pop 
    while (unlikely((q->header.tail != head)))
    {
        // if one push thread was hold before executing (q->p.tail = next);
        // other push thread will do this loop until that thread wake
        _mm_pause();
    }
    q->header.tail = next;

    return SUCCESS;
}
 
void* cpop(struct cqueue_t *q)
{
    uint32_t head, tail, mask, next;
    int ok;
    void *ret;
    mask = q->mask;

    do 
    {
        head = q->tail.head;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        tail = q->header.tail;
        if ((tail - head) < 1U)
        {
                return NULL;
        }
        next = head + 1;
        ok = __sync_bool_compare_and_swap(&q->tail.head, head, next);
    } while (!ok);

    ret = q->msgs[head & mask];
    asm volatile ("":::"memory");

    while (unlikely((q->tail.tail != head)))
    {
        _mm_pause();
    }
    q->tail.tail = next;

    return ret;
}
