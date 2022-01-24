#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include <sys/timerfd.h>

typedef void (*CALLBACK_FN)(void *);


// base timer class
class Timer
{
public:
    Timer(uint32_t interval, uint32_t  Initial);
    virtual ~Timer();
    
    virtual void start(); 
    virtual void stop();  // clean resource
    int32_t sleep();
    int32_t sleep(unsigned int us);
    void setfun(CALLBACK_FN cb, void* args);

public:
    int32_t  m_tfd;
    struct itimerspec m_val;
    uint8_t  m_running;      // running flag for 
    uint8_t  m_stoped;       // stoped by timerThread

    CALLBACK_FN m_cb;
    void* m_args;
private:
    Timer(const Timer& other);
    void init();
};




#endif
