#include "cmdSession.h"
#include "cmdSessionState.h"
#include "rtpControlProcess.h"
#include "hash.h"
#include "log.h"
#include "args.h"


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int parsingBencodeString(char* cmdstr, int* keylen, char** keystart)
{
    char* p = cmdstr;
    char* keyend = strchr(p, ':');
    if(keyend)
    {
        *keyend = '\0';
        *keylen = atoi(p);
        *keyend = ':';
        *keystart = keyend+1;
        return 0;
    }
    else
    {
        return -1;
    }
}


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
    rmSocketInfo();
    cdmParameters_map::iterator ite;
    for (ite = m_cmdparams.begin(); ite != m_cmdparams.end(); )
    {
        delete ite->second;
        m_cmdparams.erase(ite++);
    }
}

int CmdSession::sendPongResp()
{
    int len = m_session_key->m_cookie_len + 32;
    int ret = 0;
    char* pongresp = new char[len];
    snprintf(pongresp, len, "%s d6:result4:ponge", m_session_key->m_cookie);
    ret = sendcmd(pongresp);
    if(ret < 0 )
    {
        rmSocketInfo();
    }
    return ret;
}

int CmdSession::sendcmd(const char* cmdmsg)
{
    int ret = 0;
    int len = strlen(cmdmsg);
    if(m_socket_data)
    {
        if(m_sendmsgs_l.empty())
        {
            ret = m_socket_data->sendMsg(cmdmsg, len);
            if(ret == len)
            {
                ret = 0;
            }
            else if(ret <= 0)
            {
                if(errno == EAGAIN)
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "cmd session %s , send buffer full, send agin", m_session_key->m_cookie);
                    std::string* last_cmd = new std::string(cmdmsg);
                    m_sendmsgs_l.push_back(last_cmd);
                    ret = doAction2PrepareSend();
                    if(ret != 0)
                    {
                        rmSocketInfo();
                    }
                }
                else
                {
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session[%s] send cmd failed, de-attach socket info", m_session_key->m_cookie);
                    rmSocketInfo();
                    ret = -1;
                }
                goto retprocess;
            }
            else 
            { 
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unknow issue send msg len != real msg len, cmd session %s", m_session_key->m_cookie);
                rmSocketInfo();
                ret = -1;
                goto retprocess;
            }
        }
        else
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "cmd session %s , send buffer full, put msg to msg list agin", m_session_key->m_cookie);
            std::string* last_cmd = new std::string(cmdmsg);
            m_sendmsgs_l.push_back(last_cmd);
            ret = 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session[%s] has no socket info", m_session_key->m_cookie);
        ret = -1;
    }

retprocess:
    return ret;
}

int CmdSession::sendcmd(std::string* cmdstr)
{
    int ret = 0;
    int len = cmdstr->length();
    if(m_socket_data)
    {
        const char* cmdmsg = cmdstr->c_str();
        ret = m_socket_data->sendMsg(cmdmsg, len);
        if(ret == len)
        {
            ret = 0;
        }
        else if(ret <= 0)
        {
            if(errno == EAGAIN)
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "send cmd msg failed because of socket buffer, cmd session %s", m_session_key->m_cookie);
                m_sendmsgs_l.push_front(cmdstr);
                ret = 1;
            }
            else
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session[%s] send cmd failed, de-attach socket info, errno %d", m_session_key->m_cookie, errno);
                rmSocketInfo();
                ret = -1;
            }
            goto retprocess;
        }
        else
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session[%s] send cmd failed, send len != real len", m_session_key->m_cookie);
            ret = -1;
            rmSocketInfo();
            goto retprocess;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session[%s] has no socket info", m_session_key->m_cookie);
        ret = -1;
    }

retprocess:
    return ret;
}

int CmdSession::flushmsgs()
{
    int ret = 0;
    while(m_sendmsgs_l.size() > 0)
    {
        std::string* cmd = m_sendmsgs_l.front();
        m_sendmsgs_l.pop_front();
        ret = sendcmd(cmd);
        if(ret <= 0)
        {
            delete cmd;
        }
        if(ret != 0)
        {
            break;
        }
    }
    return ret==0? 0:-1;
}

int CmdSession::checkPingKeepAlive(PingCheckArgs* pingArg)
{
    int ret = m_css->checkPingKeepAlive(pingArg);
    if(ret != 0)
    {
        rmSocketInfo();
    }
    return ret;
}

int CmdSession::doAction2PrepareSend()
{
    int ret = 0;
    if(m_socket_data)
    {
        ret = m_socket_data->modify_write_event2Epoll(m_session_key);
        if(ret != 0)
        {
            rmSocketInfo();
        }
    }
    else
    {
        ret = -1;
    }
    return ret;
}

void CmdSession::setSocketInfo(Epoll_data* data)
{
    rmSocketInfo();
    m_socket_data = data;
    m_socket_data->m_session_count++;
}

void CmdSession::rmSocketInfo()
{
    if(m_socket_data)
    {
        m_socket_data->m_session_count --;
        if(0 == m_socket_data->m_session_count)
        {
            delete m_socket_data;
        }
        m_socket_data = NULL;
    }
}

int CmdSession::process_cmd(char* cmdstr)
{
    int cmd = 0;
    int cmdlen = strlen(cmdstr);
    tracelog("RTP", DEBUG_LOG,__FILE__, __LINE__, "cmd str is[%s]", cmdstr);
    parsingCmd(cmdstr, cmdlen);
    cmd = getCmd();
    return m_css->processCMD(cmd);
}

int CmdSession::process_cmd(int cmd)
{
    return m_css->processCMD(cmd);
}

int CmdSession::getCmd()
{
    std::string cmdkey("command");
    cdmParameters_map::iterator iter = m_cmdparams.find(cmdkey);
    if(iter != m_cmdparams.end())
    {
        std::string* cmd_v = iter->second;
        if(0 == cmd_v->compare(0,strlen("ping"), "ping"))
        {
            return PING_CMD;
        }
        else if(0 == cmd_v->compare(0,strlen("answer"), "answer"))
        {
            return ANSWER_CMD;
        }
        else if(0 == cmd_v->compare(0,strlen("offer"), "offer"))
        {
            return OFFER_CMD;
        }
        else if(0 == cmd_v->compare(0,strlen("delete"), "delete"))
        {
            return DELETE_CMD;
        }
        else if(0 == cmd_v->compare(0,strlen("PING"), "PING"))
        {
            return PING_CMD;
        }
        else if(0 == cmd_v->compare(0,strlen("ANSWER"), "ANSWER"))
        {
            return ANSWER_CMD;
        }
        else if(0 == cmd_v->compare(0,strlen("OFFER"), "OFFER"))
        {
            return OFFER_CMD;
        }
        else if(0 == cmd_v->compare(0,strlen("DELETE"), "DELETE"))
        {
            return DELETE_CMD;
        }
        else
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "unknown cmd, [%s]", cmd_v->c_str());
            return MAX_CONTROL_CMD;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "no cmd parameter");
        return MAX_CONTROL_CMD;
    }
}

int CmdSession::parsingCmd(char* cmdstr, int cmdlen)
{
    char* p = cmdstr;
    char* begin = p;
    int ret = 0;
    int parsingCmdLen = 0;
    while(p && *p!='\0' && parsingCmdLen>0 && parsingCmdLen<cmdlen)
    {
        int keylen = 0;
        ret = parsingBencodeString(p, &keylen, &begin);
        if(ret == 0)
        {
            std::string key(begin, keylen);
            p = begin+keylen;
            ret = parsingBencodeString(p, &keylen, &begin);
            if(ret == 0)
            {
                std::string* value = new std::string(begin, keylen);
                p = begin+keylen;
                cdmParameters_map::iterator iter = m_cmdparams.find(key);
                if(iter != m_cmdparams.end())
                {
                    std::string* oldvalue = iter->second;
                    tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "cmd session %s modify key %s old value[%s] to new balue [%s]", 
                                                                  m_session_key->m_cookie, key.c_str(), oldvalue->c_str(), value->c_str());
                    delete oldvalue;
                }
                m_cmdparams[key] = value;
            }
            else
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "paring cmd value failed, cmd [%s], value[%s]", cmdstr, p);
                break;
            }
        }
        else
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "paring cmd key failed, cmd [%s] key[%s]", cmdstr, p);
            break;
        }
        parsingCmdLen = p-cmdstr;
    }
    return ret;
}

int CmdSession::getCmdValueByStrKey(const char* key)
{
    int ret = 0;
//retprocess:
    return ret;
}


