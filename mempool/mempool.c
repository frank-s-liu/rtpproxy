#include "mempool.h"
#include "memory.h"

#include<malloc.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include <pthread.h>

extern void __real_free(void* ptr);
extern void* __real_malloc(size_t size);

static const unsigned long MAX_HOLD_BLOCK_SIZE = 0x200 ;
//static const unsigned int MAX_FREE_BLOCK_SIZE = CHUNK_128;

static const unsigned int BIGMEMORYFLAG = 0xFFFFFFFF;
//static unsigned int big_mem_counter = 0;

// log2 
__attribute((always_inline)) static inline int logfloor(unsigned int mem_size)
{
    int n = 0;
    int index = 4;
    for(index=4; index>=0; index--)
    {
        int shift = 1<<index;
        if((mem_size >> shift) != 0)
        {
            mem_size >>= shift;
            n += shift;
        }
    }
    return n;
}

int initMemoryPool(MemPools* mempools)
{
    int index = 0;
    int blockindex = 0;
    unsigned int chunk_struct_size = (sizeof(memchunk_t)+15)/16 * 16;
    mempools->pools = (mempool_t*)memalign(64, sizeof(mempool_t)*POOL_SIZE);
    for(index=0; index<POOL_SIZE; index++)
    {
        memset(&mempools->pools[index], 0, sizeof(mempool_t));
    }
    {
        mempools->pools[BLOCK_8_INDEX].block_size = BLOCK_8;
        mempools->pools[BLOCK_16_INDEX].block_size = BLOCK_16;        // 16 * 1
        mempools->pools[BLOCK_32_INDEX].block_size = BLOCK_32;        // 16 * 2
        mempools->pools[BLOCK_48_INDEX].block_size = BLOCK_48;        // 16 * 3
        mempools->pools[BLOCK_64_INDEX].block_size = BLOCK_64;        // 16 * 4
        mempools->pools[BLOCK_80_INDEX].block_size = BLOCK_80;        // 16 * 5
        mempools->pools[BLOCK_96_INDEX].block_size = BLOCK_96;        // 16 * 6
        mempools->pools[BLOCK_112_INDEX].block_size = BLOCK_112;       // 16 * 7
        mempools->pools[BLOCK_128_INDEX].block_size = BLOCK_128;       // 16 * 8
        mempools->pools[BLOCK_144_INDEX].block_size = BLOCK_144;       // 16 * 9
        mempools->pools[BLOCK_160_INDEX].block_size = BLOCK_160;      // 16 * 10
        mempools->pools[BLOCK_176_INDEX].block_size = BLOCK_176;      // 16 * 11
        mempools->pools[BLOCK_192_INDEX].block_size = BLOCK_192;      // 16 * 12
        mempools->pools[BLOCK_208_INDEX].block_size = BLOCK_208;      // 16 * 13
        mempools->pools[BLOCK_224_INDEX].block_size = BLOCK_224;      // 16 * 14
        mempools->pools[BLOCK_240_INDEX].block_size = BLOCK_240;      // 16 * 15
        mempools->pools[BLOCK_256_INDEX].block_size = BLOCK_256;      // 16 * 16
        mempools->pools[BLOCK_288_INDEX].block_size = BLOCK_288;      // 32 * 9
        mempools->pools[BLOCK_320_INDEX].block_size = BLOCK_320;      // 32 * 10
        mempools->pools[BLOCK_352_INDEX].block_size = BLOCK_352;      // 32 * 11
        mempools->pools[BLOCK_384_INDEX].block_size = BLOCK_384;      // 32 * 12
        mempools->pools[BLOCK_416_INDEX].block_size = BLOCK_416;      // 32 * 13
        mempools->pools[BLOCK_448_INDEX].block_size = BLOCK_448;      // 32 * 14
        mempools->pools[BLOCK_480_INDEX].block_size = BLOCK_480;      // 32 * 15
        mempools->pools[BLOCK_512_INDEX].block_size = BLOCK_512;      // 32 * 16        // 512 byte
        mempools->pools[BLOCK_576_INDEX].block_size = BLOCK_576;      // 64 * 9
        mempools->pools[BLOCK_640_INDEX].block_size = BLOCK_640;      // 64 * 10
        mempools->pools[BLOCK_704_INDEX].block_size = BLOCK_704;      // 64 * 11
        mempools->pools[BLOCK_768_INDEX].block_size = BLOCK_768;      // 64 * 12
        mempools->pools[BLOCK_832_INDEX].block_size = BLOCK_832;      // 64 * 13
        mempools->pools[BLOCK_896_INDEX].block_size = BLOCK_896;      // 64 * 14
        mempools->pools[BLOCK_960_INDEX].block_size = BLOCK_960;      // 64 * 15
        mempools->pools[BLOCK_1024_INDEX].block_size = BLOCK_1024;     // 64 * 16
        mempools->pools[BLOCK_1152_INDEX].block_size = BLOCK_1152;     // 128 * 9
        mempools->pools[BLOCK_1280_INDEX].block_size = BLOCK_1280;     // 128 * 10
        mempools->pools[BLOCK_1408_INDEX].block_size = BLOCK_1408;     // 128 * 11
        mempools->pools[BLOCK_1536_INDEX].block_size = BLOCK_1536;     // 128 * 12
        mempools->pools[BLOCK_1664_INDEX].block_size = BLOCK_1664;     // 128 * 13
        mempools->pools[BLOCK_1792_INDEX].block_size = BLOCK_1792;     // 128 * 14
        mempools->pools[BLOCK_1920_INDEX].block_size = BLOCK_1920;     // 128 * 15
        mempools->pools[BLOCK_2048_INDEX].block_size = BLOCK_2048;     // 128 * 16
        mempools->pools[BLOCK_2304_INDEX].block_size = BLOCK_2304;     // 256 * 9
        mempools->pools[BLOCK_2560_INDEX].block_size = BLOCK_2560;     // 256 *10
        mempools->pools[BLOCK_2816_INDEX].block_size = BLOCK_2816;     // 256 * 11
        mempools->pools[BLOCK_3072_INDEX].block_size = BLOCK_3072;     // 256 * 12
        mempools->pools[BLOCK_3328_INDEX].block_size = BLOCK_3328;     // 256 * 13
        mempools->pools[BLOCK_3584_INDEX].block_size = BLOCK_3584;     // 256 * 14
        mempools->pools[BLOCK_3840_INDEX].block_size = BLOCK_3840;     // 256 * 15
        mempools->pools[BLOCK_4096_INDEX].block_size = BLOCK_4096;     // 256 * 16
        mempools->pools[BLOCK_4608_INDEX].block_size = BLOCK_4608;     // 512 * 9
        mempools->pools[BLOCK_5120_INDEX].block_size = BLOCK_5120;     // 512 * 10
        mempools->pools[BLOCK_5632_INDEX].block_size = BLOCK_5632;     // 512 * 11
        mempools->pools[BLOCK_6144_INDEX].block_size = BLOCK_6144;     // 512 * 12
        mempools->pools[BLOCK_6656_INDEX].block_size = BLOCK_6656;     // 512 * 13
        mempools->pools[BLOCK_7168_INDEX].block_size = BLOCK_7168;     // 512 * 14
        mempools->pools[BLOCK_7680_INDEX].block_size = BLOCK_7680;     // 512 * 15
        mempools->pools[BLOCK_8192_INDEX].block_size = BLOCK_8192;     // 512 * 16
    } 
    for(index=0; index<POOL_SIZE; index++)
    {
        char* start = NULL;
        int block_size_align = 0;
        memblock_t* block = NULL;
        if(mempools->pool_capability[index] == 0 || mempools->chunk_capability[index] == 0)
        {
            mempools->pools[index].pos_mask = 0;
            mempools->pools[index].chunks = NULL;
                   
            continue;
        }
        // consider of cache line size is 64
        if(blockAlign + mempools->pools[index].block_size <= 32)
        {
            block_size_align = (blockAlign + mempools->pools[index].block_size + 31)/32 * 32; 
        }
        else
        {
            block_size_align = (blockAlign + mempools->pools[index].block_size + 63)/64 * 64;
        }
        {
            int chunkindex = 0;
            unsigned int chunk_len = mempools->chunk_capability[index]; 
            unsigned int chunkmemory = block_size_align * chunk_len;
            unsigned int chunk_size = (mempools->pool_capability[index]/chunk_len);
            memchunk_t* chunk = NULL;

            chunk_size = (((chunk_size==0) ? 1 : chunk_size));
            mempools->pools[index].pos_mask = (1 << logfloor(chunk_size)) -1;

            chunk = (memchunk_t*)memalign(64, chunk_struct_size * chunk_size);
            mempools->pools[index].chunks = (memchunk_t**)memalign(64, sizeof(memchunk_t*) * chunk_size);
            
            for(chunkindex=0; chunkindex<chunk_size; chunkindex++)
            {

                chunk->pool = &mempools->pools[index];
                chunk->memQ = memQinit(logfloor(chunk_len));                
                chunk->init_block_counter = chunk_len;
                chunk->free_block_counter = chunk_len;

                start = memalign(64, chunkmemory);
                chunk->block_addr_start = start;
                for(blockindex=0; blockindex<chunk_len-1; blockindex++)
                {
                    block = (memblock_t*)(start+(blockindex * block_size_align));
                    block->malloc_flag = 0;
                    block->block_size = mempools->pools[index].block_size;
                    //block->memQ = chunk->memQ;
                    block->chunk = chunk;
                    mempush(chunk->memQ, block);
                }
                
                mempools->pools[index].chunks[chunkindex] = chunk;
                chunk = (memchunk_t*)((char*)chunk + chunk_struct_size);
            }
        }
    }
    return 0;
}



#if 0
__attribute((always_inline))  static inline int align(unsigned int mem_size)
{
    int n = logfloor(mem_size);
    return ( (mem_size + (1<<(n-3)) -1) >> (n-3) ) << (n-3);
    //return ((1 << logfloor(mem_size))) >> 3;
}
#endif

__attribute((always_inline))  static inline  int found(unsigned int block_size)
{
    int pos = 0;
    switch (block_size)
    {
        case BLOCK_8:
            pos = BLOCK_8_INDEX;
            break;
        case BLOCK_16:
            pos = BLOCK_16_INDEX;
            break;
        case BLOCK_32:
            pos = BLOCK_32_INDEX;
            break;
        case BLOCK_48:
            pos = BLOCK_48_INDEX;
            break;
        case BLOCK_64:
            pos = BLOCK_64_INDEX;
            break;
        case BLOCK_80:
            pos = BLOCK_80_INDEX;
            break;
        case BLOCK_96:
            pos = BLOCK_96_INDEX;
            break;
        case BLOCK_112:
            pos = BLOCK_112_INDEX;
            break;
        case BLOCK_128:
            pos = BLOCK_128_INDEX;
            break;
        case BLOCK_144:
            pos = BLOCK_144_INDEX;
            break;
        case BLOCK_160:
            pos = BLOCK_160_INDEX;
            break;
        case BLOCK_176:
            pos = BLOCK_176_INDEX;
            break;
        case BLOCK_192:
            pos = BLOCK_192_INDEX;
            break;
        case BLOCK_208:
            pos = BLOCK_208_INDEX;
            break;
        case BLOCK_224:
            pos = BLOCK_224_INDEX;
            break;
        case BLOCK_240:
            pos = BLOCK_240_INDEX;
            break;
        case BLOCK_256:
            pos = BLOCK_256_INDEX;
            break;
        case BLOCK_288:
            pos = BLOCK_288_INDEX;
            break;
        case BLOCK_320:
            pos = BLOCK_320_INDEX;
            break;
        case BLOCK_352:
            pos = BLOCK_352_INDEX;
            break;
        case BLOCK_384:
            pos = BLOCK_384_INDEX;
            break;
        case BLOCK_416:
            pos = BLOCK_416_INDEX;
            break;
        case BLOCK_448:
            pos = BLOCK_448_INDEX;
            break;
        case BLOCK_480:
            pos = BLOCK_480_INDEX;
            break;
        case BLOCK_512: 
            pos = BLOCK_512_INDEX;
            break;
        case BLOCK_576:
            pos = BLOCK_576_INDEX;
            break;
        case BLOCK_640:
            pos = BLOCK_640_INDEX;
            break;
        case BLOCK_704:
            pos = BLOCK_704_INDEX;
            break;
        case BLOCK_768:
            pos = BLOCK_768_INDEX;
            break;
        case BLOCK_832:
            pos = BLOCK_832_INDEX;
            break;
        case BLOCK_896:
            pos = BLOCK_896_INDEX;
            break;
        case BLOCK_960:
            pos = BLOCK_960_INDEX;
            break;
        case BLOCK_1024:
            pos = BLOCK_1024_INDEX;
            break;
        case BLOCK_1152:
            pos = BLOCK_1152_INDEX;
            break;
        case BLOCK_1280:
            pos = BLOCK_1280_INDEX;
            break;
        case BLOCK_1408:
            pos = BLOCK_1408_INDEX;
            break;
        case BLOCK_1536:
            pos = BLOCK_1536_INDEX;
            break;
        case BLOCK_1664:
            pos = BLOCK_1664_INDEX;
            break;
        case BLOCK_1792:
            pos = BLOCK_1792_INDEX;
            break;
        case BLOCK_1920:
            pos = BLOCK_1920_INDEX;
            break;
        case BLOCK_2048:
            pos = BLOCK_2048_INDEX;
            break;
        case BLOCK_2304:
            pos = BLOCK_2304_INDEX;
            break;
        case BLOCK_2560:
            pos = BLOCK_2560_INDEX;
            break;
        case BLOCK_2816:
            pos = BLOCK_2816_INDEX;
            break;
        case BLOCK_3072:
            pos = BLOCK_3072_INDEX;
            break;
        case BLOCK_3328:
            pos = BLOCK_3328_INDEX;
            break;
        case BLOCK_3584:
            pos = BLOCK_3584_INDEX;
            break;
        case BLOCK_3840:
            pos = BLOCK_3840_INDEX;
            break;
        case BLOCK_4096:
            pos = BLOCK_4096_INDEX;
            break;
        case BLOCK_4608:
            pos = BLOCK_4608_INDEX;
            break;
        case BLOCK_5120:
            pos = BLOCK_5120_INDEX;
            break;
        case BLOCK_5632:
            pos = BLOCK_5632_INDEX;
            break;
        case BLOCK_6144:
            pos = BLOCK_6144_INDEX;
            break;
        case BLOCK_6656:
            pos = BLOCK_6656_INDEX;
            break;
        case BLOCK_7168:
            pos = BLOCK_7168_INDEX;
            break;
        case BLOCK_7680:
            pos = BLOCK_7680_INDEX;
            break;
        case BLOCK_8192:
            pos = BLOCK_8192_INDEX;
            break;
    }
    return pos;
}


__attribute((always_inline)) static inline unsigned int size_to_class(unsigned int mem_size)
{

    if(mem_size <= 8) // align with 8 byte
    {
        return 8;
    }
    else if(mem_size <= 256)// align with 16
    {
        return (((mem_size+15) >> 4)  << 4);
    }
    else if(mem_size <= 512)    // 32
    {
        return (((mem_size+31) >> 5)  << 5);
    }
    else if(mem_size <= 1024)  // 64
    {
        return (((mem_size+63) >> 6)  << 6);   
    }
    else if(mem_size <= 2048)  // 128
    {
        //return align(mem_size);
        return (((mem_size+127) >> 7)  << 7);
    }
    else if(mem_size <= 4096) // 256
    {
        return (((mem_size+255) >> 8) << 8);
    }
    else if(mem_size <= 8192) // 512
    {
        return (((mem_size+511) >> 9) << 9);
    }
    else
    {
        //printf("---mem_size is %d, out of mempool configure \n", mem_size);
        return BIGMEMORYFLAG;
    }
}

__attribute((always_inline)) static  inline void* mallocMemory(unsigned int mem_size)
{
    unsigned int size = blockAlign + mem_size;
    memblock_t* block = (memblock_t*)__real_malloc(size);
    block->malloc_flag = 1; // lazy add, no pos
    block->block_size = mem_size;
    block->chunk = NULL;
    //__sync_add_and_fetch(&big_mem_counter, 1);
    return (void*)((char*)block + blockAlign);
}

__attribute((always_inline)) static inline 
void* getMemoryFromPool(mempool_t* pool, unsigned int mem_size)
{
    if(pool->chunks)
    {
        //unsigned long pos = __sync_fetch_and_add(&pool->pos_mask, 0x100000000);
        static __thread unsigned long random = 0;
        //unsigned long random = (unsigned long)&random;
        memchunk_t* chunk = pool->chunks[pool->pos_mask & random++];  
        char* using = (char*)mempop(chunk->memQ);
        if(using)
        {
            //__sync_sub_and_fetch(&chunk->free_block_counter,1);
            return (using + blockAlign);
        }
    }
    {
        // 
        //assert(NULL);
        return mallocMemory(mem_size);
    }
}


void* getMemory(unsigned int mem_size, MemPools* mempools) 
{
    unsigned int block_size ;
    int index;
 
    if(mem_size == 0)
    {
        return NULL;
    }

    if(mem_size <= BLOCK_8192)
    {
        block_size = size_to_class(mem_size);  
        index = found(block_size);
    }
    else
    {
        return mallocMemory(mem_size);
    }
    return getMemoryFromPool(&mempools->pools[index], mem_size);
}


void freeMemory(void* ptr)
{
    memblock_header* blockheader;
    if(ptr == NULL)
    {
        return;
    }
    blockheader = (memblock_header*)((char*)ptr-blockAlign);
    if(0 == blockheader->malloc_flag)
    {
        //__sync_add_and_fetch(&blockheader->chunk->free_block_counter,1);
        mempush(blockheader->chunk->memQ, blockheader);
    }
    else
    {
        __real_free(blockheader);
        return;
    }
}



void* reallocMemory(void *ptr, size_t size, MemPools* mempools)
{
       char* tmp = (char*)ptr;
       memblock_t* block ;

       if(ptr == NULL)
       {
           if(size == 0)
           {
               return NULL;
           }
           tmp = (char*)getMemory(size, mempools);
           return tmp;
       }
       if(size == 0)
       {
           freeMemory(ptr);
           ptr = NULL;
           return NULL;
       }

       block = (memblock_t*)(tmp-blockAlign);
       if(size > block->block_size)
       {    
           tmp = (char*)getMemory(size, mempools);
           if(tmp)
           {
              //memset(tmp, 0, size);
              memcpy(tmp, ptr, block->block_size);
           }
           freeMemory(ptr);
           ptr = NULL;
           return tmp;
       }
       else
       {
           return ptr;
       }
}


void* callocMemory(unsigned int n, unsigned int size, MemPools* mempools)
{
    void* ptr = NULL;
    if(!mempools)
    {
        return NULL;
    }
    ptr = getMemory(n*size, mempools);
    if(ptr)
    {
        memset(ptr, 0, n*size);
    }
    return ptr;
}

void memory_info(char* buffer, int buflen, MemPools* mempools)
{
    int index = 0;
    int len = 0;
    for(index=0; index<POOL_SIZE; index++)
    {
        if(len >= buflen)
        {
            break;
        }
        if(mempools->pools[index].chunks)
        {
            int i = 0;
            for(i=0; i<=mempools->pools[index].pos_mask; i++)
            {
                snprintf(&buffer[len], buflen-len, "[%lu-size] pool, chunk[%d],  free %lu, init %u \n", mempools->pools[index].block_size, i,
                         memsize(mempools->pools[index].chunks[i]->memQ), 
                         mempools->pools[index].chunks[i]->init_block_counter);
                len = strlen(buffer);
            }
        }
        else
        {
            snprintf(&buffer[len], buflen-len, "[%lu-size] pool,  not init \n", mempools->pools[index].block_size);
        }
        len = strlen(buffer);
    }
}

#ifdef UT_TEST
int isMemoryclear(MemPools* mempools)
{
    int index = 0;
    assert(mempools->pools);
    for(index=0; index<POOL_SIZE; index++)
    {
       //if(mempools->pools[index].chunk_head->total_block_counter != mempools->pools[index].chunk_head->free_block_counter)
       //{
       //    return 0;
       //}
    }
    return 1;
}
#endif




