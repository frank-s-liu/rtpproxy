#ifndef _RTPPROXY_RTP_PROCESS_H__
#define _RTPPROXY_RTP_PROCESS_H__

#include "thread.h"
#include "lockfreequeue_mutipush_one_pop.h"
#include "sessionKey.h"
#include "rtpsession.h"

#include "map"


class Args;
typedef std::map<SessionKey*, RtpSession*, cmp_SessionKey> Rtp_sessions_map;
typedef memqueue_s rtpArgsQ;

class RtpProcess : public Thread
{
public:
    RtpProcess();
    virtual~RtpProcess();
    virtual void* run();
    int           add_pipe_event(Args* args);
    int           putRtpSession(RtpSession* cs);
    RtpSession*   getRtpSession(const char* key);
    RtpSession*   getRtpSession(SessionKey* sk);
    RtpSession*   popRtpSession(const char* key);
    RtpSession*   popRtpSession(SessionKey* key);
private:
    Rtp_sessions_map    m_rtp_sessions_m;
    rtpArgsQ*           m_rtp_q;
    int                 m_ep_fd;
    int                 m_fd_pipe[2];
};


#endif
