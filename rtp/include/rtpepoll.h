#ifndef __RTP_EPOLL_DATA_H__
#define __RTP_EPOLL_DATA_H__

#include "cstr.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <list>


class Epoll_data;
class SocketInfo
{
public:
    SocketInfo();
    virtual ~SocketInfo();
    virtual int sendMsg(const char* buf, int len) = 0;    
    virtual int recvBencode(Epoll_data* data) = 0;
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data) = 0;
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data) = 0;
    virtual int getRemotePort(unsigned short* port);
    virtual int getRemoteAddress(char* buf, int buflen);
public:
    int                  m_fd;
};

class TcpSocketInfo: public SocketInfo
{
public:
    TcpSocketInfo(unsigned short port);
    virtual ~TcpSocketInfo();
    virtual int sendMsg(const char* buf, int len);
    virtual int recvBencode(Epoll_data* data);
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data);
    virtual int getRemotePort(unsigned short* port);
    virtual int getRemoteAddress(char* buf, int buflen);

public:
    char*                cmd_not_completed;
    unsigned short       m_remote_port;
    char                 m_remote_ip[32];
};

class UdpSocketInfo : public SocketInfo
{
public:
    UdpSocketInfo();
    virtual ~UdpSocketInfo();
    virtual int sendMsg(const char* buf, int len);
    virtual int recvBencode(Epoll_data* data);
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data);

public:
    

};

class SessionKey;
typedef std::list<SessionKey*> Sessions_l;
class Epoll_data
{
public:
    Epoll_data();
    virtual ~Epoll_data();
    int rm_fd_from_epoll();
    int sendMsg(const char* buf, int len);
    int recvBencodeCmd();
    int modify_write_event2Epoll(SessionKey* sk);
    int modify_read_event2Epoll();
    int flushmsgs();
    int parseBencodeCmd(char* cmdstr, const char* key, int keylen);
    int parsingString(char* bencode_str_start, char** bencode_str_end);
    int parsingList(char* bencode_str_start, char** bencode_str_end);
    int bencodeCheck(char* cmdstr, char** end);

public:
    SocketInfo*              m_socket;   //socket info
    Sessions_l*              m_sessions_l; // session keys need to send msg but blocked in socket  
    int                      m_session_count;
    int                      m_epoll_fd;
    unsigned char            m_epoll_fd_type;
    cstr                     m_nocall_key; // used for pingpong
};

// RTP RTCP send recv socket epoll data
class RTP_send_recv_epoll_data
{
public:
    RTP_send_recv_epoll_data();
    virtual ~RTP_send_recv_epoll_data();
    
public:
    void*                    m_data;
    unsigned char            m_epoll_fd_type;
};

enum socket_type
{
    RTP_RES_CMD_SOCKET_ACCEPT_FD = 1,
    RTP_RES_CMD_SOCKET_UDP_FD,
    RTP_RES_CMD_SOCKET_TCP_FD,
    RTP_SEND_RECV_SOCKET_FD,
    RTCP_SEND_RECV_SOCKET_FD,
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
