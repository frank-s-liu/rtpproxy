/************************************************
 * note: this table is not thread safe 
 * 
 * 
 * *************************************************/

#include "log.h"
#include "memory.h"
#include "hash.h"

/**********************************************************************************************
 *
 *
          |------------------|        |------|-----|      |------|-----|      
          |  Hkey == 1       |------->|  key | val |----->|  key | val |----->NULL
          |----------------- |        |------|-----|      |------|-----|
          |  Hkey == 2       |------->NULL
          |------------------|
          |  Hkey == 3       |------->NULL
          |------------------|
*
*
*************************************************************************************************/

#define HASHTAB_MAX_NODES       0x2ffff



struct hashtab_node_s 
{
    void *key;
    void *datum;
    struct hashtab_node_s *next;
};


struct hashtable_s 
{
    struct hashtab_node_s** htable;                                   /* hash table */
    unsigned long slots;                                              /* number of slots in hash table */
    unsigned long size;                                               /* number of elements in hash table */
    hash_code hc;                                                     /* hash function */
    keycmp kc;                                                        /* key comparison function */
    MemPools* module_pools;                                           /* mempool pool*/
};

struct hashtable_s* hashtab_create(hash_code hc, keycmp kc, unsigned long slots, MemPools* module_pools)
{
    struct hashtable_s* p = NULL;
    unsigned long i = 0;
    
    if(NULL == module_pools)
    {
        tracelog("util", ERROR_LOG,__FILE__, __LINE__, "create hash table failed, no mempool", __FILE__,__LINE__);
        return NULL;
    }
    p = (struct hashtable_s*)getMemory(sizeof(struct hashtable_s), module_pools);
    if (p == NULL)
    {
        return NULL;
    }
    p->slots = slots;
    p->size = 0;
    p->hc = hc;
    p->kc = kc;
    p->module_pools = module_pools;
    p->htable = (struct hashtab_node_s**)getMemory(sizeof(struct hashtab_node_s*) * slots, module_pools);
    if(p->htable == NULL) 
    {
        tracelog("util", ERROR_LOG,__FILE__, __LINE__, "create hash table failed, canot get memory from  mempool", __FILE__,__LINE__);
        return NULL;
    }
 
    for(i = 0; i < slots; i++)
    {
        p->htable[i] = NULL;
    }
    return p;
}

int hashtab_insert(struct hashtable_s *h, void *key, void *datum)
{
    unsigned long hashcode = 0;
    struct hashtab_node_s* prev = NULL;
    struct hashtab_node_s* cur = NULL;
    struct hashtab_node_s* newnode = NULL;
 
    if(NULL == key || !h)
    {
        return HASH_NULL;
    }
    if (h->size == HASHTAB_MAX_NODES)
    {
        return HASH_FULL;
    }
    hashcode = h->hc(h->slots, key);
    prev = NULL;
    cur = h->htable[hashcode];
    while(cur && (h->kc(h, key, cur->key) != 0)) 
    {
        prev = cur;
        cur = cur->next;
    }
 
    if(cur && (h->kc(h, key, cur->key) == 0))
    {
        return HASH_EXIST;
    }
    newnode = (struct hashtab_node_s*)getMemory(sizeof(struct hashtab_node_s), h->module_pools);
    if(newnode == NULL)
    {
        tracelog("util", ERROR_LOG, __FILE__, __LINE__,"create hash table failed, canot get memory from  mempool", __FILE__,__LINE__);
        return HASH_NO_MEMORY;
    }
    newnode->key = key;
    newnode->datum = datum;
    if (prev) 
    {
        newnode->next = prev->next;
        prev->next = newnode;
    } 
    else 
    {
        newnode->next = h->htable[hashcode];
        h->htable[hashcode] = newnode;
    }
 
    h->size++;
    return 0;
}

void* hashtab_search(struct hashtable_s* h, void* key)
{
    unsigned long hcode = 0;
    struct hashtab_node_s* cur = NULL;
 
    if(!h)
    {
        return NULL;
    }
    hcode = h->hc(h->slots, key);
    cur = h->htable[hcode];
    while (cur != NULL && h->kc(h, key, cur->key) != 0)
    {
        cur = cur->next;
    }
    if (cur == NULL)
    {
        return NULL;
    }
    return cur->datum;
}

void hashtab_remove(struct hashtable_s* h, void* key)
{
    unsigned long hcode = 0;
    struct hashtab_node_s* cur = NULL;
    struct hashtab_node_s* prev = NULL;
    struct hashtab_node_s* next = NULL; 
    if(!h)
    {
        return ;
    }
    hcode = h->hc(h->slots, key);
    cur = h->htable[hcode];
    while (cur != NULL && h->kc(h, key, cur->key) != 0)
    {
        prev = cur; 
        cur = cur->next;
    }
    if (cur == NULL)
    {
        return ;
    }
    if(prev)
    {
        next = cur->next;
        prev->next = next;
    }
    else
    {
        h->htable[hcode] = cur->next;
    }
    h->size--;
    freeMemory(cur);
}

void hashtab_destroy(struct hashtable_s* h)
{
    unsigned long i = 0;
    struct hashtab_node_s* cur = NULL;
    struct hashtab_node_s* temp = NULL;
 
    if (!h)
    {
        return;
    }
    for (i = 0; i < h->slots; i++) 
    {
        cur = h->htable[i];
        while (cur != NULL) 
        {
            temp = cur;
            cur = cur->next;
            freeMemory(temp);
        }
        h->htable[i] = NULL;
    }
 
    freeMemory(h->htable);
    h->htable = NULL;
    h->module_pools = NULL;
    freeMemory(h);
    h = NULL;
}
