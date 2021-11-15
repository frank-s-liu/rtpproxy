#include "task.h"
#include "timerMeter.h"
#include "timerProcess.h"

#include <stdio.h>


static TimerMeter* s_tm = NULL;
//static TimerProcessor* s_tp = NULL;


void initTask()
{
    s_tm = new TimerMeter();
    s_tm->start();
    //s_tp = new TimerProcessor();
    //s_tp->start();
}


int add_task(unsigned int duration, void (*cb)(void* e), void* args)
{
    return s_tm->addTask(duration, cb, args);
}

//int pushTimeEvent(TimeEvent* event)
//{
//    return s_tp->pushTimeEvent(event);
//}               
