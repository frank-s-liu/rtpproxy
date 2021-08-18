#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "memory.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum hash_status
{
    HASH_OK = 0,
    HASH_FULL,
    HASH_EXIST,
    HASH_NULL,
    HASH_KEY_TO_LONG,
    HASH_NO_MEMORY
};

typedef struct hashtable_s hashtable_t;
typedef unsigned long (*hash_code)(unsigned long slots, void* key);
typedef int (*keycmp)(hashtable_t *h, void* key1, void* key2);

__attribute__((visibility("default"))) hashtable_t* hashtab_create(hash_code hc, keycmp kc, unsigned long slots, MemPools* module_pools);
__attribute__((visibility("default"))) int hashtab_insert(hashtable_t *h, void *key, void *datum);
__attribute__((visibility("default"))) void* hashtab_search(hashtable_t* h, void* key);
__attribute__((visibility("default"))) void hashtab_remove(hashtable_t* h, void* key);
__attribute__((visibility("default"))) void hashtab_destroy(hashtable_t* h);
__attribute__((visibility("default"))) long BKDRHash(const char* str, int len);

#ifdef __cplusplus
}
#endif


#endif
