#ifndef __UTIL_QUEUE__FIFO_MULTI_ONE_H_
#define __UTIL_QUEUE__FIFO_MULTI_ONE_H_

#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>


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
    uint32_t memory_size;
    char pad[CACHE_LINE_SIZE - sizeof(uint32_t) - sizeof(uint32_t)];

    struct 
    {
        void *m;
        /*
         *0x00   -->can push, can not pop
         *ox01   -->pushing, can not pop
         *0x10   -->can pop, can not push
         *0x11   -->poping, can not push
         */
        volatile unsigned char lock;
    }msgs[1];
}memqueue_s;

__attribute((always_inline)) inline memqueue_s* initQ(uint32_t capacity)
{
    memqueue_s* q = NULL;
    uint32_t cap = 0;
    if(capacity > 16)
    {
        return NULL;
    }
    cap = (1<<capacity);
    int pagesize = getpagesize(); 
    unsigned int size = sizeof(memqueue_s) + cap * sizeof(q->msgs[0]);
    size = (size + pagesize -1)/pagesize * pagesize;
    q = (memqueue_s*)mmap(NULL, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(MAP_FAILED == q)
    {
        assert(0);
        return NULL;
    }
    memset(q, 0, size);
    q->mask = cap-1;
    q->memory_size = size;
    return q;
}
 
__attribute((always_inline)) inline int push(memqueue_s* q, void* memory)
{
    uint32_t head, tail, pos, next, size;
    int ok = 0;
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
            next = head +1;
            ok = __sync_bool_compare_and_swap(&q->header.head, head, next);
            if(ok)
            {
                break;
            }
            else
            {
                continue;
            }
        }
        else if(size > mask)
        {
            //assert(NULL);
            continue;
        }
        else
        {
            return 1;
        }
    }while(!ok);

    pos = head & mask;
    do
    {
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].lock, 0x0, 0x01); // wait can be push
    }while(!ok);
    asm volatile ("":::"memory");
    q->msgs[pos].m = memory;
    asm volatile ("":::"memory");
    q->msgs[pos].lock = 0x10;
    return 0;
}

__attribute((always_inline)) inline int pop(memqueue_s* q, void** data)
{
    uint32_t head, tail, pos; 
    int ok = 0;
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
    do
    {
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].lock, 0x10, 0x11);
    }while(!ok);
    asm volatile ("":::"memory");
    *data = q->msgs[pos].m;
    asm volatile ("":::"memory");
    q->msgs[pos].lock = 0x0;
    q->tail.tail++;
    return 0;
}

__attribute((always_inline)) inline int get(memqueue_s* q, void** data)
{
    uint32_t head, tail, pos; 
    int ok = 0;
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
    do
    {
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].lock, 0x10, 0x11);
    }while(!ok);
    asm volatile ("":::"memory");
    *data = q->msgs[pos].m;
    asm volatile ("":::"memory");
    q->msgs[pos].lock = 0x0;
    //q->tail.tail++;
    return 0;
}

__attribute((always_inline)) inline int size(memqueue_s* q)
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


__attribute((always_inline)) inline void freeQ(memqueue_s* q)
{
    if(q)
    {
        munmap(q, q->memory_size);
    }
}

#ifdef __cplusplus
}
#endif
#endif
