#include "log.h"

#include "timerThread.h"
#include <sys/epoll.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>


static const int EPOLL_LISTEN_CNT = 10240;
static const int EPOLL_LISTEN_TIMEOUT = 500; //ms

TimerThread::TimerThread():Thread("Timer")
{
    init();
}

TimerThread::TimerThread(const char* name):Thread(name)
{
    init();
}

TimerThread::~TimerThread()
{

}

void* TimerThread::run()
{
    int i = 0;
    int fd_cnt = 0;
    struct epoll_event events[EPOLL_LISTEN_CNT];
 
    memset(events, 0, sizeof(events));
    while(!m_isStop) 
    {
        /* wait epoll event */
        fd_cnt = epoll_wait(m_epfd, events, EPOLL_LISTEN_CNT, EPOLL_LISTEN_TIMEOUT);
        if(0 == fd_cnt)
        {
            continue;
        }
        for(i = 0; i < fd_cnt; i++) 
        {
            if(events[i].events & EPOLLIN) 
            {
                Timer* timer = (Timer*)events[i].data.ptr;
                uint64_t exp = 0;
                read(timer->m_tfd, &exp, sizeof(uint64_t));
                if(exp > 1)
                {
                    tracelog("util", WARNING_LOG, __FILE__, __LINE__, "timer %d, expired for %u times ", timer->m_tfd, exp);
                }
                if(0 == timer->m_running || (0 == timer->m_val.it_interval.tv_sec && 0 == timer->m_val.it_interval.tv_nsec))
                {
                    if(epoll_ctl(m_epfd, EPOLL_CTL_DEL, timer->m_tfd, NULL) < 0)
                    {
                        // log
                        assert(NULL);
                    }
                    timer->m_cb(timer->m_args);
                    timer->m_stoped = 1;
                }
                else
                {
                    struct epoll_event event;
     
                    memset(&event, 0, sizeof(event));
                    event.data.ptr = (void*)timer;
                    event.events = EPOLLIN;
                    if(epoll_ctl(m_epfd, EPOLL_CTL_MOD, timer->m_tfd, &event) < 0)
                    {
                        //log
                        assert(NULL);
                    }
                    timer->m_cb(timer->m_args);
                }
            }
        }
    }
    m_status = THREAD_STOPPED;
    return NULL;
} 

void TimerThread::init()
{
    m_epfd = epoll_create(EPOLL_LISTEN_CNT); 
    assert(m_epfd >= 0);
}


// While one thread is blocked in a call to epoll_pwait(), it is
// possible for another thread to add a file descriptor to the waited-
// upon epoll instance.  If the new file descriptor becomes ready, it
// will cause the epoll_wait() call to unblock.
void TimerThread::addTimer(Timer* timer)
{
    int ret = 0;
    struct epoll_event event;
 
    memset(&event, 0, sizeof(event));
    event.data.ptr = (void*)timer;
    event.events = EPOLLIN;
 
    ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, timer->m_tfd, &event);
    if(ret < 0) 
    {
        // log
    }
}
