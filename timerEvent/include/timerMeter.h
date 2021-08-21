#ifndef __TIMERMETER__H_
#define __TIMERMETER__H_

#include "thread.h"
#include "timeEvent.h"
#include "timeQueue.h"

#include <set>

typedef std::multiset<TimeEvent*, TimeEventCmp> TimerEvents;

class TimerMeter : virtual public Thread
{
public:
    TimerMeter();
    virtual ~TimerMeter();
    virtual void* run();
    int addTask(unsigned int duration, void (*cb)(void* e), void* args, unsigned int event_id=0);
    virtual void doStop();

private:
   void initPipe();
   void initTimerMeter();
   void bindEvent2Meter(TimeEvent* event);

private:
   int m_tfd_ms;
   unsigned int m_timer_accurary; //ms
   int m_fd_pipe[2];
   void* m_events_add_Q;
   TimerEvents m_ms_meter[TICK_PER_WHEEL];
};

#endif
