#ifndef __TIMERPROCESSOR__H_
#define __TIMERPROCESSOR__H_

#include "thread.h"
#include "timeQueue.h"
#include "timeEvent.h"

class TimerProcessor : virtual public Thread
{
public:
    TimerProcessor();
    virtual ~TimerProcessor();
    virtual void* run();
    int pushTimeEvent(TimeEvent* event);

private:
    TimeEventProcessQ* m_event_q;
    int m_fd_pipe[2];
};

#endif
