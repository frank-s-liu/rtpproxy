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
    virtual ~CmdSessionManager();
    int putinCmdSession(CmdSession* cs);
    CmdSession* getCmdSession(SessionKey* sk);
    CmdSession* getCmdSession(char* key);
    CmdSession* popCmdSession(SessionKey* sk);
    static CmdSessionManager* getInstance();
private:
    CmdSessionManager();
    //int sendCmdResp(); 
   
private:
    cdm_sessions_map m_cmdSessionsMap;
    static CmdSessionManager* s_instance;
};

#endif
