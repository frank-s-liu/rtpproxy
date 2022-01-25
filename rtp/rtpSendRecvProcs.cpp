#include "rtp_process.h"
#include "rtpepoll.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

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
    m_offer_q = initQ(8);
    m_answer_q = initQ(8);
    m_delete_q = initQ(8);
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
    if(m_offer_q)
    {
        free(m_offer_q);
        m_offer_q = NULL;
    }
    if(m_answer_q)
    {
        free(m_answer_q);
        m_answer_q = NULL;
    }
    if(m_delete_q)
    {
        free(m_delete_q);
        m_delete_q = NULL;
    }
}

void* RtpProcess::run()
{
    struct epoll_event event;
    struct epoll_event events[EPOLL_LISTEN_CNT];
    int ret = 0;
    int fd_cnt = 0;
    memset(&event, 0, sizeof(event)); 
    Epoll_Data data;
    data.epoll_fd_type = RTP_EPOLL_PIPE;
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
                Epoll_Data* data = (Epoll_Data*)events[i].data.ptr;
                if(events[i].events & EPOLLIN)
                {
                    int type = data->epoll_fd_type;
                    if(type == RTP_PIPE_FD)
                    {
                        char buf[1] = {0};
                        int len = read(m_fd_pipe[0], buf, sizeof(buf));
                        if(len > 0)
                        {
                            void* data = NULL;
                            if(buf[0] == 'a')
                            {
                                pop(m_offer_q, &data);
                                if(data)
                                {
                                    
                                }
                                else
                                {
                                    tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown issue,  no spd poped");
                                }
                            }
                            else if(buf[0] == 'b')
                            {

                            }
                            else if(buf[0] == 'c')
                            {

                            }
                            else
                            {
                                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "wrong pipe read value %c", buf[0]);
                            }
                        }
                        else if(len == 0)
                        {
                            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown issue, read len is 0");
                            continue;
                        }
                        else
                        {
                            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknown pipe issue, error no. %d", errno);
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
                    deletesocker;
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

int RtpProcess::offerProcess(std::string* sdp)
{
    int ret = 0;
    if(!m_isStop)
    {
        if(push(m_offer_q, sdp))
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "offer queue is full");
            ret = 1;
        }
        else
        {
            char buf[1] = {'a'};
            int len = write(m_fd_pipe[1], buf, sizeof(buf));
            if(len <= 0)
            {
                void* tmpsdp = NULL;
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "offerProcess failed, pipe write issue, errno %d", errno);
                pop(m_offer_q, &tmpsdp);
                ret = 1;
            }
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rep process thread has exit");
        ret = 1;
    }
    return ret;
}
