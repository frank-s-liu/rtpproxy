#ifndef __RTP_EPOLL_DATA_H__
#define __RTP_EPOLL_DATA_H__

#include <netinet/in.h>
#include <stdlib.h>


typedef struct socketinfo
{
    char*                cmd_not_completed;
    int                  fd;
}SocketInfo;

class Epoll_data
{
public:
    virtual ~Epoll_data();
    int rm_fd_from_epoll();
public:
    socketinfo*          data;  //socket info
    int                  session_count;
    int                  epoll_fd;
    char                 epoll_fd_type;
    unsigned char        fd_state; 
};

typedef enum socketstate
{
    CLOSED = 0,
    CONNECTED, // just mean fd is OK, tcp mean TCP connected, udp means not closed.
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
