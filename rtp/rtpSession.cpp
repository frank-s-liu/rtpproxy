#include "rtpsession.h"
#include "log.h"
#include "sdp.h"
#include "rtpSendRecvProcs.h"


RtpSession::RtpSession()
{
}

RtpSession::RtpSession(SessionKey* key, RtpProcess* process)
{
    m_session_key = key;
    m_rtp_sendrecv_process = process;
    process->putRtpSession(this);
}

RtpSession::~RtpSession()
{
    if(m_session_key)
    {
        delete m_session_key;
    }
    m_rtp_sendrecv_process = NULL;
}

int RtpSession::processSdp(Sdp_session* sdp, RTPDirection direction)
{
    int ret = 0;
    if(!sdp || 0 == sdp->m_parsed)
    {
        return -1;
    }
    switch (direction)
    {
        case EXTERNAL_PEER:
        {
            if(!m_external)
            {
                m_external = new RtpStream(this);
            }
            m_external->set_local_rtp_network("10.100.126.229", IPV4, direction);
            m_external->set_remote_peer_rtp_network(&sdp->m_con.address);
            break;
        }
        case INTERNAL_PEER:
        {
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "process sdp error with direction  %d for rtp session %s", direction, m_session_key->m_cookie);
            ret = -1;
            break;
        }
    }
    return ret;
}


