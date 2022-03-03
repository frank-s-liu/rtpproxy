#include "rtpepoll.h"
#include "log.h"
#include "bencode.h"

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

int TcpSocketInfo::sendMsg(char* buf, int len)
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


int TcpSocketInfo::recvBencode()
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
                goto errprocess;
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
            ret = bencodeCheck(cmd, &end);
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
                goto errprocess;
            }
        }
        goto retprocess;
    }
    else if(len == 0)
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error, peer side close socket");
        ret = -1;
        goto errprocess;
    }
    else
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error,  errno is %d", errno);
        ret = -1;
        goto errprocess;
    }

errprocess:
    close(m_fd);
    m_fd = -1;
    //socket->fd_tcp_state = CLOSED;
retprocess:
    if(cmdnew)
    {
        delete[] cmdnew;
        cmdnew = NULL;
    }
    return ret;
}

UdpSocketInfo::UdpSocketInfo()
{
}

UdpSocketInfo::~UdpSocketInfo()
{

}

int UdpSocketInfo::sendMsg(char* buf, int len)
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
}

int Epoll_data::rm_fd_from_epoll()
{
    int ret = 0;
    ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_socket->m_fd, NULL);
    if(ret < 0)  
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "can not rm socket from epool loop, errno is %d", errno);
    }
    return ret;
}

int Epoll_data::sendMsg(char* buf, int len)
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
        return m_socket->recvBencode();
    }
    else
    {
        return -1;
    }
}
