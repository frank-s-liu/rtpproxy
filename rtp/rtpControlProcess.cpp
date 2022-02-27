#include "rtpControlProcess.h"
#include "rtpConfiguration.h"
#include "log.h"
#include "bencode.h"
#include "args.h"

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <string.h>

static void setnoblock(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

static void setkeepalive(int fd, int interval)
{   
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)
    {   
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "set socket SO_KEEPALIVE failed, err no is %d", errno);
        return;
    }
    #ifdef __linux__
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0)
    {   
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "set socket TCP_KEEPIDLE failed, err no is %d", errno);
        return;
    }
    val = interval/3; 
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) 
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "set socket TCP_KEEPINTVL failed, err no is %d", errno);
        return;
    }   
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0)
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "set socket TCP_KEEPCNT failed, err no is %d", errno);
        return;
    }
    #endif
}

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
    m_fd_socketInfo = NULL;
    m_epoll_socket_data = NULL;
    m_pipe_timer_events = initQ(10);
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
        close(m_fd_socketInfo[i].fd);
        m_fd_socketInfo[i].fd = -1;
    }
    delete[] m_fd_socketInfo;
    delete[] m_epoll_socket_data;
    if(m_pipe_timer_events)
    {
        PipeTimerEventArgs* args = NULL;
        while(0 == pop(m_pipe_timer_events,(void**)&args))
        {
            delete args;
        }
        free(m_pipe_timer_events);
        m_pipe_timer_events = NULL;
    }   
}

void* ControlProcess::run()
{
    struct epoll_event events[EPOLL_LISTEN_CNT];
    int ep_fd = epoll_create(EPOLL_LISTEN_CNT);
    Epoll_data pipe_data;
    RTP_CONFIG* rtpconfig = getRtpConf();
    m_fd_socket_num = rtpconfig->rtp_ctl_interfaces_num;
    m_fd_socketInfo = new SocketInfo[m_fd_socket_num];
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
                m_fd_socketInfo[i].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                // when restart service, if socket is in timewait state, maybe something wrong, So need to set SO_REUSEADDR.
                if(-1 == setsockopt(m_fd_socketInfo[i].fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)))
                {
                    tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "set option of listen socket failed, err no is %d", errno);
                    assert(0);
                }
            }
            else if(RTP_CTL_UDP == rtpconfig->rtpctl_interfaces[i].transport)
            {
                m_fd_socketInfo[i].fd = socket(AF_INET, SOCK_DGRAM, 0);
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
            if(-1 == bind(m_fd_socketInfo[i].fd, (struct sockaddr*)&server, sizeof(server)))
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "bind rtp control listen socket failed, err no is %d", errno);
                assert(0);
            }
            if(RTP_CTL_TCP == rtpconfig->rtpctl_interfaces[i].transport)
            {
                if(-1 == listen(m_fd_socketInfo[i].fd, SOMAXCONN))
                {
                    tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, " listen on listen_socket failed, err no is %d", errno);
                    assert(0);
                }
                //m_fd_socketInfo[i].fd_tcp_state = LISTENED;
                m_epoll_socket_data[i].epoll_fd_type = RTP_RES_CMD_SOCKET_ACCEPT_FD;
            }
            else
            {
                m_epoll_socket_data[i].epoll_fd_type = RTP_RES_CMD_SOCKET_UDP_FD;
            }
            m_epoll_socket_data[i].data = &m_fd_socketInfo[i];
            event.data.ptr = &m_epoll_socket_data[i];
            event.events = EPOLLIN;
            if(-1 == epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_fd_socketInfo[i].fd, &event))
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "epoll_ctl EPOLL_CTL_ADD cmd socket error %d, ip %s", errno, rtpconfig->rtpctl_interfaces[i].ip);
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
                        int new_client_fd = -1;
                        SocketInfo* srvSocketInfo = (SocketInfo*)data->data;
                        struct sockaddr_in client_addr;
                        socklen_t cliaddr_len = sizeof(client_addr);
                        new_client_fd = accept(srvSocketInfo->fd, (struct sockaddr*)&client_addr, &cliaddr_len);
                        if(-1 != new_client_fd)
                        {
                            struct epoll_event event;
                            Epoll_data* tcpclientdata = new Epoll_data();
                            SocketInfo* socketinfo = new SocketInfo();
                            socketinfo->fd = new_client_fd;
                            //socketinfo->fd_tcp_state = CONNECTED;
                            socketinfo->cmd_not_completed = NULL;
                            tcpclientdata->epoll_fd_type = RTP_RES_CMD_SOCKET_TCP_FD;
                            tcpclientdata->data = (void*)socketinfo;
                            setnoblock(new_client_fd);
                            setkeepalive(new_client_fd, 3600);
                            event.data.ptr = (void*)tcpclientdata;
                            event.events = EPOLLIN;
                            if(-1 == epoll_ctl(ep_fd, EPOLL_CTL_ADD, new_client_fd, &event))
                            {
                                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "epoll_ctl EPOLL_CTL_ADD error %d", errno);
                                close(new_client_fd);
                                delete socketinfo;
                                delete tcpclientdata;
                            }
                            continue;
                        }
                        else
                        {
                            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "accept  errno  %d", errno);
                            break;
                        }
                    }
                    else if(type == RTP_RES_CMD_SOCKET_TCP_FD)
                    {
                        //SocketInfo* socketinfo = (SocketInfo*)data->data;
                        tcpRecvBencode(data);
                    }
                    else if(type == RTP_RES_CMD_SOCKET_UDP_FD)
                    {

                    }
                    else if(type == RTP_EPOLL_PIPE_FD)
                    {
                        char buf[1] = {1};
                        int len = read(m_fd_pipe[0], buf, sizeof(buf));
                        if(len > 0)
                        {
                            //Endpoint* u = NULL;
                            //if(buf[0] == 'a')
                            //{
                                //pop(m_user_q, (void**)&u);
                                //if(u)
                                //{
                                    //u->set_epoll_fd_type(SOCKET_TYPE);
                                    //u->addSock2Epoll_recv(m_ep_fd);
                                //}
                                //else
                                //{
                                //    tracelog("SIP", ERROR_LOG, __FILE__, __LINE__, "unknown issue, pop user is null");
                                //}
                            //}
                        }       
                        else if(len == 0)
                        {       
                            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unknown issue, read len is 0");
                        }           
                        else    
                        {       
                            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "pipe read error, no. is %d", errno);
                        }
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
                    else// maybe client disconnect with server becauseof network issue, So when srv send keepalive msg to client, may got this error
                    {
                        SocketInfo* info = (SocketInfo*)data->data;
                        epoll_ctl(ep_fd, EPOLL_CTL_DEL, info->fd, NULL);
                        close(info->fd);
                        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "socket error, event is %d", events[i].events);
                        if(info->cmd_not_completed)
                        {
                            delete[] info->cmd_not_completed;
                        }
                        delete info;
                        if(0 == data->session_count)
                        {
                            delete data;
                        }
                        else
                        {
                            data->data = NULL;
                        }
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
