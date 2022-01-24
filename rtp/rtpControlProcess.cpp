#include "ControlProcess.h"
#include "rtpepoll.h"

#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


ControlProcess::ControlProcess():Thread("signalCtl")
{
    m_ep_fd = epoll_create(EPOLL_LISTEN_CNT);
}

ControlProcess::~ControlProcess()
{
    if(m_ep_fd >= 0)
    {
        close(m_ep_fd);
        m_ep_fd = -1;
    }
}

ControlProcess::run()
{
    //struct epoll_event event;
    struct epoll_event events[EPOLL_LISTEN_CNT];
    //int ret = 0;
    int fd_cnt = 0;
    memset(&event, 0, sizeof(event));
    Epoll_data data;
    data.epoll_fd_type = RTP_RES_CMD_SOCKET_FD;
    
    while( !m_isStop )
    {
        fd_cnt = epoll_wait(m_ep_fd, events, EPOLL_LISTEN_CNT, EPOLL_LISTEN_TIMEOUT);
        if(fd_cnt == 0)
        {
            continue;
        }
        else if(fd_cnt > 0)
        {
            for(int i = 0; i < fd_cnt; i++)
            {
                Epoll_Data* data = (Epoll_Data*)events[i].data.ptr;
                if(events[i].events & EPOLLIN)
                {
                    int type = data->epoll_fd_type;
                    if(type == RTP_RES_CMD_SOCKET_FD)
                    {
                        
                    }
                }
                else if( (events[i].events & EPOLLERR) ||
                         (events[i].events & EPOLLHUP) ||
                         (events[i].events & EPOLLRDHUP) )
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "socket error, event is %d", events[i].events);
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
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "media-ctl thread: epoll_wait error %d", errno);
                break;
            }
        }
    }
    m_status = THREAD_STOPPED;
    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media-ctl thread exit");
    return NULL;
}
