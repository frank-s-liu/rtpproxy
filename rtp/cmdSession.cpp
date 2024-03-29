#include "cmdSession.h"
#include "cmdSessionState.h"
#include "rtpControlProcess.h"
#include "hash.h"
#include "log.h"
#include "args.h"
#include "sdp.h"
#include "rtpConstStr.h"

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
    else if(p && *p=='e')
    {
        *keylen = 0;
        *keystart = NULL;
        return 0;
    }
    else
    {
        return -1;
    }
}

SessionKey::SessionKey(const char* cookie)
{
    int len = strlen(cookie);
    m_cookie = new char[len+1];
    snprintf(m_cookie, len+1, "%s", cookie);
    m_cookie_id = BKDRHash(m_cookie, len);
    m_cookie_len = len;
}

SessionKey::SessionKey(const char* cookie, int len)
{
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

LastCookie::LastCookie(const char* cookie, int len)
{
    m_cookie.s = new char[len+1];
    m_cookie.len = len;
    snprintf(m_cookie.s, len+1, "%s", cookie);
    m_cookie_id = BKDRHash(m_cookie.s, len);
    m_resp.s = NULL;
    m_resp.len = 0;
}

LastCookie::~LastCookie()
{
    if(m_cookie.len)
    {
        delete[] m_cookie.s;
        m_cookie.s = NULL;
        m_cookie.len = 0;
    }
    if(m_resp.len)
    {
        delete[] m_resp.s;
        m_resp.s = NULL;
        m_resp.len = 0;
    }
}

CmdSession::CmdSession()
{
    m_css = new CmdSessionInitState(this);
    m_socket_data = NULL;
    m_last_cookie = NULL;
    m_cookie.len = 0;
    m_cookie.s = NULL;
    m_state_check_count = 0;
}

CmdSession::CmdSession(char* cookie)
{
    m_css = new CmdSessionInitState(this);
    m_session_key = new SessionKey(cookie);
    m_socket_data = NULL;
    m_last_cookie = NULL;
    m_cookie.s = NULL;
    m_cookie.len = 0;
    m_state_check_count = 0;
}

CmdSession::~CmdSession()
{
    if(m_css)
    {
        delete m_css;
        m_css = NULL;
    }
    rmSocketInfo();
    cdmParameters_map::iterator ite;
    for (ite = m_cmdparams.begin(); ite != m_cmdparams.end(); )
    {
        delete ite->second;
        m_cmdparams.erase(ite++);
    }

    MsgSend_l::iterator ite_l = m_sendmsgs_l.begin();
    for(; ite_l != m_sendmsgs_l.end(); )
    {
        if(*ite_l)
        {
            delete *ite_l;
        }
        else
        {
            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknow issue, cmd str need to send is null");
        }
        ite_l = m_sendmsgs_l.erase(ite_l);
    }
    if(m_last_cookie)
    {
        delete m_last_cookie;
        m_last_cookie = NULL;
    }
    if(m_session_key)
    {
        tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "destroy cmd session [%s] instance", m_session_key->m_cookie);
        delete m_session_key;
        m_session_key = NULL;
    }
    if(m_cookie.len)
    {
        delete[] m_cookie.s;
        m_cookie.s = NULL;
        m_cookie.len = 0;
    }
}

int CmdSession::sendPongResp()
{
    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "must not send Pong response  %s", m_session_key->m_cookie);
    return -1;
}

int CmdSession::sendcmd(const char* cmdmsg, int len)
{
    int ret = 0;
    if(m_socket_data && cmdmsg)
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
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session[%s] send cmd failed, de-attach socket info, errno [%d], ret [%d]", m_session_key->m_cookie, errno, ret);
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
        if(!m_socket_data)
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session[%s] has no socket info", m_session_key->m_cookie);
        }
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
    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "must not check pingpong for call session %s ", m_session_key->m_cookie);
    return -1;
}

int CmdSession::checkState(StateCheckArgs* stateArgs)
{
    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "must not check state for none call session %s ", m_session_key->m_cookie);
    return -1;
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
    char remote_ip[64];
    unsigned short remote_port;
    if(m_socket_data)
    {
        if(data == m_socket_data)
        {
            return;
        }
        else
        {
            rmSocketInfo();
        }
    }
    m_socket_data = data;
    m_socket_data->m_session_count++;
    data->m_socket->getRemoteAddress(remote_ip, sizeof(remote_ip));
    data->m_socket->getRemotePort(&remote_port);
    tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "setSocketInfo in cmd session [%s], connection with remote add[%s:%d], session counter[%d] ", 
                                                   m_session_key?m_session_key->m_cookie:"still no session key",
                                                   remote_ip, remote_port, m_socket_data->m_session_count);
}

void CmdSession::rmSocketInfo()
{
    if(m_socket_data)
    {
        char remote_ip[64];
        unsigned short remote_port;
        m_socket_data->m_socket->getRemoteAddress(remote_ip, sizeof(remote_ip));
        m_socket_data->m_socket->getRemotePort(&remote_port);
        m_socket_data->m_session_count --;
        tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "rmSocketInfo[%s:%d] in cmd session [%s], session counter %d ", 
                                                      remote_ip, remote_port, m_session_key?m_session_key->m_cookie:"still no session key", m_socket_data->m_session_count);
        if(0 == m_socket_data->m_session_count)
        {
            tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "delete Epoll_data info[%s:%d]  in cmd session [%s] ", 
                                                           remote_ip, remote_port, m_session_key?m_session_key->m_cookie:"still no session key");
            delete m_socket_data;
        }
        m_socket_data = NULL;
    }
}

int CmdSession::process_cmd(char* cmdstr)
{
    int cmd = 0;
    int cmdlen = strlen(cmdstr);
    int ret;
    parsingCmd(cmdstr, cmdlen);
    cmd = getCmd();
    CmdSessionState* css = NULL;
    //tracelog("RTP", DEBUG_LOG,__FILE__, __LINE__, "cmd session [%s] cookie[%s] is processing cmd [%s]", m_session_key->m_cookie,m_cookie.s, CMD_STR[cmd]);
    ret =m_css->processCMD(cmd, &css);
    if(0 == ret && css)
    {
        delete m_css;
        m_css = css;
    }
    tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "cmd session [%s] with cookie [%s] has processed cmd [%s], result is %s ", 
                                                  m_session_key->m_cookie, m_cookie.s, CMD_STR[cmd], ret==0?"SUCCESS":"FAILED");
    return ret;
}

int CmdSession::process_cmd(int cmd)
{
    CmdSessionState* css = NULL;
    tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "cmd session %s is processing cmd [%d]", m_session_key->m_cookie, cmd);
    int ret = m_css->processCMD(cmd, &css);
    if(0 == ret && css)
    {
        delete m_css;
        m_css = css;
    }
    tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "cmd session %s has processed cmd [%d], result is %s ", m_session_key->m_cookie, cmd, ret==0?"SUCCESS":"FAILED");
    return ret;
}

int CmdSession::processSdpResp(Sdp_session* sdp, RTPDirection direction)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "must not process SdpResp in CmdSession, call id %s", m_session_key->m_cookie);
    return -1;
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
    while(p && *p!='\0' && parsingCmdLen>=0 && parsingCmdLen<cmdlen)
    {
        int keylen = 0;
        ret = parsingBencodeString(p, &keylen, &begin);
        if(ret == 0 && keylen>0)
        {
            std::string key(begin, keylen);
            std::string* value = NULL;
            p = begin+keylen;
            if(*p>='0' && *p<='9')
            {
                ret = parsingBencodeString(p, &keylen, &begin);
                if(ret == 0)
                {
                    value = new std::string(begin, keylen);
                    p = begin+keylen;
                }
                else
                {
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "paring cmd value failed, cmd [%s], value[%s]", cmdstr, p);
                    goto retcode;
                }
            }
            else if(*p == 'l')
            {
                char* start = (++p);
                char* l_end = start;
                while((*p > '0') && (*p<='9'))
                {
                    int ret = parsingBencodeString(p, &keylen, &l_end);
                    if(ret != 0)
                    {
                        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "paring cmd list failed, cmd [%s], value[%s]", cmdstr, p);
                        goto retcode;
                    }
                    p = l_end+keylen;
                }
                if(*p != 'e')
                {
                    ret = -1;
                    goto retcode;
                }
                value = new std::string(start, p-start);
                p++;
            }
            cdmParameters_map::iterator iter = m_cmdparams.find(key);
            if(iter != m_cmdparams.end())
            {
                std::string* oldvalue = iter->second;
                //tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "cmd session %s modify key %s old value[%s] to new balue [%s]", 
                //                                              m_session_key->m_cookie, key.c_str(), oldvalue->c_str(), value->c_str());
                delete oldvalue;
            }
            m_cmdparams[key] = value;
            //tracelog("RTP", DEBUG_LOG,__FILE__, __LINE__, "cmd session %s, parameters key: [%s]: value:[%s]", m_session_key->m_cookie, key.c_str(), value->c_str());
        }
        else if(ret == 0 && keylen == 0)
        {
            break;
        }
        else
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "paring cmd key failed, cmd [%s] key[%s]", cmdstr, p);
            goto retcode;
        }
        parsingCmdLen = p-cmdstr;
    }
retcode:
    return ret;
}

void CmdSession::getCmdValueByStrKey(const char* key_c, std::string** v)
{
    std::string key(key_c);;
    cdmParameters_map::iterator iter = m_cmdparams.find(key);
    if(iter != m_cmdparams.end())
    {
        *v = iter->second;
    }
    else
    {
        *v = NULL;
    }
}

int CmdSession::process_cookie(const char* cookie, int cookie_len)
{
    if(m_last_cookie)
    {
        if(m_last_cookie->m_cookie.len == cookie_len)
        {
            if(0 == strncmp(m_last_cookie->m_cookie.s, cookie, cookie_len))// retransmit
            {
                int ret = sendcmd(m_last_cookie->m_resp.s, m_last_cookie->m_resp.len);
                if(ret < 0 )
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "retransmit response failed  [%s] ", m_session_key->m_cookie);
                    rmSocketInfo();
                    return -1;
                }
            }
        }
    }
    return 0;
}

int CmdSession::resp_cookie_cache_with_newcookie(const char* cookie, int cookie_len)
{
    if(m_last_cookie)
    {
        int len = m_last_cookie->m_resp.len - m_last_cookie->m_cookie.len + cookie_len;
        char* newresp = new char[len +1];
        snprintf(newresp, len+1, "%s%s", cookie, m_last_cookie->m_resp.s+m_last_cookie->m_cookie.len);
        delete m_last_cookie;
        m_last_cookie = new LastCookie(cookie, cookie_len);
        m_last_cookie->m_resp.len = len;   
        m_last_cookie->m_resp.s = newresp;

        tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__,"resp_cookie_cache_with_newcookie [%s]", m_last_cookie->m_resp.s);
        int ret = sendcmd(m_last_cookie->m_resp.s, m_last_cookie->m_resp.len);
        if(ret < 0 )
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "retransmit response failed  %s ", m_session_key->m_cookie);
            rmSocketInfo();
            return -1;
        }
        return 0;
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"no cookie cache in session [%s]", m_session_key->m_cookie);
        return -1;
    }
}

int CmdSession::cache_cookie_resp(const char* cookie, int cookie_len, const char* resp, int resp_len)
{
    if(m_last_cookie)
    {
        if(m_last_cookie->m_cookie.len == cookie_len)
        {
            if(0 == strncmp(m_last_cookie->m_cookie.s, cookie, cookie_len))// retransmit
            {
                return 0;
            }
        }
        delete m_last_cookie;
        goto cachecookie;
    }
    else
    {
        goto cachecookie;
    }

cachecookie:
    m_last_cookie = new LastCookie(cookie, cookie_len);
    m_last_cookie->m_resp.len = resp_len;
    m_last_cookie->m_resp.s = new char[resp_len+1];
    snprintf(m_last_cookie->m_resp.s, resp_len+1, "%s", resp);
    return 0;
}

int CmdSession::setSdp(int type, Sdp_session* sdp)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "must not set sdp info in none call cmd session, session key %s", m_session_key->m_cookie);
    return -1;   
}

int CmdSession::getSdp(int type, Sdp_session**sdp)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "must not set sdp info in none call cmd session, session key %s", m_session_key->m_cookie);
    *sdp = NULL;
    return -1;
}

void CmdSession::resetCookie(const char* cookie, int len)
{
    if(m_cookie.len > 0)
    {
        delete[] m_cookie.s;
    }
    m_cookie.len = len;
    m_cookie.s = new char[len+1];
    snprintf(m_cookie.s, len+1, "%s", cookie);
}

CallCmdSession::CallCmdSession():CmdSession()
{
    external_peer_sdp = NULL;
    external_local_sdp = NULL;
    internal_peer_sdp = NULL;
    internal_local_sdp = NULL;
}

CallCmdSession::CallCmdSession(char* cookie):CmdSession(cookie)
{
    external_peer_sdp = NULL;
    external_local_sdp = NULL;
    internal_peer_sdp = NULL;
    internal_local_sdp = NULL;
}

CallCmdSession::~CallCmdSession()
{
    if(external_peer_sdp)
    {
        delete external_peer_sdp;
    }
    if(external_local_sdp)
    {
        delete external_local_sdp;
    }
    if(internal_peer_sdp)
    {
        delete internal_peer_sdp;
    }
    if(internal_local_sdp)
    {
        delete internal_local_sdp;
    }
}

int CallCmdSession::setSdp(int type, Sdp_session* sdp)
{
    switch (type)
    {
        case EXTERNAL_PEER:
        {
            external_peer_sdp = sdp;
            break;
        }
        case INTERNAL_PEER:
        {
            internal_peer_sdp = sdp;
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "set sdp info with wrong direction, session key %s", m_session_key->m_cookie);
            return -1;
        }
    }
    return 0;
}

int CallCmdSession::getSdp(int type, Sdp_session**sdp)
{
    switch (type)
    {
        case EXTERNAL_PEER:
        {
            *sdp = external_peer_sdp;
            break;
        }
        case INTERNAL_PEER:
        {
            *sdp = internal_peer_sdp;
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "get sdp info with wrong direction, session key %s", m_session_key->m_cookie);
            return -1;
        }
    }
    return 0;
}

int CallCmdSession::checkState(StateCheckArgs* stateArgs)
{
    return m_css->checkState(stateArgs);
}

int CallCmdSession::processSdpResp(Sdp_session* sdp, RTPDirection direction)
{
    CmdSessionState* nextState = NULL;
    int ret = m_css->processSdpResp(sdp, direction, &nextState);
    if(ret == 0 && nextState)
    {
        delete m_css;
        m_css = nextState;
    }
    return ret;
}

NoneCallCmdSession::NoneCallCmdSession():CmdSession()
{
}

NoneCallCmdSession::~NoneCallCmdSession()
{
}

int NoneCallCmdSession::checkPingKeepAlive(PingCheckArgs* pingArg)
{
    int ret = m_css->checkPingKeepAlive(pingArg);
    if(ret != 0)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "checkPingKeepAlive failed %s ", m_session_key->m_cookie);
        rmSocketInfo();
    }
    return ret;
}


int NoneCallCmdSession::sendPongResp()
{
    int ret = 0;
    char pongresp[256];
    int len = snprintf(pongresp, sizeof(pongresp), "%s d6:result4:ponge", m_cookie.s);
    ret = sendcmd(pongresp, len);
    if(ret < 0 )
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "cmd session %s, response pong cmd", m_session_key->m_cookie);
        rmSocketInfo();
    }
    //tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__, "cmd session [%s], response pong [%s]", m_session_key->m_cookie, pongresp);
    return ret;
}


