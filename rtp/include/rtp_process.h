#ifndef _RTPPROXY_RTP_PROCESS_H__
#define _RTPPROXY_RTP_PROCESS_H__

#include "thread.h"
#include "lockfreequeue_mutipush_one_pop.h"

typedef memqueue_s rtpQ;

class RtpProcess : public Thread
{
public:
    RtpProcess();
    virtual~RtpProcess();
    virtual void* run();

private:
    int m_ep_fd;
    int m_fd_pipe[2];
    rtpQ* m_offer_q;
    rtpQ* m_answer_q;
    rtpQ* m_delete_q;
};


#endif
