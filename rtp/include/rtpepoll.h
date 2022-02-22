#ifndef __RTP_EPOLL_DATA_H__
#define __RTP_EPOLL_DATA_H__

#include <netinet/in.h>

typedef struct epolldata
{
    int epoll_fd_type;
    void* data;  //socket info
}Epoll_data;

typedef struct socketinfo
{
    void*                data;
    int                  fd;
    unsigned char        fd_tcp_state; // 0: closed 1: connected 2: listened
}SocketInfo;

typedef enum socketstate
{
    CLOSED = 0,
    CONNECTED,
    LISTENED,
    UDP,
    MAX
}SOCKET_STATE;

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
