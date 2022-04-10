#include "rtpsession.h"
#include "log.h"
#include "sdp.h"



RtpSession::RtpSession()
{
}

RtpSession::RtpSession(SessionKey* key)
{
    m_session_key = key;
}

RtpSession::~RtpSession()
{
    if(m_session_key)
    {
        delete m_session_key;
    }
}

int RtpSession::processSdp(Sdp_session* sdp, RTPDirection direction)
{
    int ret = 0;
    switch (direction)
    {
        case EXTERNAL_PEER:
        {
            if(!m_external)
            {
                m_external = new RtpStream();
                m_external->set_local_rtp_network("a", IPV4, direction);
            }
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


