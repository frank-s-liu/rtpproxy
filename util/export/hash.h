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

__attribute__((visibility("default"))) unsigned long BKDRHash(const char* str, int len);

#ifdef __cplusplus
}
#endif


#endif
