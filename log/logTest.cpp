#include "log.h"

//std include
#include <pthread.h>
#include <unistd.h> 
#include <assert.h>

void* threadEntry1(void* args)
{
    while(1)
    {
        static unsigned long index = 0;
        tracelog("TESTMODULE", INFO_LOG, __FILE__, __LINE__, "abc-1--[%ld] %s:%d", index, __FILE__,__LINE__);
        index++;
        if(index % 400 == 0)
        {
            usleep(2000);
        }
    }
    return NULL;
}
void* threadEntry2(void* args)
{
    while(1)
    {
        static unsigned long index = 0;
        tracelog("TESTMODULE", INFO_LOG, __FILE__, __LINE__, "abc-2--aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaassssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss[%ld] %s:%d",index, __FILE__,__LINE__);
        index++;
        if(index % 300 == 0)
        {
            usleep(2000);
        }
    }
    return NULL;
}
void* threadEntry3(void* args)
{
    while(1)
    {
        static unsigned long index = 0;
        tracelog("TESTMODULE", INFO_LOG, __FILE__, __LINE__,"abc-3-----------dddddddddddddddddddddsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssddddddddddddddddddddddddddddddddddddsssssssssssssssssssssssssssdddddddddddddddddddddddddddddddsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss-[%ld] %s:%d",index, __FILE__,__LINE__);
        index++;
        if(index % 200 == 0)
        {
            usleep(2000);
        }
    }
    return NULL;
}
void* threadEntry4(void* args)
{
    while(1)
    {
        static unsigned long index = 0;
        tracelog("TESTMODULE", INFO_LOG, __FILE__, __LINE__, "abc-4-cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc-[%ld] %s:%d",index, __FILE__,__LINE__);
        index++;
        if(index % 100 == 0)
        {
            usleep(20000);
        }
    }
    return NULL;
}

void* threadEntry5(void* args)
{
    while(1)
    {
        static unsigned long index = 0;
        tracelog("TESTMODULE", INFO_LOG, __FILE__, __LINE__, "abc-5--[%ld] %s:%d", index, __FILE__,__LINE__);
        index++;
        if(index % 400 == 0)
        {
            usleep(2000);
        }
    }
    return NULL;
}

int main()
{
    int ret = 0;
    initLog("./Mylog/"); 
    regiterLog("TESTMODULE", DEBUG_LOG);
    pthread_t thread_id1;
    pthread_t thread_id2;
    pthread_t thread_id3;
    pthread_t thread_id4;
    pthread_t thread_id5;
    ret = pthread_create(&thread_id1, NULL, threadEntry1, NULL );
    ret = pthread_create(&thread_id2, NULL, threadEntry2, NULL );
    ret = pthread_create(&thread_id3, NULL, threadEntry3, NULL );
    ret = pthread_create(&thread_id4, NULL, threadEntry4, NULL );
    ret = pthread_create(&thread_id5, NULL, threadEntry5, NULL );
    assert(ret == 0);
    pthread_join(thread_id1, NULL);
    return 0;
}
