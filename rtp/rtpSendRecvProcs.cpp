#include "rtpSendRecvProcs.h"
#include "rtpepoll.h"
#include "log.h"
#include "sdp.h"
#include "args.h"
#include "rtpepoll.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


class Sdp_session;

RtpProcess::RtpProcess():Thread("rtpprocs")
{
    m_ep_fd = epoll_create(EPOLL_LISTEN_CNT);
    m_fd_pipe[0] = -1;
    m_fd_pipe[1] = -1;
    if(pipe(m_fd_pipe) < 0)
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "RTP process thread create pipe failed");
        assert(0);
    }
    int flags = fcntl(m_fd_pipe[0], F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(m_fd_pipe[0], F_SETFL, flags);
    flags = fcntl(m_fd_pipe[1], F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(m_fd_pipe[1], F_SETFL, flags);
    m_rtp_q = initQ(10);
}

RtpProcess::~RtpProcess()
{
    if(m_ep_fd >= 0)
    {
        close(m_ep_fd);
        m_ep_fd = -1;
    }
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
    if(m_rtp_q)
    {
        PipeEventArgs* args = NULL;
        while(0 == pop(m_rtp_q,(void**)&args))
        {   
            delete args;
        }
        freeQ(m_rtp_q);
        m_rtp_q = NULL;
    }
}

void* RtpProcess::run()
{
    struct epoll_event event;
    struct epoll_event events[EPOLL_LISTEN_CNT];
    int ret = 0;
    int fd_cnt = 0;
    memset(&event, 0, sizeof(event)); 
    Epoll_data data;
    data.m_epoll_fd_type = RTP_EPOLL_PIPE_FD;
    event.data.ptr = &data;
    event.events = EPOLLIN;
    ret = epoll_ctl(m_ep_fd, EPOLL_CTL_ADD, m_fd_pipe[0], &event);
    if(ret < 0)
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "epoll_ctl failed in RTP_send");
        assert(0);
    }
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
                Epoll_data* data = (Epoll_data*)events[i].data.ptr;
                if(events[i].events & EPOLLIN)
                {
                    int type = data->m_epoll_fd_type;
                    if(type == RTP_EPOLL_PIPE_FD)
                    {
                        char buf[1] = {0};
                        int len = read(m_fd_pipe[0], buf, sizeof(buf));
                        if(len > 0)
                        {
                            PipeEventArgs* pipeArg = NULL;
                            if(buf[0] == 'a')
                            {
                                if(0 == pop(m_rtp_q, (void**)&pipeArg))
                                {
                                    Args* arg = pipeArg->args_data;
                                    arg->processCmd();
                                    delete pipeArg;   
                                }
                                else
                                {
                                    tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown issue,  no spd poped");
                                }
                            }
                            else
                            {
                                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "wrong pipe read value %c", buf[0]);
                                break;
                            }
                        }
                        else if(len == 0)
                        {
                            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown issue, read len is 0, empty pipe");
                            break;
                        }
                        else
                        {
                            if(EAGAIN == errno)
                            {
                                continue;
                            }
                            else
                            {
                                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown pipe issue, error no. %d", errno);
                                break;
                            }
                        }
                    }
                    else if(RTP_RECV_SOCKET_FD)
                    {

                    }
                    else
                    {
                        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unknow type: %d", type);
                    }
                }
                else if(events[i].events & EPOLLOUT)
                {

                }
                else if((events[i].events & EPOLLERR) ||
                        (events[i].events & EPOLLHUP) ||
                        (events[i].events & EPOLLRDHUP))
                {
                    tracelog("SIP", WARNING_LOG, __FILE__, __LINE__, "socker error, event %d", events[i].events);
                    break;
                }
                else
                {
                    tracelog("SIP", WARNING_LOG, __FILE__, __LINE__, "rtp process, unknown event %u", events[i].events);
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
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "rtp process thread has epoll_wait issue, error %d", errno);
                break;
            }
        }
    }
    m_status = THREAD_STOPPED;
    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp process thread exit");
    return NULL;
}

int RtpProcess::add_pipe_event(Args* args)
{
    int ret = 0;
    if(!m_isStop)
    {
        PipeEventArgs* pipe_event   = new PipeEventArgs();
        pipe_event->args_data       = args;
        if(push(m_rtp_q, pipe_event))
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "queue is full, can not push offer req");
            ret = -1;
        }
        else
        {
            char buf[1] = {'a'};
            int len = write(m_fd_pipe[1], buf, sizeof(buf));
            if(len <= 0)
            {
                void* tmpsdp = NULL;
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "offerProcess failed, pipe write issue, errno %d", errno);
                pop(m_rtp_q, &tmpsdp);
                ret = -1;
            }
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rep process thread has exit");
        ret = -1;
    }
    return ret;
}
