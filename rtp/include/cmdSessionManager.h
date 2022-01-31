#ifndef __CMD_SESSION_MANAGER_H__
#define __CMD_SESSION_MANAGER_H__

#include "cmdSession.h"
#include <map>

//typedef std::map<long, sipUser*> cmd_Sessions_map;

struct cmp_SessionKey
{
    bool operator()(const SessionKey* l, const SessionKey* r)
    {
        return (*l < *r);
    }
};

typedef std::map<SessionKey*, CmdSession*, cmp_SessionKey> cdm_sessions_map;

class CmdSessionManager
{
public:
    CmdSessionManager();
    virtual ~CmdSessionManager();
    int processCmd();
    void putinCmdSession(CmdSession* cs);
private:
    //int sendCmdResp(); 
   
private:
    cdm_sessions_map m_cmdSessionsMap;
};

#endif
