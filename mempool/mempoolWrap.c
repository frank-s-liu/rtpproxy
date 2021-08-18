#include "memory.h"
#include "mempool.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern void __real_free(void* ptr);
extern void* __real_malloc(size_t size);

MemPools global_module_pools;

__attribute__((constructor)) static void init_global_pool()
{
    int index = 0;
    memset(&global_module_pools, 0, sizeof(MemPools));
    for(index=0; index<POOL_SIZE; index++)
    {
        if(index < 32)
        {
            global_module_pools.pool_capability[index] = 1024*32;
            global_module_pools.chunk_capability[index] = CHUNK_4096;
        }
        else 
        {
            global_module_pools.pool_capability[index] = 256;
            global_module_pools.chunk_capability[index] = CHUNK_512;
        } 
    }

     global_module_pools.pool_capability[0] = 1024*512;
     global_module_pools.chunk_capability[0] = CHUNK_131072;

     global_module_pools.pool_capability[1] = 1024*512;
     global_module_pools.pool_capability[2] = 1024*1024;
     global_module_pools.pool_capability[3] = 1024*512;
     global_module_pools.pool_capability[4] = 1024*512;
     global_module_pools.pool_capability[5] = 1024*256;
     global_module_pools.pool_capability[6] = 1024*128;
     global_module_pools.pool_capability[7] = 1024*64;
     global_module_pools.pool_capability[8] = 1024*128;
     global_module_pools.pool_capability[13] = 1024*128;
     global_module_pools.pool_capability[17] = 1024*128;
     global_module_pools.pool_capability[20] = 1024*16;
     global_module_pools.pool_capability[21] = 1024*16;
     global_module_pools.pool_capability[25] = 1024*128;
     global_module_pools.pool_capability[26] = 1024*16;
     global_module_pools.pool_capability[27] = 1024*16;
     global_module_pools.pool_capability[29] = 1024*16;
     global_module_pools.pool_capability[30] = 1024*64;
    
     global_module_pools.pool_capability[32] = 512*16;
     global_module_pools.pool_capability[33] = 512*4;
     global_module_pools.pool_capability[34] = 512*4;
     global_module_pools.pool_capability[35] = 512*8;
     global_module_pools.pool_capability[36] = 512*2;
     global_module_pools.pool_capability[37] = 512*2;
     global_module_pools.pool_capability[39] = 512*2;
     global_module_pools.pool_capability[40] = 512*4;
 


    initMemoryPool(&global_module_pools);
}

void* __wrap_malloc(size_t size)
{
    //memblock_t* block = NULL;
    char* tmp = (char*)getMemory(size, &global_module_pools);
    //block = *(memblock_t**)(tmp-infoAlign);
    //snprintf(block->stack, 16, "%p", __builtin_return_address(0));
    return (void*) tmp;
}

void __wrap_free(void* ptr)
{
    freeMemory(ptr);
}

void* __wrap_realloc(void* prt, size_t size)
{
    return reallocMemory(prt, size, &global_module_pools);
}

void* __wrap_calloc(unsigned int n, unsigned int size)
{
    return callocMemory(n, size, &global_module_pools);
}

char* __wrap_strdup(const char *s)
{
    size_t len = 0;
    char* new = NULL;
    if(!s)
    {
        return new;
    }
    len = strlen(s) + 1;
    new = getMemory(len, &global_module_pools);
    memcpy(new, s, len-1);
    new[len-1] = '\0';
    return new;
}

char* __wrap_strndup(const char *s, size_t len_c)
{
    size_t len = 0;
    char* new = NULL;
    if(!s)
    {
        return new;
    }
    len = strlen(s);
    if(len > len_c)
    {
        len = len_c +1; 
    }
    else
    {
        len = len+1;
    }
    new = getMemory(len, &global_module_pools);
    memcpy(new, s, len-1);
    new[len-1] = '\0';
    return new;
}

int __wrap_vasprintf(char **strp, const char *fmt, va_list ap)
{
     char* buf = NULL;
     int len = 0;
     size_t buflen = 0;
     va_list ap2;
     char* tmp = NULL;

     va_copy(ap2, ap);
     len = vsnprintf(tmp, 0, fmt, ap2);
     if (len > 0 && (buf = (char*)getMemory((buflen = (size_t) (len + 1)), &global_module_pools)) != NULL) 
     {
         len = vsnprintf(buf, buflen, fmt, ap);
         *strp = buf;
     } 
     else 
     {
         *strp = NULL;
         len = -1;
     }

     va_end(ap2);
     return len;
}
