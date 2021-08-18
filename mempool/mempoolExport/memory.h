#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" 
{
#endif 

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE  64
#endif 

typedef struct memblock memblock_t;
typedef struct memchunk memchunk_t;


typedef struct mempool 
{
   unsigned long block_size;               // block memory size: 8, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 256, 288, 320 ...
   unsigned long pos_mask;
   memchunk_t** chunks;

}mempool_t;

typedef enum _chunk_size
{
    CHUNK_128 = 128,
    CHUNK_256 = 256,
    CHUNK_512 = 512,
    CHUNK_1024 = 1024,
    CHUNK_1536 = 1536,
    CHUNK_2048 = 2048,
    CHUNK_2560 = 2560,
    CHUNK_3072 = 3072,
    CHUNK_3584 = 3584,
    CHUNK_4096 = 4096,
    CHUNK_8192 = 8192,
    CHUNK_16384 = 16384,
    CHUNK_32768 = 32768,
    CHUNK_65536 = 65536,
    CHUNK_131072 = 131072,
    CHUNK_262144 = 262144,
    CHUNK_524288 = 524288 
}chunk_size;

typedef enum _block_size_index
{
    BLOCK_8_INDEX = 0,
    BLOCK_16_INDEX,
    BLOCK_32_INDEX,
    BLOCK_48_INDEX,
    BLOCK_64_INDEX,
    BLOCK_80_INDEX,   // 5
    BLOCK_96_INDEX,
    BLOCK_112_INDEX,
    BLOCK_128_INDEX,
    BLOCK_144_INDEX,
    BLOCK_160_INDEX,  // 10
    BLOCK_176_INDEX,
    BLOCK_192_INDEX,
    BLOCK_208_INDEX,
    BLOCK_224_INDEX,
    BLOCK_240_INDEX, // 15
    BLOCK_256_INDEX,
    BLOCK_288_INDEX,
    BLOCK_320_INDEX,
    BLOCK_352_INDEX,
    BLOCK_384_INDEX,  // 20
    BLOCK_416_INDEX,
    BLOCK_448_INDEX,
    BLOCK_480_INDEX,
    BLOCK_512_INDEX,
    BLOCK_576_INDEX,  // 25
    BLOCK_640_INDEX,
    BLOCK_704_INDEX,
    BLOCK_768_INDEX,
    BLOCK_832_INDEX,
    BLOCK_896_INDEX, // 30
    BLOCK_960_INDEX,
    BLOCK_1024_INDEX,
    BLOCK_1152_INDEX,
    BLOCK_1280_INDEX,
    BLOCK_1408_INDEX, // 35
    BLOCK_1536_INDEX,
    BLOCK_1664_INDEX,
    BLOCK_1792_INDEX,
    BLOCK_1920_INDEX,
    BLOCK_2048_INDEX,  // 40
    BLOCK_2304_INDEX,
    BLOCK_2560_INDEX,
    BLOCK_2816_INDEX,
    BLOCK_3072_INDEX,
    BLOCK_3328_INDEX, // 45
    BLOCK_3584_INDEX,
    BLOCK_3840_INDEX,
    BLOCK_4096_INDEX,
    BLOCK_4608_INDEX,
    BLOCK_5120_INDEX, // 50
    BLOCK_5632_INDEX,
    BLOCK_6144_INDEX,
    BLOCK_6656_INDEX,
    BLOCK_7168_INDEX,
    BLOCK_7680_INDEX, // 55
    BLOCK_8192_INDEX,
    POOL_SIZE 
}block_size_index;

typedef enum _block_size
{
    BLOCK_8 = 8,
    BLOCK_16 = 16,
    BLOCK_32 = 32,
    BLOCK_48 = 48,
    BLOCK_64 = 64,
    BLOCK_80 = 80,
    BLOCK_96 = 96,
    BLOCK_112 = 112,
    BLOCK_128 = 128,
    BLOCK_144 = 144,
    BLOCK_160 = 160,
    BLOCK_176 = 176,
    BLOCK_192 = 192,
    BLOCK_208 = 208,
    BLOCK_224 = 224,
    BLOCK_240 = 240,
    BLOCK_256 = 256,
    BLOCK_288 = 288,
    BLOCK_320 = 320,
    BLOCK_352 = 352,
    BLOCK_384 = 384,
    BLOCK_416 = 416,
    BLOCK_448 = 448,
    BLOCK_480 = 480,
    BLOCK_512 = 512,
    BLOCK_576 = 576,
    BLOCK_640 = 640,
    BLOCK_704 = 704,
    BLOCK_768 = 768,
    BLOCK_832 = 832,
    BLOCK_896 = 896,
    BLOCK_960 = 960,
    BLOCK_1024 = 1024,
    BLOCK_1152 = 1152,
    BLOCK_1280 = 1280,
    BLOCK_1408 = 1408,
    BLOCK_1536 = 1536,
    BLOCK_1664 = 1664,
    BLOCK_1792 = 1792,
    BLOCK_1920 = 1920,
    BLOCK_2048 = 2048, 
    BLOCK_2304 = 2304,
    BLOCK_2560 = 2560,
    BLOCK_2816 = 2816,
    BLOCK_3072 = 3072,
    BLOCK_3328 = 3328,
    BLOCK_3584 = 3584,
    BLOCK_3840 = 3840,
    BLOCK_4096 = 4096,
    BLOCK_4608 = 4608,
    BLOCK_5120 = 5120, 
    BLOCK_5632 = 5632,
    BLOCK_6144 = 6144,
    BLOCK_6656 = 6656,
    BLOCK_7168 = 7168,
    BLOCK_7680 = 7680,
    BLOCK_8192 = 8192,
    BLOCK_9216 = 9216,
    BLOCK_10240 = 10240,
    BLOCK_11264 = 11264,
    BLOCK_12288 = 12288,
    BLOCK_13312 = 13312,
    BLOCK_14336 = 14336,
    BLOCK_15360 = 15360,
    BLOCK_16384 = 16384,
    BLOCK_18432 = 18432,
    BLOCK_20480 = 20480,
    BLOCK_22528 = 22528,
    BLOCK_24576 = 24576,
    BLOCK_26624 = 26624,
    BLOCK_28672 = 28672,
    BLOCK_30720 = 30720,
    BLOCK_32768 = 32768,
    BLOCK_36864 = 36864,
    BLOCK_40960 = 40960,
    BLOCK_45056 = 45056,
    BLOCK_49152 = 49152,
    BLOCK_53248 = 53248,
    BLOCK_57344 = 57344,
    BLOCK_61440 = 61440,
    BLOCK_65536 = 65536,
    BLOCK_73728 = 73728,
    BLOCK_81920 = 81920,
    BLOCK_90112 = 90112,
    BLOCK_98304 = 98304,
    BLOCK_106496 = 106496,  
    BLOCK_114688 = 114688,
    BLOCK_122880 = 122880,
    BLOCK_131072 = 131072
}block_size;

typedef struct mem_pools
{                       
    mempool_t* pools;
    unsigned int pool_capability[POOL_SIZE];
    chunk_size chunk_capability[POOL_SIZE];
}MemPools; 

typedef struct fs_memchunk_t fs_memchunk_s;

typedef struct fs_mempool_t
{
    unsigned int reuse;
    unsigned int chunk_size;
    fs_memchunk_s* chunk_start;
    fs_memchunk_s* chunk_current;
    fs_memchunk_s* max_chunk;
    struct fs_mempool_t* next;
    pthread_mutex_t* mutex;
}fs_mempool_s;


__attribute__((visibility("default")))  int  initMemoryPool(MemPools* mempools);
__attribute__((visibility("default"))) void* getMemory(unsigned int size, MemPools* mempools);
__attribute__((visibility("default"))) void  freeMemory(void* ptr);
__attribute__((visibility("default"))) void  destroy(MemPools* mempools);
__attribute__((visibility("default"))) void* reallocMemory(void *ptr, size_t size, MemPools* mempools);
__attribute__((visibility("default"))) void* callocMemory(unsigned int n, unsigned int size, MemPools* mempools);
__attribute__((visibility("default"))) void memory_info(char** info, MemPools* mempools);

__attribute__((visibility("default"))) void* realloc_memory(void *ptr, size_t size, fs_mempool_s* mempools);
__attribute__((visibility("default"))) fs_mempool_s* create_pool(unsigned long reuse, int needlock);
__attribute__((visibility("default"))) void destroy_pool(fs_mempool_s* pool);

#ifdef UT_TEST
__attribute__((visibility("default"))) int  isMemoryclear(MemPools* mempools); 
#endif

#ifdef __cplusplus
}
#endif 

#endif
