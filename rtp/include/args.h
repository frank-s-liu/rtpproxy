#ifndef _RTPPROXY_ARGS_H__
#define _RTPPROXY_ARGS_H__


#include <stdio.h>

enum pipe_event_type
{
    RTP_TIMER_EVENT = 0,
    RTP_SEND_CMD_EVENT,
    RTP_EVENT_MAX_TYPE
};

class Args
{
public:
    virtual ~Args(){};
    virtual int processCmd() = 0;
};

class PingCheckArgs : public Args
{
public:
    PingCheckArgs(char* cs_key, int len);
    virtual ~PingCheckArgs();
    virtual int processCmd();

public:
    unsigned long      ping_recv_count;
    char*              cs_cookie;
};

class SendCMDArgs : public Args
{
public:
    SendCMDArgs(char* cs_key, int len);
    virtual ~SendCMDArgs();
    virtual int processCmd();

public:
    char*              cs_cookie;
};

class PipeEventArgs
{
public:
    virtual ~PipeEventArgs();

public:
    Args*                   args_data;
};

#endif
