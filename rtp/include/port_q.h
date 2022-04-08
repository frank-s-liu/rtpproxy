#ifndef __UTIL_QUEUE__NOT_FIFO_H_
#define __UTIL_QUEUE__NOT_FIFO_H_

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
typedef struct portQ_t
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
    uint32_t memory_size;
    char pad[CACHE_LINE_SIZE - sizeof(uint32_t) - sizeof(uint32_t)];

    struct 
    {
        unsigned short m;
        /*
         *0x00   -->can push, can not pop
         *ox01   -->pushing, can not pop
         *0x10   -->can pop, can not push
         *0x11   -->poping, can not push
         */
        volatile unsigned char lock;
    }msgs[1];
}portQ_s;

__attribute((always_inline)) inline portQ_s* initPortsQ(uint32_t capacity)
{
    portQ_s* q = NULL;
    uint32_t cap = 0;
    if(capacity > 17)
    {
        return NULL;
    }
    cap = (1<<capacity);
    int pagesize = getpagesize();
    unsigned int size = sizeof(portQ_s) + cap * sizeof(q->msgs[0]);
    size = (size + pagesize -1)/pagesize * pagesize;
    q = (portQ_s*)mmap(NULL, size, PROT_READ | PROT_WRITE,
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
 
__attribute((always_inline)) inline int pushPort(portQ_s* q, unsigned short port)
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
            continue;
        }
        else
        {
            //assert(NULL);
            return 1;
        }
    }while(!ok);

    pos = head & mask;
    do
    {
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].lock, 0x0, 0x01);
    }while(!ok);
    asm volatile ("":::"memory");
    q->msgs[pos].m = port;
    asm volatile ("":::"memory");
    q->msgs[pos].lock = 0x10;
    return 0;
}

__attribute((always_inline)) inline int popPort(portQ_s* q, unsigned short* port)
{
    uint32_t head, tail, size, pos, next; 
    int ok = 0;
    uint32_t mask = q->mask;

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
            *port = 0;
            return 1;
        }

        next = tail +1;
        ok = __sync_bool_compare_and_swap(&q->tail.tail, tail, next);
    }while(!ok);
    
    pos = tail & mask;
    do
    {
        ok = __sync_bool_compare_and_swap(&q->msgs[pos].lock, 0x10, 0x11);
    }while(!ok);
    asm volatile ("":::"memory");
    *port = q->msgs[pos].m;
    asm volatile ("":::"memory");
    q->msgs[pos].lock = 0;
    return 0;
}

__attribute((always_inline)) inline void freePortsQ(portQ_s* q)
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
