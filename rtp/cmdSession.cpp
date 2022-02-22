#include "cmdSession.h"
#include "cmdSessionState.h"
#include "hash.h"


#include <string.h>
#include <stdio.h>

SessionKey::SessionKey(char* cookie)
{
    int len = strlen(cookie);
    m_cookie = new char[len+1];
    snprintf(m_cookie, len+1, "%s", cookie);
    m_cookie_id = BKDRHash(m_cookie, len);
    m_cookie_len = len;
}

SessionKey::~SessionKey()
{
    delete[] m_cookie;
    m_cookie = NULL;
    m_cookie_len = 0;
}

bool SessionKey::operator <(const SessionKey &k) const
{
    if (m_cookie_id < k.m_cookie_id) // primary key
    {
        return true;
    }
    else if (m_cookie_id == k.m_cookie_id) // second key
    {
        int ret = strncmp(m_cookie, k.m_cookie, m_cookie_len < k.m_cookie_len?m_cookie_len:k.m_cookie_len);
        if (ret < 0)
        {
            return true;
        }
    }
    return false;
}

CmdSession::CmdSession()
{
    m_css = new CmdSessionInitState(this);
    m_socketInfo = NULL;
}

CmdSession::CmdSession(char* cookie)
{
    m_css = new CmdSessionInitState(this);
    m_session_key = new SessionKey(cookie);
    m_socketInfo = NULL;
}

CmdSession::~CmdSession()
{
    if(m_css)
    {
        delete m_css;
        m_css = NULL;
    }
    if(m_session_key)
    {
        delete m_session_key;
        m_session_key = NULL;
    }
    if(m_socketInfo)
    {
        delete m_socketInfo;
        m_socketInfo = NULL;
    }
}

void CmdSession::setSocketInfo(SocketInfo* info)
{
    m_socketInfo = info;
}

int CmdSession::process_cmd(int cmd)
{
    return m_css->processCMD(cmd);
}
