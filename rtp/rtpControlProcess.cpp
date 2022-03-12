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

ControlProcess* ControlProcess::s_instance = NULL;

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
    m_pipe_events = initQ(10);
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
        delete m_fd_socketInfo[i];
    }
    delete[] m_fd_socketInfo;
    delete[] m_epoll_socket_data;
    if(m_pipe_events)
    {
        PipeEventArgs* args = NULL;
        while(0 == pop(m_pipe_events,(void**)&args))
        {
            delete args;
        }
        freeQ(m_pipe_events);
        m_pipe_events = NULL;
    }
}

ControlProcess* ControlProcess::getInstance()
{
    if(!s_instance)
    {
        s_instance = new ControlProcess();
    }
    return s_instance;
}

int ControlProcess::add_pipe_event(Args* args)
{
    int ret = 0;
    if(!m_isStop)
    {
        PipeEventArgs* pipe_event   = new PipeEventArgs();
        pipe_event->args_data       = args;
        //timer_event->event_type      = RTP_TIMER_EVENT;
        if(push(m_pipe_events, pipe_event))
        {
            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "pipe_events queue is full");
            delete pipe_event;
            ret = -1;
            goto retprocess;
        }
        else
        {
            char buf[1] = {'a'};
            int len = write(m_fd_pipe[1], buf, sizeof(buf));
            if(len <=0 )
            {
                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "write pipe failed, errno is %d", errno);
                ret = -1;
                goto retprocess;
            }
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "cmd ControlProcess thread has exit, don't add event");
        delete args;
        ret = -1;
        goto retprocess;
    }

retprocess:
    return ret;
}

void* ControlProcess::run()
{
    struct epoll_event events[EPOLL_LISTEN_CNT];
    int ep_fd = epoll_create(EPOLL_LISTEN_CNT);
    Epoll_data pipe_data;
    RTP_CONFIG* rtpconfig = getRtpConf();
    m_fd_socket_num = rtpconfig->rtp_ctl_interfaces_num;
    m_fd_socketInfo = new SocketInfo* [m_fd_socket_num];
    m_epoll_socket_data = new Epoll_data[m_fd_socket_num];

    {
        struct epoll_event event;
        int ret;
        memset(&event, 0, sizeof(event));
        pipe_data.m_epoll_fd_type = RTP_EPOLL_PIPE_FD;
        pipe_data.m_socket = NULL;
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
                int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                m_fd_socketInfo[i] = new TcpSocketInfo();
                m_fd_socketInfo[i]->m_fd = fd;
                // when restart service, if socket is in timewait state, maybe something wrong, So need to set SO_REUSEADDR.
                if(-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)))
                {
                    tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "set option of listen socket failed, err no is %d", errno);
                    assert(0);
                }
            }
            else if(RTP_CTL_UDP == rtpconfig->rtpctl_interfaces[i].transport)
            {
                int fd = socket(AF_INET, SOCK_DGRAM, 0);
                m_fd_socketInfo[i] = new UdpSocketInfo();
                m_fd_socketInfo[i]->m_fd = fd;
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
            if(-1 == bind(m_fd_socketInfo[i]->m_fd, (struct sockaddr*)&server, sizeof(server)))
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "bind rtp control listen socket failed, err no is %d", errno);
                assert(0);
            }
            if(RTP_CTL_TCP == rtpconfig->rtpctl_interfaces[i].transport)
            {
                if(-1 == listen(m_fd_socketInfo[i]->m_fd, SOMAXCONN))
                {
                    tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, " listen on listen_socket failed, err no is %d", errno);
                    assert(0);
                }
                m_epoll_socket_data[i].m_epoll_fd_type = RTP_RES_CMD_SOCKET_ACCEPT_FD;
            }
            else
            {
                m_epoll_socket_data[i].m_epoll_fd_type = RTP_RES_CMD_SOCKET_UDP_FD;
            }
            m_epoll_socket_data[i].m_socket = m_fd_socketInfo[i];
            event.data.ptr = &m_epoll_socket_data[i];
            event.events = EPOLLIN;
            if(-1 == epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_fd_socketInfo[i]->m_fd, &event))
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
                int type = data->m_epoll_fd_type;
                if(events[i].events & EPOLLIN)
                {
                    if(type == RTP_RES_CMD_SOCKET_ACCEPT_FD)
                    {
                        int new_client_fd = -1;
                        SocketInfo* srvSocketInfo = (SocketInfo*)data->m_socket;
                        struct sockaddr_in client_addr;
                        socklen_t cliaddr_len = sizeof(client_addr);
                        new_client_fd = accept(srvSocketInfo->m_fd, (struct sockaddr*)&client_addr, &cliaddr_len);
                        if(-1 != new_client_fd)
                        {
                            struct epoll_event event;
                            Epoll_data* tcpclientdata = new Epoll_data();
                            SocketInfo* socketinfo = new TcpSocketInfo();
                            socketinfo->m_fd = new_client_fd;
                            tcpclientdata->m_epoll_fd_type = RTP_RES_CMD_SOCKET_TCP_FD;
                            tcpclientdata->m_socket = socketinfo;
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
                    else if(type == RTP_RES_CMD_SOCKET_TCP_FD || type==RTP_RES_CMD_SOCKET_UDP_FD)
                    {
                        int ret = data->recvBencodeCmd();
                        if(ret != 0)
                        {
                            tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "recv Bencode error, delete socket");
                            data->rm_fd_from_epoll();
                            delete data->m_socket;
                            data->m_socket = NULL;
                            if(0 == data->m_session_count)
                            {
                                delete data;
                            }
                            break;// must beak and start a new epoll
                        }
                    }
                    else if(type == RTP_EPOLL_PIPE_FD)
                    {
                        char buf[1] = {1};
                        int len = read(m_fd_pipe[0], buf, sizeof(buf));
                        if(len > 0)
                        {
                            PipeEventArgs* pipeArg = NULL;
                            if(0 == pop(m_pipe_events,(void**)&pipeArg))
                            {
                                Args* arg = pipeArg->args_data;
                                arg->processCmd();
                                delete pipeArg;
                            }
                            else
                            {
                                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unknown issue, this must not happen");
                            }
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
                else if(events[i].events & EPOLLOUT)
                {
                    int ret = data->flushmsgs();
                    if(ret == 0)
                    {
                        data->modify_read_event2Epoll();
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
                        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "socket error, event is %d", events[i].events);
                        data->rm_fd_from_epoll();
                        delete data->m_socket;
                        data->m_socket = NULL;
                        if(0 == data->m_session_count)
                        {
                            delete data;
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
