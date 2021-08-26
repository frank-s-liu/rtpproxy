#ifndef __RTP_EPOLL_DATA_H__
#define __RTP_EPOLL_DATA_H__


typedef struct epoll_data                                                                                                                             
{                                                                                                                                                     
    int epoll_fd_type;                                                                                                                                
}Epoll_data;

typedef enum socket_type
{
    RTP_RES_CMD_SOCKET_FD = 1,
    RTP_RECV_SOCKET_FD,
    RTP_SEND_SOCKET_FD,
    RTP_RECV_SOCKET_FD,
    RTCP_SEND_SOCKET_FD,
    RTP_STATISTICS_SOCKET_FD,
    RTP_PIPE_FD,
    RTP_MAX_FD_TYPE
};

static const int EPOLL_LISTEN_CNT = 8;
static const int EPOLL_LISTEN_TIMEOUT = 500; //ms


#endif
