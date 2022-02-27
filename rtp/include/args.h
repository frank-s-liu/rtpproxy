#ifndef _RTPPROXY_ARGS_H__
#define _RTPPROXY_ARGS_H__

#include "cmdSession.h"

class Args
{
    
};

class PingCheckArgs : public Args
{
public:
    virtual ~PingCheckArgs(){}; // don't need to delete cm here

public:
    unsigned long      ping_recv_count;
    CmdSession*        cs;
};

class PipeTimerEventArgs
{
    Args*                   args_data;
    unsigned char           event_type;
    virtual ~PipeTimerEventArgs()
    {
        delete args_data;
    }
};

#endif
