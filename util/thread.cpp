#include "thread.h"

// std include
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>
//static area
//
//

void* Thread::threadEntry(void* param)
{
    Thread* pthread = (Thread*) param;
    if(pthread->preStart())
    {
        if(pthread->m_namelen > 0)
        {
            prctl(PR_SET_NAME, pthread->m_thread_name);
        }
        if(DE_ATTACH == pthread->m_deAttach_state)
        {
             pthread_detach(pthread_self());
        }
        pthread->run();   
    }
    if(DE_ATTACH != pthread->m_deAttach_state)
    {
        pthread_exit(0);
    }
    pthread->m_status = THREAD_STOPPED;
    return NULL;
}

Thread::Thread()
{
    initThread();
}

Thread::Thread(int stackSize) // Byte
{
    initThread();
    m_stackSize = stackSize;
}

Thread::Thread(const char* name)
{
    initThread();
    if(NULL != name)
    {
        strncpy(m_thread_name, name, sizeof(m_thread_name)-1);
        m_namelen = strlen(m_thread_name);
    } 
}

Thread::~Thread()
{
    
}

bool Thread::start()
{
    int ret = -1;
    m_isStop = false;
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setstacksize(&attr, m_stackSize);
    ret = pthread_create(&m_thread_id, &attr, threadEntry, (void*)this );
    pthread_attr_destroy(&attr);
    return (ret == 0);
}

void Thread::stop(int millisecond)
{
    m_isStop = true;  // exit run()
    doStop();
    if(ATTACH == m_deAttach_state)
    {
        join(millisecond);
    }
}

bool Thread::join(int millisecond)
{
    if(m_thread_id == (pthread_t)-1)
    {
        return true;
    }
    if(millisecond <= 0)
    {
       return (0 ==  pthread_join(m_thread_id, NULL));
    }
    struct timespec  ts; 
    getTimeout( &ts, millisecond);
    return ( 0 == ::pthread_timedjoin_np(m_thread_id, NULL, &ts));
}

void Thread::getTimeout(struct timespec *spec, int timer)
{
    struct timeval current; 
    gettimeofday( &current, NULL );
    TIMEVAL_TO_TIMESPEC( &current, spec );
    spec->tv_sec = current.tv_sec + ((timer + current.tv_usec / 1000) / 1000);
    spec->tv_nsec = ((current.tv_usec / 1000 + timer) % 1000) * 1000000;    
}

bool Thread::preStart()
{
    return true;
}


void Thread::doStop()
{
    while(THREAD_STOPPED != m_status)
    {
        usleep(5000);
    }
}

void Thread::initThread()
{
    m_stackSize = 8*1024*1024; //8M
    memset(m_thread_name, 0, sizeof(m_thread_name));
    m_namelen = 0;
    m_deAttach_state = DE_ATTACH;
    m_thread_id = (pthread_t)-1; 
    m_isStop = true;
    m_status = THREAD_RUNNING;
}

void Thread::setAttachMode(int attach)
{
    m_deAttach_state = attach;
}
