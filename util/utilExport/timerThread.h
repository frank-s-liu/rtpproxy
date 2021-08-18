#ifndef _TIMER_THREAD_H
#define _TIMER_THREAD_H
#include "thread.h"
#include "timer.h"


class TimerThread : public Thread
{
public:
    TimerThread();
    TimerThread(const char* name);
    virtual ~TimerThread();
    virtual void* run();
    void addTimer(Timer* timer);    

private:
    void init();
    int m_epfd;
};

#endif
