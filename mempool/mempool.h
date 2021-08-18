#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "memory.h"
//#include "memqueue.h"
//#include "memoryqueue.h"
//#include "memorystack.h"
#include "memorylockfreestack.h"
#include <stddef.h>

#define DEBUG_INFO_LEN 16

typedef memstack_s mem_stack;

typedef struct memblock
{
    int malloc_flag;
    unsigned int block_size;   
    memstack_s* memQ;  // apending addr  
    struct memchunk* chunk;
}memblock_header; 


struct memchunk
{
    char* block_addr_start;
    unsigned int  init_block_counter;
    unsigned int  free_block_counter;
    char pad[64-(sizeof(unsigned int)*2 + sizeof(char*))];
    memstack_s* memQ;
    mempool_t* pool;
};



static const int infoAlign = (sizeof(memblock_t) + 15)/16 *16;
//static const int blockAlign = (sizeof(memblock_t) + 15)/16 *16;

void* __real_malloc(size_t size); // just state

int   memoryPoolInit(MemPools* mempools);



#endif

