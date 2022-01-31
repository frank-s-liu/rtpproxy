#include "cmdSessionManager.h"
#include "cmdSession.h"
#include "hash.h"

#include <errno.h>


static unsigned long CMD_PING = BKDRHash("ping", 4);
static unsigned long CMD_OFFER = BKDRHash("offer",5);
static unsigned long CMD_ANSWER = BKDRHash("answer",6);
static unsigned long CMD_DELETE = BKDRHash("delete",6);
static unsigned long CMD_QUERY = BKDRHash("query",5);


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

int CmdSessionManager::processCmd()
{
    return 0;
}

