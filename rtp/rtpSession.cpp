#include "rtpsession.h"
#include "log.h"
#include "sdp.h"
#include "rtpSendRecvProcs.h"
#include "args.h"
#include "rtpControlProcess.h"


RtpSession::RtpSession()
{
    m_external = NULL;
    m_internal = NULL;
    m_session_key = NULL;
    m_rtp_sendrecv_process = NULL;
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
    if(m_external)
    {
        delete m_external;
        m_external = NULL;
    }
    if(m_internal)
    {
        delete m_internal;
        m_internal = NULL;
    }
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
            unsigned short local_rtp_port = 0;
            if((!m_internal && m_external) ||(m_internal && !m_external))
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "m_internal and m_external must be both NULL all both not NULL im cmd session %s", 
                                                                  m_session_key->m_cookie);
                ret = -1;
                break;
            }
            if(!m_external)
            {
                m_external = new RtpStream(this);
            }
            m_external->set_local_rtp_network("10.100.126.230", IPV4, direction);
            m_external->set_remote_peer_rtp_network(&sdp->m_con.address);
            if(!m_internal)
            {
                m_internal = new RtpStream(this);;
            }
            m_internal->set_local_rtp_network("10.100.125.147", IPV4, INTERNAL_PEER);
            sdp->replaceOrigin("10.100.125.147", strlen("10.100.125.147"));
            sdp->replaceCon("10.100.125.147", strlen("10.100.125.147"));
            local_rtp_port = m_internal->getLocalPort();
            ret = sdp->replaceMedia(local_rtp_port, RTP_AVP);
            SDPRespArgs* arg = new SDPRespArgs(m_session_key->m_cookie, m_session_key->m_cookie_len);
            arg->sdp = sdp;
            arg->direction = EXTERNAL_PEER;
            ControlProcess::getInstance()->add_pipe_event(arg);
            break;
        }
        case INTERNAL_PEER:
        {
            unsigned short local_rtp_port = 0;
            if((!m_internal && m_external) ||(m_internal && !m_external))
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "m_internal and m_external must be both NULL all both not NULL im cmd session %s", 
                                                                  m_session_key->m_cookie);
                ret = -1;
                break;
            }
            if(!m_internal) // both NULL
            {
                m_internal = new RtpStream(this);
                m_external = new RtpStream(this);;
                m_internal->set_local_rtp_network("10.100.125.147", IPV4, direction);
                m_internal->set_remote_peer_rtp_network(&sdp->m_con.address);
                m_external->set_local_rtp_network("10.100.126.230", IPV4, INTERNAL_PEER);
            }
            else
            {
                m_internal->set_remote_peer_rtp_network(&sdp->m_con.address);

            }
            sdp->replaceOrigin("10.100.126.230", strlen("10.100.126.230"));
            sdp->replaceCon("10.100.126.230", strlen("10.100.126.230"));
            local_rtp_port = m_external->getLocalPort();
            ret = sdp->replaceMedia(local_rtp_port, RTP_SAVP);
            SDPRespArgs* arg = new SDPRespArgs(m_session_key->m_cookie, m_session_key->m_cookie_len);
            arg->sdp = sdp;
            arg->direction = INTERNAL_PEER;
            ControlProcess::getInstance()->add_pipe_event(arg);
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


