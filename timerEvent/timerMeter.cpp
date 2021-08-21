#include "timerMeter.h"
#include "timeQueue.h"
#include "log.h"


#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdio.h>


static const int TIME_ACCURARY = 16; // ms 
static const int TIME_ACCURARY_MATH = 4;


static    unsigned long ms_meter_pos = 0;

TimerMeter::TimerMeter():Thread("timeMeter") 
{
    m_timer_accurary = TIME_ACCURARY;
    initTimerMeter();
}

TimerMeter::~TimerMeter()
{
    if(m_tfd_ms >= 0)
    {
        close(m_tfd_ms);
        m_tfd_ms = -1;
    }
    
    if(m_fd_pipe[0] >= 0)
    {
        close(m_fd_pipe[0]);
        m_fd_pipe[0] = -1;
    }

    if(m_fd_pipe[1] >= 0)
    {
        close(m_fd_pipe[1]);
        m_fd_pipe[1] = -1;
    }
}

void TimerMeter::initTimerMeter()
{
    initPipe();
    m_events_add_Q = initQ(QSIZE);
}

void TimerMeter::initPipe()
{
    m_fd_pipe[0] = -1;
    m_fd_pipe[1] = -1;
    if(pipe(m_fd_pipe) < 0)
    {
        tracelog("TIMER", ERROR_LOG, __FILE__, __LINE__, "create pipe failed");
        exit(1);
    }
}

void TimerMeter::bindEvent2Meter(TimeEvent* event)
{
    if(!event)
    {
        tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "bind timer event error, event is null");
        return;
    }
    unsigned long pos_off = event->duration >> TIME_ACCURARY_MATH;
    unsigned long target_pos = pos_off + ms_meter_pos; 
    event->pos = target_pos;
    m_ms_meter[target_pos&TICK_PER_WHEEL_MASK].insert(event);
}

void* TimerMeter::run()
{
    struct epoll_event events[EPOLL_LISTEN_CNT];
    int fd_cnt = 0;
    int ep_fd = -1;
    int runagain = 0;
    // create time_fd
    {
        struct itimerspec val_ms;
        val_ms.it_interval.tv_sec = 0;
        val_ms.it_interval.tv_nsec = m_timer_accurary * 1000000;
        val_ms.it_value.tv_sec = 0;
        val_ms.it_value.tv_nsec = m_timer_accurary * 1000000;
        m_tfd_ms = timerfd_create(CLOCK_MONOTONIC, 0);
        assert(m_tfd_ms >= 0);
        
        if (timerfd_settime(m_tfd_ms, 0, &val_ms, NULL) < 0)
        {
            goto retpoint;
        }
 
    }
    memset(events, 0, sizeof(events));

    // create epoll fd
     {
        struct epoll_event event;
        int ret = 0;
        ep_fd = epoll_create(EPOLL_LISTEN_CNT);
        assert(ep_fd >= 0);
        
        memset(&event, 0, sizeof(event));
        event.data.u32 = TIME_MS_FD_TYPE;
        event.events = EPOLLIN;
        ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_tfd_ms, &event);
        if(ret < 0)
        {
            goto retpoint;
        }

        memset(&event, 0, sizeof(event));
        event.data.u32 = PIPE_TYPE;
        event.events = EPOLLIN;
        ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_fd_pipe[0], &event);
        if(ret < 0)
        {
            goto retpoint;
        }
    }
runpoint:
    while(!m_isStop || size((TimeEventPollQ*)m_events_add_Q))
    { 
        fd_cnt = epoll_wait(ep_fd, events, EPOLL_LISTEN_CNT, EPOLL_LISTEN_TIMEOUT);
        if(fd_cnt == 0)
        {
            continue;
        }
        for(int i = 0; i < fd_cnt; i++)
        {
            if(events[i].events & EPOLLIN)
            {
                unsigned int type = events[i].data.u32;
                if(type == TIME_MS_FD_TYPE)
                {
                    uint64_t exp = 0;
                    read(m_tfd_ms, &exp, sizeof(uint64_t));
                    if(exp > 1)
                    {
                        tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "timer event fd_ms timer %d, expired for %u times ", m_tfd_ms, exp);
                    }
                    while(exp>=1)
                    {
                        int position = ms_meter_pos & TICK_PER_WHEEL_MASK;
                        TimerEvents::iterator it = m_ms_meter[position].begin();
                        while(!m_ms_meter[position].empty())
                        {
                            it = m_ms_meter[position].begin();
                            if((*it)->pos == ms_meter_pos)
                            {
                                TimeEvent* event = *it;
                                int ret = pushTimeEvent(event);
                                if(0 != ret)
                                {
                                    tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "can not process task, process queue is full");
                                    event->duration = 64 + random()%1024;
                                    bindEvent2Meter(event);
                                }
                                m_ms_meter[position].erase(it);
                            }
                            else if((*it)->pos > ms_meter_pos)
                            {
                                break;
                            }
                            else
                            {
                                tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "missing timer click? it must not  come here");
                            }
                        }
                        ms_meter_pos++;
                        exp--;
                    }
                    continue;
                }
                else if(type == PIPE_TYPE)
                {
                    char buf[1] = {1};
                    int len = read(m_fd_pipe[0], buf, sizeof(buf));
                    if(len > 0)
                    {
                        TimeEvent* event = NULL;
                        pop((TimeEventPollQ*)m_events_add_Q, (void**)&event);
                        bindEvent2Meter(event);
                        continue;
                    }
                    else if(len == 0)
                    {
                        tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "unkonwn issue?");
                        continue;
                    }
                    else
                    {
                        // error occurs
                        assert(NULL);      
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                assert(NULL);
            }
        }
    }
    if(runagain == 0) // make sure TimeEventPollQ is empty to clear resource
    {
        runagain++;
        sleep(1);
        goto runpoint;
    }

retpoint:
    if(m_tfd_ms >= 0)
    {
        close(m_tfd_ms);
        m_tfd_ms = -1;
    }

    if(ep_fd>=0)
    {
        close(ep_fd);
        ep_fd = -1;
    }

    m_status = THREAD_STOPPED;
    return NULL;
}

/***************************************************************************************************
 *
 * if add task success, the args must be delete in call-back function
 * if failed, the call must delete the args itself
 *
 *
 ***************************************************************************************************/

int TimerMeter::addTask(unsigned int duration, void (*cb)(void* e), void* args, unsigned int event_id)
{
    if(duration < m_timer_accurary)
    {
        tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "can not support time accuracy %u", duration);
        return -1;
    }
    static unsigned long s_event_id = 0;
    if(!m_isStop)
    {
        TimeEvent* event = new TimeEvent();
        if(event_id > 0)
        {
            event->event_id = event_id;    
        }
        else
        {
            __sync_bool_compare_and_swap(&s_event_id, 0x7FFFFFFFFFFFFF00, 0);
            event->event_id = __sync_fetch_and_add(&s_event_id, 1);
        }
        event->duration = duration;
        event->cb = cb;
        event->args = args;
        if(push((TimeEventPollQ*)m_events_add_Q, event))
        {
            tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "task queue is full");
            delete event;
            return -1;
        }
        else
        {
            char buf[1] = {'a'};
            // POSIX.1-2001 says that write(2)s of less than PIPE_BUF bytes must be atomic:
            write(m_fd_pipe[1], buf, sizeof(buf));
        }
        return 0;   
    }
    else
    {
        // the caller must delete the memory args
        return -1;       
    }
}

void TimerMeter::doStop()
{

}
