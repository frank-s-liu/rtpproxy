#ifndef __UTIL_QUEUE__FIFO_ONE_ONE_H_
#define __UTIL_QUEUE__FIFO_ONE_ONE_H_

#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CACHE_LINE_SIZE         64 
typedef struct memqueue_t
{
    union
    {
        volatile uint32_t head;
        char pad1[CACHE_LINE_SIZE];
    } header;

    union
    {
        volatile uint32_t tail;
        char pad2[CACHE_LINE_SIZE];
    } tail;

    uint32_t mask;
    char pad[CACHE_LINE_SIZE - sizeof(uint32_t)];

    void* m[1];
}memqueue_s;

__attribute((always_inline)) static inline memqueue_s* initQ(uint32_t capacity)
{       
    memqueue_s* q = NULL;
    uint32_t cap = 0;
    if(capacity > 31)
    {   
        return NULL;
    }   
    cap = (1<<capacity);                                                                       
    q = (memqueue_s*)malloc(sizeof(memqueue_s) + cap * sizeof(q->m));
    memset(q, 0, sizeof(memqueue_s) + cap * sizeof(q->m));
    q->mask = cap-1;
    
    return q;
}   
 
__attribute((always_inline)) static inline int push(memqueue_s* q, void* memory)
{
    uint32_t head, tail, pos, size;
    uint32_t mask = q->mask;
    do
    {
        head = q->header.head;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        tail = q->tail.tail;
        size = head - tail;
        if(size < mask)
        {
                break;
        }
        else if(size > mask)
        {
            assert(NULL);
            continue;
        }
        else
        {
            return 1;
        }
    }while(1);

    pos = head & mask;
    q->m[pos] = memory;
    q->header.head++;
    return 0;
}

__attribute((always_inline)) static inline int pop(memqueue_s* q, void** data)
{
    uint32_t head, tail, pos; 
    uint32_t mask = q->mask;

    tail = q->tail.tail;
#ifndef __x86_64__
    asm volatile ("mfence":::"memory");
#endif
    head = q->header.head;
    if( (head-tail) == 0)
    {
        *data = NULL;
        return 1;
    }

    pos = tail & mask;
    *data = q->m[pos];

    q->tail.tail++;
    return 0;
}


__attribute((always_inline)) static inline int size(memqueue_s* q)
{
    uint32_t head, tail, size;
    while(1)
    {
        head = q->header.head;
#ifndef __x86_64__
        asm volatile ("mfence":::"memory");
#endif
        tail = q->tail.tail;
        if(head < tail)
        {
            continue;
        }
        size = head - tail;
        return size;
    }
}


#ifdef __cplusplus
}
#endif
#endif
