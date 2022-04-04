#include "cmdSessionManager.h"
#include "cmdSession.h"
#include "hash.h"
#include "log.h"

#include <errno.h>


static unsigned long CMD_PING = BKDRHash("ping", 4);
static unsigned long CMD_OFFER = BKDRHash("offer",5);
static unsigned long CMD_ANSWER = BKDRHash("answer",6);
static unsigned long CMD_DELETE = BKDRHash("delete",6);
static unsigned long CMD_QUERY = BKDRHash("query",5);


CmdSessionManager* CmdSessionManager::s_instance = NULL;

CmdSessionManager::CmdSessionManager()
{
}

CmdSessionManager::~CmdSessionManager()
{
    cdm_sessions_map::iterator ite;
    for (ite = m_cmdSessionsMap.begin(); ite != m_cmdSessionsMap.end(); )
    {
        delete ite->second;
        m_cmdSessionsMap.erase(ite++);
    }
}

CmdSessionManager* CmdSessionManager::getInstance()
{
    if(!s_instance)
    {
        s_instance = new CmdSessionManager();
    }
    return s_instance;
}

int CmdSessionManager::putinCmdSession(CmdSession* cs)
{
    SessionKey* key = cs->m_session_key;
    cdm_sessions_map::iterator iter = m_cmdSessionsMap.find(key);
    if(iter != m_cmdSessionsMap.end())
    {   
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "same session key found");
        return 1;
    }
    else 
    {
        m_cmdSessionsMap[key] = cs;
    }   
    return 0;
}
 
CmdSession* CmdSessionManager::getCmdSession(char* key)
{
    if(!key)
    {
        return NULL;
    }
    SessionKey* sk = new SessionKey(key);
    CmdSession* cs = getCmdSession(sk);
    delete sk;
    return cs;
}

CmdSession* CmdSessionManager::getCmdSession(SessionKey* sk)
{
    cdm_sessions_map::iterator iter = m_cmdSessionsMap.find(sk);
    if(iter != m_cmdSessionsMap.end())
    {
        return iter->second;
    }
    return NULL;
}

CmdSession* CmdSessionManager::popCmdSession(SessionKey* sk)
{
    CmdSession* cs = NULL;
    cdm_sessions_map::iterator iter = m_cmdSessionsMap.find(sk);
    if(iter != m_cmdSessionsMap.end())
    {
        cs = iter->second;
        m_cmdSessionsMap.erase(iter);
    }
    return cs;
}
