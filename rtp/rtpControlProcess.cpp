#include "rtpControlProcess.h"
#include "rtpConfiguration.h"
#include "log.h"

#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <string.h>

/*
static void setkeepalive(int fd, int interval)
{   
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)
    {   
        tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, "set socket SO_KEEPALIVE failed, err no is %d", errno);
        return;
    }
    #ifdef __linux__
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0)
    {   
        tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, "set socket TCP_KEEPIDLE failed, err no is %d", errno);
        return;
    }
    val = interval/3; 
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) 
    {
        tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, "set socket TCP_KEEPINTVL failed, err no is %d", errno);
        return;
    }   
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0)
    {
        tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, "set socket TCP_KEEPCNT failed, err no is %d", errno);
        return;
    }
    #endif
}
*/

typedef struct clientinfo
{
    int                  fd;
    unsigned char        fd_state; // 0: closed 1: connected
}ClientInfo;

ControlProcess::ControlProcess():Thread("rtpCtl")
{
    m_fd_pipe[0] = -1;
    m_fd_pipe[1] = -1;
    if(pipe(m_fd_pipe) < 0)
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "rtp control thread create pipe failed");
        assert(0);
    }
    m_fd_socket_num = 0;
    m_fd_socket = NULL;
    m_epoll_socket_data = NULL;
}

ControlProcess::~ControlProcess()
{
    if(m_fd_pipe[0] >= 0)
    {
        close(m_fd_pipe[0]);
        m_fd_pipe[0] = -1;
    }
    if(m_fd_pipe[1] >= 0)
    {
        close(m_fd_pipe[1]);
        m_fd_pipe[1] = -1;
    }
    for(int i=0; i<m_fd_socket_num; i++)
    {
        close(m_fd_socket[i]);
        m_fd_socket[i] = -1;
    }
    delete[] m_fd_socket;
    delete[] m_epoll_socket_data;
}

void* ControlProcess::run()
{
    struct epoll_event events[EPOLL_LISTEN_CNT];
    int ep_fd = epoll_create(EPOLL_LISTEN_CNT);
    Epoll_data pipe_data;
    RTP_CONFIG* rtpconfig = getRtpConf();
    m_fd_socket_num = rtpconfig->rtp_ctl_interfaces_num;
    m_fd_socket = new int[m_fd_socket_num];
    m_epoll_socket_data = new Epoll_data[m_fd_socket_num];

    {
        struct epoll_event event;
        int ret;
        memset(&event, 0, sizeof(event));
        pipe_data.epoll_fd_type = RTP_EPOLL_PIPE_FD;
        pipe_data.data = NULL;
        event.data.ptr = &pipe_data;
        event.events = EPOLLIN;
    
        ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_fd_pipe[0], &event);
        if(ret < 0)
        {
            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "rtp control epoll_ctl add pipe fd failed");
            assert(0);
        }
    }

    {
        for(int i=0; i<m_fd_socket_num; i++)
        {
            unsigned short port = rtpconfig->rtpctl_interfaces[i].port;
            struct sockaddr_in server;
            struct epoll_event event;
            if(RTP_CTL_TCP == rtpconfig->rtpctl_interfaces[i].transport)
            {
                int reuse_addr = 1;
                m_fd_socket[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                // when restart service, if socket is in timewait state, maybe something wrong, So need to set SO_REUSEADDR.
                if(-1 == setsockopt(m_fd_socket[i], SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)))
                {
                    tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, "set option of listen socket failed, err no is %d", errno);
                    assert(0);
                }
            }
            else if(RTP_CTL_UDP == rtpconfig->rtpctl_interfaces[i].transport)
            {
                m_fd_socket[i] = socket(AF_INET, SOCK_DGRAM, 0);
            }
            else
            {
                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "rtp control only support UDP&TCP, current configuration is %d", rtpconfig->rtpctl_interfaces[i].transport);
                assert(0);
            }
            memset(&server, 0, sizeof(struct sockaddr_in));
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr(rtpconfig->rtpctl_interfaces[i].ip);
            server.sin_port = htons(port);
            if(-1 == bind(m_fd_socket[i], (struct sockaddr*)&server, sizeof(server)))
            {
                tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, "bind rtp control listen socket failed, err no is %d", errno);
                assert(0);
            }
            if(RTP_CTL_TCP == rtpconfig->rtpctl_interfaces[i].transport)
            {
                if(-1 == listen(m_fd_socket[i], SOMAXCONN))
                {
                    tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, " listen on listen_socket failed, err no is %d", errno);
                    assert(0);
                }
                m_epoll_socket_data[i].epoll_fd_type = RTP_RES_CMD_SOCKET_ACCEPT_FD;
            }
            else
            {
                m_epoll_socket_data[i].epoll_fd_type = RTP_RES_CMD_SOCKET_FD;
            }
            m_epoll_socket_data[i].data = NULL;
            event.data.ptr = &m_epoll_socket_data[i];
            event.events = EPOLLIN;
            if(-1 == epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_fd_socket[i], &event))
            {
                tracelog("CLI", ERROR_LOG,__FILE__, __LINE__, "epoll_ctl EPOLL_CTL_ADD cmd socket error %d, ip %s", errno, rtpconfig->rtpctl_interfaces[i].ip);
                assert(0);
            }
        }
    }
              
    while(!m_isStop)
    {
        int fd_cnt = epoll_wait(ep_fd, events, EPOLL_LISTEN_CNT, EPOLL_LISTEN_TIMEOUT);
        if(fd_cnt == 0)
        {
            continue;
        }
        else if(fd_cnt > 0)
        {
            for(int i = 0; i < fd_cnt; i++)
            {
                Epoll_data* data = (Epoll_data*)events[i].data.ptr;
                int type = data->epoll_fd_type;
                if(events[i].events & EPOLLIN)
                {
                    if(type == RTP_RES_CMD_SOCKET_ACCEPT_FD)
                    {
                        
                    }
                }
                else if( (events[i].events & EPOLLERR) ||
                         (events[i].events & EPOLLHUP) )
                {
                    if(type == RTP_RES_CMD_SOCKET_ACCEPT_FD)
                    {
                        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "rtp cmd listen socket error, event is %d", events[i].events);
                        goto returnpoint;
                    }
                    else// maybe client disconnect with server becauseof network issue, So when srv send msg to client, may got this error
                    {
                        ClientInfo* info = (ClientInfo*)data->data;
                        epoll_ctl(ep_fd, EPOLL_CTL_DEL, info->fd, NULL);
                        close(info->fd);
                        info->fd_state = 0;
                        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "socket error, event is %d", events[i].events);
                        break;// must beak and start a new epoll
                    }
                }
                else
                {
                    tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown event %u", events[i].events);
                }
            }
        }
        else
        {
            if(errno == EINTR)
            {
                continue;
            }
            else
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "rtp cmd thread: epoll_wait error %d", errno);
                break;
            }
        }
    }
returnpoint:
    m_status = THREAD_STOPPED;
    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp cmd control thread exit");
    close(ep_fd);
    delete this;
    return NULL;
}
