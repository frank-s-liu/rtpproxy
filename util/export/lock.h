#ifndef COMMON_LOCK_H
#define COMMON_LOCK_H

#include <pthread.h>

class CriticalSection
{
public:
    CriticalSection();

    virtual ~CriticalSection();

    void lock();

    void unlock();

    pthread_mutex_t &getLock();

private:
    pthread_mutex_t m_mutex;

};

/// auto critical section
class AutoCritSec
{
public:
    explicit AutoCritSec(CriticalSection& cs); 

    virtual ~AutoCritSec();

    void unlock();

private:
    bool m_locked;
    CriticalSection& m_cs;

    AutoCritSec(const AutoCritSec& mtx);
    AutoCritSec& operator =(const AutoCritSec& mtx);

};
#endif
