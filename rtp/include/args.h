#ifndef _RTPPROXY_ARGS_H__
#define _RTPPROXY_ARGS_H__

#include "sdp.h"

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
};

class StateCheckArgs : public Args
{
public:
    StateCheckArgs(char* cs_key, int len);
    virtual ~StateCheckArgs();
    virtual int processCmd();
public:
    char*         cs_key; // call id
    unsigned char state;
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

class SDPArgs : public Args
{
public:
    SDPArgs(const char* call_id, int len);
    virtual ~SDPArgs();
    virtual int processCmd();
public:
    char*           call_id;
    Sdp_session*    sdp;   
    char            direction;
};

class PipeEventArgs
{
public:
    virtual ~PipeEventArgs();

public:
    Args*                   args_data;
};

#endif
