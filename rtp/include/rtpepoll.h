#ifndef __RTP_EPOLL_DATA_H__
#define __RTP_EPOLL_DATA_H__

#include <netinet/in.h>
#include <stdlib.h>
#include <list>


class SocketInfo
{
public:
    SocketInfo();
    virtual ~SocketInfo();
    virtual int sendMsg(char* buf, int len) = 0;    
    virtual int recvBencode() = 0;
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data) = 0;
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data) = 0;

public:
    int                  m_fd;
};

class TcpSocketInfo: public SocketInfo
{
public:
    TcpSocketInfo();
    virtual ~TcpSocketInfo();
    virtual int sendMsg(char* buf, int len);
    virtual int recvBencode();
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data);
    

public:
    char*                cmd_not_completed;
};

class UdpSocketInfo : public SocketInfo
{
public:
    UdpSocketInfo();
    virtual ~UdpSocketInfo();
    virtual int sendMsg(char* buf, int len);
    virtual int recvBencode();
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data);

public:
    

};

class SessionKey;

typedef std::list<SessionKey*> Sessions_l;
class Epoll_data
{
public:
    virtual ~Epoll_data();
    int rm_fd_from_epoll();
    int sendMsg(char* buf, int len);
    int recvBencodeCmd();
    int modify_write_event2Epoll(SessionKey* sk);
    int modify_read_event2Epoll();

public:
    SocketInfo*              m_socket;   //socket info
    Sessions_l*              m_sessions_l; // sessions need to send msg but blocked in socket  
    int                      m_session_count;
    int                      m_epoll_fd;
    unsigned char            m_epoll_fd_type;
};


enum socket_type
{
    RTP_RES_CMD_SOCKET_ACCEPT_FD = 1,
    RTP_RES_CMD_SOCKET_UDP_FD,
    RTP_RES_CMD_SOCKET_TCP_FD,
    RTP_RECV_SOCKET_FD,
    RTP_SEND_SOCKET_FD,
    RTCP_RECV_SOCKET_FD,
    RTCP_SEND_SOCKET_FD,
    RTP_STATISTICS_SOCKET_FD,

// none socket type
    RTP_EPOLL_PIPE_FD,
    RTP_TIMER_FD,
    RTP_MAX_FD_TYPE
};

enum transport_type
{
    RTP_CTL_UDP = 0,
    RTP_CTL_TCP,
    RTP_CTL_MAX_TYPE
};

static const int EPOLL_LISTEN_CNT = 16;
static const int EPOLL_LISTEN_TIMEOUT = 500; //ms


#endif
