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
    RTP_send_recv_epoll_data data;
    data.m_epoll_fd_type = RTP_EPOLL_PIPE_FD;
    event.data.ptr = &data;
    event.events = EPOLLIN;
    ret = epoll_ctl(m_ep_fd, EPOLL_CTL_ADD, m_fd_pipe[0], &event);
    if(ret < 0)
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "epoll_ctl failed in RTP_send");
        assert(0);
    }
    tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "rtp send recv thread start");
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
                RTP_send_recv_epoll_data* data = (RTP_send_recv_epoll_data*)events[i].data.ptr;
                if(events[i].events & EPOLLIN)
                {
                    int type = data->m_epoll_fd_type;
                    if(RTP_EPOLL_PIPE_FD == type)
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
                                    tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown issue,  no pipe args poped, but recv a pipe single");
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
                            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown issue, read len is 0, empty pipe, but epoll said it can be read");
                            break;
                        }
                        else
                        {
                            if(EAGAIN == errno)
                            {
                                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "rtp process thread pipe read nothing, no pipe data?");
                                break;
                            }
                            else
                            {
                                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown pipe issue, error no. %d", errno);
                                break;
                            }
                        }
                    }
                    else if(RTP_SEND_RECV_SOCKET_FD == type)
                    {
                        RtpStream* rs = (RtpStream*)data->m_data;
                        rs->readAndProcess();
                    }
                    else
                    {
                        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unknow type: %d", type);
                    }
                }
                else if(events[i].events & EPOLLOUT)
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"----------------");
                }
                else if((events[i].events & EPOLLERR) ||
                        (events[i].events & EPOLLHUP) ||
                        (events[i].events & EPOLLRDHUP))
                {
                    tracelog("SIP", WARNING_LOG, __FILE__, __LINE__, "rtp process socker error, event %d", events[i].events);
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
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "rtp process thread was interupted, continue");
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


// if return -1, caller need to delete args itself
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
            pipe_event->args_data = NULL;
            delete pipe_event;
            ret = -1;
        }
        else
        {
            char buf[1] = {'a'};
            int len = write(m_fd_pipe[1], buf, sizeof(buf));
            if(len <= 0)
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "offerProcess failed, pipe write issue, errno %d", errno);
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

RtpSession* RtpProcess::getRtpSession(const char* key)
{
    if(!key)
    {
        return NULL;
    }
    SessionKey* sk = new SessionKey(key);
    RtpSession* rs = getRtpSession(sk);
    delete sk;
    return rs;
}

RtpSession* RtpProcess::getRtpSession(SessionKey* sk)
{       
    Rtp_sessions_map::iterator iter = m_rtp_sessions_m.find(sk);
    if(iter != m_rtp_sessions_m.end())
    {
        return iter->second;
    }
    return NULL;
}

RtpSession* RtpProcess::popRtpSession(const char* key)
{
    if(!key)
    {
        return NULL;
    }
    SessionKey* sk = new SessionKey(key);
    RtpSession* rs = popRtpSession(sk);
    delete sk;
    return rs;
}

RtpSession* RtpProcess::popRtpSession(SessionKey* sk)
{
    RtpSession* rs = NULL;
    Rtp_sessions_map::iterator iter = m_rtp_sessions_m.find(sk);
    if(iter != m_rtp_sessions_m.end())
    {
        rs = iter->second;
        m_rtp_sessions_m.erase(iter);
    }
    return rs;
}

int RtpProcess::putRtpSession(RtpSession* rs)
{
    SessionKey* key = rs->m_session_key;
    Rtp_sessions_map::iterator iter = m_rtp_sessions_m.find(key);
    if(iter != m_rtp_sessions_m.end())
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "when put rtp session, same session key found, %s", key->m_cookie);
        return 1;
    }
    else 
    {
        m_rtp_sessions_m[key] = rs;
    }   
    return 0;
}

int RtpProcess::getEpoll_fd()
{
    return m_ep_fd;
}

