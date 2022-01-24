#ifndef __MEMORY_QUEUE__NOT_FIFO_H_
#define __MEMORY_QUEUE__NOT_FIFO_H_

#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>

#ifdef __cplusplus
extern "C"
{
#endif


#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE         64 
#endif
typedef struct memqueue_t
{
    struct
    {
        volatile uint32_t head;
        char pad1[CACHE_LINE_SIZE - sizeof(uint32_t)];
    } header;

    struct
    {
        volatile uint32_t tail;
        char pad2[CACHE_LINE_SIZE - sizeof(uint32_t)];
    } tail;
    uint32_t mask;
    char pad[CACHE_LINE_SIZE - sizeof(uint32_t)];

    struct 
    {
        void *m;
        volatile int flag;
    }msgs[1];
}memqueue_s;

__attribute((always_inline)) static inline memqueue_s* memQinit(uint32_t capacity)
{
    memqueue_s* q = NULL;
    uint32_t cap = 0;
    if(capacity > 31)
    {
        return NULL;
    }
    cap = (1<<capacity);
    q = (memqueue_s*)malloc(sizeof(memqueue_s) + cap * sizeof(q->msgs[0]));
    memset(q, 0, sizeof(memqueue_s) + cap * sizeof(q->msgs[0]));
    q->mask = cap-1;
    
    return q;
}   
 
int mempush(memqueue_s* q, void* memory)
{
    uint32_t head, tail, next;
    int ok;
    uint32_t mask = q->mask;
    uint32_t size;   
    do
    {
        head = q->header.head;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        tail = q->tail.tail;
        size = head - tail;
        if(size >= mask)
        {
            return 1;
        }
        next = head +1;
        ok = __sync_bool_compare_and_swap(&q->header.head, head, next);
    }while(!ok);

    while(1 == q->msgs[head & mask].flag)
    {
        _mm_pause();
    }
    q->msgs[head & mask].m = memory;
    asm volatile ("":::"memory");
    q->msgs[head & mask].flag = 1;
    return 0;
}

void* mempop(memqueue_s* q)
{
    uint32_t head, tail, next, size; 
    int ok;
    uint32_t mask = q->mask;
    void* ret = NULL;    

    do
    {
        tail = q->tail.tail;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        head = q->header.head;
        size = head - tail;
        if(size == 0)
        {
            return NULL;
        }
        next = tail +1;
        ok = __sync_bool_compare_and_swap(&q->tail.tail, tail, next); 
    }while(!ok);
    
    while(0 == q->msgs[tail & mask].flag)
    {
        _mm_pause();
    }
 
    ret = q->msgs[tail & mask].m;
    asm volatile ("":::"memory");
    q->msgs[tail & mask].flag = 0;
    return ret;
}


#ifdef __cplusplus
}
#endif
#endif
