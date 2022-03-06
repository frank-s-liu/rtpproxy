#include "rtpepoll.h"
#include "log.h"
#include "bencode.h"
#include "cmdSession.h"
#include "cmdSessionManager.h"

#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdio.h>

SocketInfo::SocketInfo()
{
    m_fd = -1;
}

SocketInfo::~SocketInfo()
{
    if(m_fd >=0)
    {
        close(m_fd);
        m_fd = -1;
    }
}

TcpSocketInfo::TcpSocketInfo()
{
    cmd_not_completed = NULL;
}

TcpSocketInfo::~TcpSocketInfo()
{
    if(cmd_not_completed)
    {
        delete[] cmd_not_completed;
        cmd_not_completed = NULL;
    }
}

int TcpSocketInfo::sendMsg(const char* buf, int len)
{
    int ret  = 0;
    if(m_fd >= 0) 
    {
        ret = send(m_fd, buf, len, 0);
    }
    else
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "recv bencode cmd failed becauseof socket fd is not initialed");
        ret = -1;
    }
    return ret;
}

int TcpSocketInfo::recvBencode(Epoll_data* data)
{
    char buffer[2048];
    char* cmd = NULL;
    char* cmdnew = NULL;
    int len = 0;
    int ret = 0;
    len = recv(m_fd, buffer, sizeof(buffer)-1, 0);
    if(len > 0)
    {
        char* end = NULL;
        buffer[len] = '\0';
        if(!cmd_not_completed)
        {
            cmd = buffer;
        }
        else
        {
            int l = strlen(cmd_not_completed);
            l += len;
            if(l>8192)
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "recv bencode cmd too long");
                ret = -1;
                goto retprocess;
            }
            cmdnew = new char[l+1];
            snprintf(cmdnew, l+1, "%s%s",cmd_not_completed, buffer); 
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "recv bencode cmd, tcp packet splicing");
            cmd = cmdnew;
            delete[] cmd_not_completed;
            cmd_not_completed = NULL;
        }
        end = cmd;

        while(end && *end != '\0')
        {
            ret = data->bencodeCheck(cmd, &end);
            if(ret == SUCCESS)
            {
                cmd = end;
                continue;
            }
            else if(ret == BENCODE_NOT_COMPLETED)
            {
                int len = strlen(cmd);
                cmd_not_completed = new char[len+1];
                snprintf(cmd_not_completed, len+1, "%s", cmd);
                tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "recv bencode cmd not completed");
                break;
            }
            else
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "recv bencode cmd str format error, [%s]", cmd);
                goto retprocess;
            }
        }
        goto retprocess;
    }
    else if(len == 0)
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error, peer side close socket");
        ret = -1;
        goto retprocess;
    }
    else
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error,  errno is %d", errno);
        ret = -1;
        goto retprocess;
    }

retprocess:
    if(cmdnew)
    {
        delete[] cmdnew;
        cmdnew = NULL;
    }
    return ret;
}

int TcpSocketInfo::modify_write_event2Epoll(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    ret = epoll_ctl(ep_fd, EPOLL_CTL_MOD, m_fd, &event);
    if(ret < 0) 
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "can not modify tcp socket to epool loop,  reason %d ", errno);
    }
    return ret;
}

int TcpSocketInfo::modify_read_event2Epoll(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLRDHUP;
    ret = epoll_ctl(ep_fd, EPOLL_CTL_MOD, m_fd, &event);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify tcp socket to epool loop, reason %d ", errno);
    }
    return ret;
}

UdpSocketInfo::UdpSocketInfo()
{
}

UdpSocketInfo::~UdpSocketInfo()
{

}

int UdpSocketInfo::recvBencode(Epoll_data* data)
{
    return 0;
}

int UdpSocketInfo::modify_write_event2Epoll(int ep_fd, void* event_data)
{
    return 0;
}

int UdpSocketInfo::modify_read_event2Epoll(int ep_fd, void* event_data)
{
    return 0;
}

int UdpSocketInfo::sendMsg(const char* buf, int len)
{
#if 0
        struct sockaddr_in rmt_addr;
        rmt_addr.sin_family = AF_INET;
        rmt_addr.sin_port = htons(getSIPparams()->outboundport);
        rmt_addr.sin_addr.s_addr = inet_addr(getSIPparams()->outboundhost);
        len = m_socket->send_to(sip->c_str(), sip->length(), 0, (struct sockaddr* )&rmt_addr, sizeof(struct sockaddr_in));
#endif
    return 0;
}

Epoll_data::~Epoll_data()
{
    if(m_socket)
    {
        rm_fd_from_epoll();
        delete m_socket;
        m_socket = NULL;
    }
    if(m_sessions_l)
    {
        Sessions_l::iterator ite = m_sessions_l->begin();
        for(; ite != m_sessions_l->end(); )
        {
            if(*ite)
            {
                delete *ite;
            }
            else
            {
                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknow issue, session key is null");
            }
            ite = m_sessions_l->erase(ite);
        }
    }
    tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "delete socket");
}

int Epoll_data::rm_fd_from_epoll()
{
    int ret = 0;
    if(m_socket)
    {
        ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_socket->m_fd, NULL);
        if(ret < 0)  
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "can not rm socket from epool loop, errno is %d", errno);
        }
    }
    return ret;
}

int Epoll_data::sendMsg(const char* buf, int len)
{
    if(m_socket)
    {
        return m_socket->sendMsg(buf, len);
    }
    else
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "socket is null");
        return -1;
    }
}

int Epoll_data::recvBencodeCmd()
{
    if(m_socket)
    {
        return m_socket->recvBencode(this);
    }
    else
    {
        return -1;
    }
}

int Epoll_data::modify_write_event2Epoll(SessionKey* sk)
{
    if(m_socket)
    {
        if(!m_sessions_l)
        {
            m_sessions_l = new Sessions_l;
        }
        SessionKey* nsk = new SessionKey(sk->m_cookie);
        m_sessions_l->push_back(nsk);
        return m_socket->modify_write_event2Epoll(m_epoll_fd, this);
    }
    else
    {
        return -1;
    }
}

int Epoll_data::modify_read_event2Epoll()
{
    if(m_socket)
    {
        return m_socket->modify_read_event2Epoll(m_epoll_fd, this);
    }
    else
    {
        return -1;
    }

}

int Epoll_data::flushmsgs()
{
    int ret = 0;
    if(m_sessions_l)
    {
        Sessions_l::iterator ite = m_sessions_l->begin();
        for(; ite != m_sessions_l->end(); )
        {
            SessionKey* sk = *ite;
            if(sk)
            {
                CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(sk);
                if(cs)
                {
                    ret = cs->flushmsgs();
                }
                else
                {
                    tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "when send msg for cs of %s, cs has beed delete", sk->m_cookie);
                }
                delete sk;
            }
            else
            {
                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknow issue, session key is null");
            }

            ite = m_sessions_l->erase(ite);
            if(0 != ret)
            {
                break;
            }
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "write event triggered, but no msg need to send");
    }
    return ret;
}

int Epoll_data::bencodeCheck(char* cmdstr, char** end)
{
    char* p = cmdstr;
    char* begin = p;
    char* cookie = strstr(p, " d");
    if(cookie)
    {
        p = cookie+2;
        begin = p;
        while(*p)
        {
            int ret = parsingString(p, &begin); // string type key
            if(SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                p = begin;
            }
            if(*p>='0' && *p<='9')// string type value
            {
                ret = parsingString(p, &begin);
            }
            else if(*p == 'i') // int type value
            {
                //ret = parsingInt(p, &begin);
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "not support int type value");
                return FORMAT_ERR;
            }
            else
            {
                return FORMAT_ERR;
            }
            if(SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                p = begin;
            }
            if(*p == 'e')
            {
                char tmp = '\0';
                p++;
                if(*p != '\0')
                {
                    tmp = *p;
                    *p = '\0';
                }
                parseBencodeCmd(cmdstr);
                *p = tmp;
                *end = p;
                return SUCCESS;
            }
            else
            {
                continue;
            }
        }
        return BENCODE_NOT_COMPLETED;
    }
    else
    {
        return FORMAT_ERR;
    }
    return SUCCESS;
}

int Epoll_data::parsingString(char* bencode_str_start, char** bencode_str_end)
{
    char* p = bencode_str_start;
    char* begin = p; 
    int cmdlen = 0;
    while(*p)
    {
        if(*p>='0' && *p<='9')
        {
            p++;
        }
        else
        {
            break;
        }
    } 
    if(*p != ':')
    {
        if(*p == '\0')
        {
            return BENCODE_NOT_COMPLETED;
        }
        return FORMAT_ERR;
    }
    else
    {
        *p = '\0';
        int len = atoi(begin);
        if(len <= 0)
        {
            return FORMAT_ERR;
        }
        *p = ':';
        cmdlen = strlen(p);
        if(cmdlen <= len)
        {
            return BENCODE_NOT_COMPLETED;
        }
        p += (len+1);
        *bencode_str_end = p;
    }
    return SUCCESS;
}

int Epoll_data::parseBencodeCmd(char* cmdstr)
{   
    char* start = cmdstr;
    char* cookie = strstr(start, " d");
    if(cookie)
    {   
        *cookie = '\0';
        SessionKey* sk = new SessionKey(start);
        *cookie = ' '; 
        CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(sk);
        if(!cs)
        {   
            cs = new CmdSession();
            cs->m_session_key = sk;
            CmdSessionManager::getInstance()->putinCmdSession(cs);
            tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "new cmd session, cookie is [%s]", sk->m_cookie);
        }
        else
        {   
            delete sk;
        }
        cs->setSocketInfo(this);
        cs->process_cmd(cookie+2);
    }
    else
    {   
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "new cmd session, no cookie");
    }
    return 0;
}


