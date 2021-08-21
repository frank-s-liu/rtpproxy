#ifndef __TIME_QUEUE__H_
#define __TIME_QUEUE__H_

#include "lockfreequeue_mutipush_one_pop.h"
#include "timeEvent.h"
/********************************************************************************
 * this file is used only in this module, it is not exported to other module
 * *******************************************************************************
 */

typedef memqueue_s TimeEventProcessQ;
typedef memqueue_s TimeEventPollQ;

static const int QSIZE = 16;
static const unsigned int TICK_PER_WHEEL = 1024;
static const unsigned int TICK_PER_WHEEL_MATH = 10;
static const unsigned int TICK_PER_WHEEL_MASK = TICK_PER_WHEEL-1;

extern int pushTimeEvent(TimeEvent* event);
#endif
