#include "lock.h"


CriticalSection::CriticalSection()
{
    pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ::pthread_mutex_init(&m_mutex, &attr);
    ::pthread_mutexattr_destroy(&attr);
}

CriticalSection::~CriticalSection()
{
    ::pthread_mutex_destroy(&m_mutex);
}


void CriticalSection::lock()
{
    ::pthread_mutex_lock(&m_mutex);
}


void CriticalSection::unlock()
{
    ::pthread_mutex_unlock(&m_mutex);
}

pthread_mutex_t& CriticalSection::getLock()
{
    return m_mutex;
}



AutoCritSec::AutoCritSec(CriticalSection& cs) : m_locked(false), m_cs(cs)
{
    m_cs.lock();
    m_locked = true;
}

AutoCritSec::~AutoCritSec()
{
    unlock();
}

void AutoCritSec::unlock()
{
    if (m_locked)
    {
        m_cs.unlock();
        m_locked = false;
    }
}
