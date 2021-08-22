#ifndef __RTP_CONTROL_PROCESS_H__
#define __RTP_CONTROL_PROCESS_H__

#include "thread.h"

class ControlProcess : public Thread
{
public:
    ControlProcess();
    virtual ~ControlProcess();
    virtual void doStop();

private:
    

};

#endif
