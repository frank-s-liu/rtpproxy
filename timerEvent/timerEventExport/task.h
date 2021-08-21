#ifndef __TIMER_TASK_H__
#define __TIMER_TASK_H__

#ifdef __cplusplus
extern "C"
{
#endif


void initTask();
int add_task(unsigned int duration, void (*cb)(void* e), void* args);

#ifdef __cplusplus
}
#endif

#endif
