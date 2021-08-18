#include "timer.h"
#include "commonError.h"

#include <unistd.h> 
#include <sched.h>
#include <assert.h>

Timer::Timer(uint32_t interval, uint32_t  Initial)
{

    m_val.it_interval.tv_sec = interval / 1000;
    m_val.it_interval.tv_nsec = (interval % 1000) * 1000000;
    m_val.it_value.tv_sec = Initial / 1000;
    m_val.it_value.tv_nsec = (Initial % 1000) * 1000000;

    m_running = 0;
    m_tfd = -1;
    m_stoped = 0;
    m_args = NULL;
    m_cb = NULL;
    init();
}

Timer::~Timer()
{
    if(m_tfd >= 0)
    {
        close(m_tfd);
    }
    if(m_args)
    {
        //delete m_args;
    }
}

void Timer::setfun(CALLBACK_FN cb, void* args)
{
    m_cb = cb;
    m_args = args;
}

void Timer::start()
{
    m_running = 1;
    if (timerfd_settime(m_tfd, 0, &m_val, NULL) < 0)
    {
        close(m_tfd);
        m_tfd = -1;
        return;
    }
}

void Timer::stop()
{
    m_running = 0;
    while(!m_stoped)
    {
        sched_yield();
    }
}

void Timer::init()
{

    m_tfd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (m_tfd < 0) 
    {
        assert(NULL);
        return ;
    }
}

int32_t Timer::sleep()
{
    if(m_tfd != -1)
    {
        if (timerfd_settime(m_tfd, 0, &m_val, NULL) < 0)
        {
            close(m_tfd);
            m_tfd = -1;
            return FAILED;
        }
        uint64_t exp = 0;
        read(m_tfd, &exp, sizeof(uint64_t));
        return SUCCESS;
    }
    return FAILED;
}
int32_t Timer::sleep(unsigned int us)
{
    m_val.it_value.tv_sec = us / 1000000;
    m_val.it_value.tv_nsec = (us % 1000000) * 1000;
    return sleep();
}
