#ifndef _RTPPROXY_ARGS_H__
#define _RTPPROXY_ARGS_H__

#include "cmdSession.h"
#include "cmdSessionManager.h"

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
    PingCheckArgs(char* cs_key, int len)
    {
        cs_cookie = new char[len+1];
        snprintf(cs_cookie, len+1, "%s", cs_key);
    }

    virtual ~PingCheckArgs()
    {
       if(cs_cookie)
       {
           delete[] cs_cookie;
           cs_cookie = NULL;
       }
    }
    virtual int processCmd()
    {
        CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(cs_cookie);
        if(cs)
        {
            return cs->process_cmd(cmdtype);
        }
        else
        {
            return -1;
        }
    }

public:
    unsigned long      ping_recv_count;
    char*              cs_cookie;
    unsigned char      cmdtype;
};

class PipeEventArgs
{
public:
    virtual ~PipeEventArgs()
    {
        if(args_data)
        {
            delete args_data;
            args_data = NULL;
        }
    }

public:
    Args*                   args_data;
    unsigned char           event_type;
};

#endif
