#include "resource.h"
#include "memory.h"
#include "lockfreequeue_mutipush_one_pop.h"
#include "timer.h"
#include "logserver.h"

//std include
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

static MemPools logmempool;
typedef memqueue_s log_queue;
ResourceManager* ResourceManager::s_instance = NULL;
static log_queue* s_logqueue = NULL;
//static Timer s_pushtimer(0,1);
const int TIME_LEN = 23;

static char s_localtime[TIME_LEN+4] = {0};

static pthread_mutex_t s_mutex;
//static pthread_cond_t s_not_full;
static pthread_cond_t s_not_empty;
//static volatile  unsigned int full_waiter = 0;
static volatile unsigned int empty_waiter = 0; 



__attribute__((constructor)) static void init_log_pool() 
{
    memset(&logmempool, 0, sizeof(MemPools));
    logmempool.pool_capability[BLOCK_8_INDEX] = CHUNK_128;
    logmempool.chunk_capability[BLOCK_8_INDEX] = CHUNK_128;
    logmempool.pool_capability[BLOCK_16_INDEX] = CHUNK_128;
    logmempool.chunk_capability[BLOCK_16_INDEX] = CHUNK_128;
    logmempool.pool_capability[BLOCK_32_INDEX] = CHUNK_128;
    logmempool.chunk_capability[BLOCK_32_INDEX] = CHUNK_128;
    logmempool.pool_capability[BLOCK_48_INDEX] = CHUNK_128;
    logmempool.chunk_capability[BLOCK_48_INDEX] = CHUNK_128;

    logmempool.pool_capability[BLOCK_256_INDEX] = CHUNK_1024;
    logmempool.chunk_capability[BLOCK_256_INDEX] = CHUNK_256;
    logmempool.pool_capability[BLOCK_512_INDEX] = CHUNK_256;
    logmempool.chunk_capability[BLOCK_512_INDEX] = CHUNK_256;
    
    initMemoryPool(&logmempool);
    ResourceManager::getInstance();
}

ResourceManager* ResourceManager::getInstance()
{
    if(s_instance == NULL)
    {
        s_instance = new ResourceManager();
    }
    return s_instance;
}

ResourceManager::ResourceManager()
{
    s_logqueue = initQ(18);
    pthread_mutex_init(&s_mutex, NULL);
    //pthread_cond_init( &s_not_full, NULL);
    pthread_cond_init( &s_not_empty, NULL);
}

ResourceManager::~ResourceManager()
{
    // error
}

void* ResourceManager::operator new(size_t sz)
{
     return getMemory(sz, &logmempool);
}

void ResourceManager::operator delete(void* p)
{
     return freeMemory(p);
}

void* ResourceManager::getresource(unsigned int size)
{
    if(size <= BLOCK_256)
    {
        size = BLOCK_256;
    }
    else if(size <=BLOCK_512)
    {
        size = BLOCK_512;
    }
    else if(size <=BLOCK_1024)
    {
        size = BLOCK_1024;
    }
    else if(size <=BLOCK_2048)
    {
        size = BLOCK_2048;
    }
    return getMemory(size, &logmempool);
}

// muti-thread push
void ResourceManager::pushlog(void* data)
{
     if(0 == push(s_logqueue, data))
     {
         //pthread_mutex_lock(&s_mutex); don't using mutex is ok in this scenario
         if(empty_waiter)
         {
             //empty_waiter--;
             //pthread_mutex_unlock(&s_mutex);
             pthread_cond_signal(&s_not_empty);
         }
         else
         {
             //pthread_cond_signal(&s_not_empty);
         }
     }
}

// one thread pop
void* ResourceManager::poplog()
{
      void* log = NULL;
      int ret = 0;
      ret = pop(s_logqueue, &log);
      while(ret)
      {
          pthread_mutex_lock(&s_mutex);
          empty_waiter++;
          Logserver::getInstance()->flushcache();
          pthread_cond_wait(&s_not_empty, &s_mutex);
          empty_waiter--;
          pthread_mutex_unlock(&s_mutex);
          ret = pop(s_logqueue, &log);
          // assert(log); // log can be NULL, because push e1, e2 on position 1, 2, maybe 2 finished before 1
          updateTime();
      }
      memcpy((char*)log+24, s_localtime, TIME_LEN);
      return log;
}


void ResourceManager::freeresource(void* data)
{
    freeMemory(data);
}


void ResourceManager::updateTime()
{
    struct timeval tv;
    struct tm      timeinfo;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);
    snprintf(s_localtime, sizeof(s_localtime), "%04d-%02d-%02d %02d:%02d:%02d:%-3d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             (int)tv.tv_usec/1000);
                                
}

char* ResourceManager::logMemoryInfo()
{
    char* mem_info = new char[4096];
    mem_info[0] = '\0';
    memory_info(mem_info, 4096, &logmempool);
    return mem_info;
}
