#include "timerProcess.h"
#include "log.h"


#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>


TimerProcessor::TimerProcessor():Thread("eventprocess")
{
    m_fd_pipe[0] = -1;
    m_fd_pipe[1] = -1;
    if(pipe(m_fd_pipe) < 0)
    {
        assert(NULL);
    }
    m_event_q = initQ(QSIZE);
}

TimerProcessor::~TimerProcessor()
{
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

int TimerProcessor::pushTimeEvent(TimeEvent* event)
{
    if(!m_isStop)
    {
        if(push(m_event_q, event))
        {
            tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "timer event queue is full");
            return 1;
        }
        else
        {
            char buf[1] = {'a'};
            write(m_fd_pipe[1], buf, sizeof(buf));
            return 0;
        }
    }
    tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "timer process has exit");
    return 1;
}

void* TimerProcessor::run()
{
    struct epoll_event event;
    int ret = 0;
    int fd_cnt = 0;
    int ep_fd = -1;
    struct epoll_event events[EPOLL_LISTEN_CNT];
    ep_fd = epoll_create(EPOLL_LISTEN_CNT);
    assert(ep_fd >= 0);
    memset(&event, 0, sizeof(event));
    event.data.u32 = PIPE_TYPE;
    event.events = EPOLLIN;
    ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_fd_pipe[0], &event);
    if(ret < 0)
    {
        assert(0);
    }
    while(!m_isStop)
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
                if(type == PIPE_TYPE)
                {
                    char buf[1] = {1};
                    int len = read(m_fd_pipe[0], buf, sizeof(buf));
                    if(len > 0)
                    {
                        TimeEvent* event = NULL;
                        pop(m_event_q, (void**)&event);   
                        if(event)
                        {
                            event->cb(event->args);
                            delete event;
                        }
                        else
                        {
                            tracelog("TIMER", ERROR_LOG, __FILE__, __LINE__, "event is null ?");
                        }
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
                    tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "unkonwn issue, not pipe type");
                    continue;
                }
            }
            else
            {
                tracelog("TIMER", WARNING_LOG, __FILE__, __LINE__, "unkonwn issue?");
            }
        }
    }
    m_status = THREAD_STOPPED;
    return NULL;
}

