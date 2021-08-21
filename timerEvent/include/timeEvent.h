#ifndef __TIME_EVENT__H_
#define __TIME_EVENT__H_

class TimeEvent
{
public:
    TimeEvent()
    {
        pos = 0;
        type = 0;
        duration = 0;
        event_id = 0;
        args =NULL;
        cb = NULL;
    }
    virtual ~TimeEvent()
    {};
    unsigned long pos; 
    unsigned long event_id;
    unsigned int type;
    unsigned int duration; // ms
    void* args;
    void (*cb)(void* e);
public:
    bool operator < (const TimeEvent& tb) const
    {
        return pos < tb.pos;
    }
};

class TimeEventCmp
{
public:
    bool operator()(const TimeEvent* t1, const TimeEvent* t2)
    {
        return *t1 < *t2;
    }
};

int fireTimeEvent(TimeEvent* event);
static const int EPOLL_LISTEN_CNT = 4;
static const int EPOLL_LISTEN_TIMEOUT = 20;
static const unsigned int TIME_MS_FD_TYPE = 0;
static const unsigned int PIPE_TYPE = 3;

#endif
