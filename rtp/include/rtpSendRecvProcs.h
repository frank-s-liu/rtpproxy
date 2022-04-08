#ifndef _RTPPROXY_RTP_PROCESS_H__
#define _RTPPROXY_RTP_PROCESS_H__

#include "thread.h"
#include "lockfreequeue_mutipush_one_pop.h"


typedef memqueue_s rtpQ;

class Args;
class RtpProcess : public Thread
{
public:
    RtpProcess();
    virtual~RtpProcess();
    virtual void* run();
    int add_pipe_event(Args* args);
private:
    int m_ep_fd;
    int m_fd_pipe[2];
    rtpQ* m_rtp_q;
};


#endif
