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
    m_socket_data = NULL;
}

CmdSession::CmdSession(char* cookie)
{
    m_css = new CmdSessionInitState(this);
    m_session_key = new SessionKey(cookie);
    m_socket_data = NULL;
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
    if(m_socket_data)
    {
        if(m_socket_data->session_count > 1)
        {
            m_socket_data->session_count --;
        }
        else
        {
            delete m_socket_data;
        }
        m_socket_data = NULL;
    }
    if(m_cmd_str)
    {
        delete[] m_cmd_str;
        m_cmd_str = NULL;
    }
}

void CmdSession::setSocketInfo(Epoll_data* data)
{
    if(m_socket_data)
    {
        if(m_socket_data->session_count > 1)
        {
            m_socket_data->session_count --;
        }
        else
        {
            delete m_socket_data;
        }
        
    }
    m_socket_data = data;
}

void CmdSession::setCmdStr(const char* cmdStr)
{
    int len = strlen(cmdStr) +1;
    if(m_cmd_str)
    {
        delete m_cmd_str;
    }
    m_cmd_str = new char[len];
    snprintf(m_cmd_str, len, "%s", cmdStr);
}

int CmdSession::process_cmd()
{
    int cmd = 0;
    return m_css->processCMD(cmd);
}

int CmdSession::getCmdValueByStrKey(const char* key, int keylen)
{
    int ret = 0;
    char keytmp[160];
    if(m_cmd_str && keylen < 128)
    {
        snprintf(keytmp, sizeof(keytmp), "%d:%s", keylen, key);
        char* targetkey = strstr(m_cmd_str, keytmp);
        if(targetkey)
        {
        
        }
        else
        {
            ret = -1;
            goto retprocess;
        } 
    }
    else
    {
        ret = -1;
        goto retprocess;
    }

retprocess:
    return ret;
}


