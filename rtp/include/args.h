#ifndef _RTPPROXY_ARGS_H__
#define _RTPPROXY_ARGS_H__


#include <stdio.h>

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
    //unsigned char      cmdtype;
};

class SessionKeyArgs : public Args
{
public:
    SessionKeyArgs(char* cs_key, int len);
    virtual ~SessionKeyArgs();
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
    unsigned char           event_type;
};

#endif
