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
    m_external = NULL;
    m_internal = NULL;
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
    unsigned short peer_port = 0;
    if(!sdp || 0 == sdp->m_parsed)
    {
        ret = -1;
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"sdp is null or sdp is not parsed");
        goto errorProcess;
    }
    sdp->getAudioMediaPort(&peer_port);
    switch (direction)
    {
        case EXTERNAL_PEER:
        {
            unsigned short local_rtp_port = 0;
            char local_internal_address[64];
            local_internal_address[0] = '\0';
            if((!m_internal && m_external) ||(m_internal && !m_external))
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "m_internal and m_external must be both NULL all both not NULL im cmd session %s", 
                                                                  m_session_key->m_cookie);
                ret = -1;
                goto errorProcess;
            }
            if(!m_external) // both NULL
            {
                m_external = new RtpStream(this);
                m_internal = new RtpStream(this);;
                m_external->set_local_rtp_network("10.100.126.230", IPV4, direction);
                m_external->set_remote_peer_rtp_network(sdp->m_con.address.address.s, peer_port);
                m_external->chooseCrypto2Local(sdp, AEAD_AES_256_GCM);

                m_internal->set_local_rtp_network("10.100.125.147", IPV4, INTERNAL_PEER);
                m_internal->getLocalAddress(local_internal_address, sizeof(local_internal_address));
                local_rtp_port = m_internal->getLocalPort();
                sdp->replaceOrigin(local_internal_address, strlen(local_internal_address));
                sdp->replaceCon(local_internal_address, strlen(local_internal_address));
                ret = sdp->replaceMedia(local_rtp_port, RTP_AVP);
                if(ret != 0)
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "cmd session %s replace media line error",
                                                                      m_session_key->m_cookie);
                    goto errorProcess;
                }
                ret = sdp->removeCryptoAttr();
            }
            else
            {
                m_external->set_remote_peer_rtp_network(sdp->m_con.address.address.s, peer_port);
                ret = m_external->checkAndSetRemoteCrypto(sdp);
                if(ret != 0)
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "cmd session %s replace media line error",
                                                                      m_session_key->m_cookie);
                    goto errorProcess;
                }
                sdp->destroySdp();
                sdp->m_sdp_str.len = m_internal->m_local_sdp.len;
                sdp->m_sdp_str.s = m_internal->m_local_sdp.s;
                m_internal->m_local_sdp.len = 0;
                m_internal->m_local_sdp.s = NULL;
            }
            
            SDPRespArgs* arg = new SDPRespArgs(m_session_key->m_cookie, m_session_key->m_cookie_len);
            arg->sdp = sdp;
            sdp = NULL;
            arg->direction = direction;
            if(0 != ControlProcess::getInstance()->add_pipe_event(arg))
            {
                delete arg;
            }
            break;
        }
        case INTERNAL_PEER:
        {
            unsigned short local_rtp_port = 0;
            char local_external_address[64];
            local_external_address[0] = '\0';
            if((!m_internal && m_external) ||(m_internal && !m_external))
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "m_internal and m_external must be both NULL all both not NULL im cmd session %s", 
                                                                  m_session_key->m_cookie);
                ret = -1;
                goto errorProcess;
            }
            if(!m_internal) // both NULL
            {
                m_internal = new RtpStream(this);
                m_external = new RtpStream(this);;
                m_internal->set_local_rtp_network("10.100.125.147", IPV4, direction);
                m_internal->set_remote_peer_rtp_network(sdp->m_con.address.address.s, peer_port);
                m_internal->produceLocalInternalSdp(sdp);
                m_external->set_local_rtp_network("10.100.126.230", IPV4, INTERNAL_PEER);
                m_external->getLocalAddress(local_external_address, sizeof(local_external_address));

                local_rtp_port = m_external->getLocalPort();
                sdp->replaceOrigin(local_external_address, strlen(local_external_address));
                sdp->replaceCon(local_external_address, strlen(local_external_address));
                ret = sdp->replaceMedia(local_rtp_port, RTP_SAVP);
                m_external->addCrypto2External(sdp, AEAD_AES_256_GCM);
            }
            else
            {
                m_internal->set_remote_peer_rtp_network(sdp->m_con.address.address.s, peer_port);
                sdp->destroySdp();
                sdp->m_sdp_str.len = m_external->m_local_sdp.len;
                sdp->m_sdp_str.s = m_external->m_local_sdp.s;
                m_external->m_local_sdp.len = 0;
                m_external->m_local_sdp.s = NULL;
            }

            SDPRespArgs* arg = new SDPRespArgs(m_session_key->m_cookie, m_session_key->m_cookie_len);
            arg->sdp = sdp;
            sdp = NULL;
            arg->direction = INTERNAL_PEER;
            if(0 != ControlProcess::getInstance()->add_pipe_event(arg))
            {
                tracelog("RTP", ERROR_LOG, __FILE__, __LINE__,"pipe issue");
                delete arg;
            }
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "process sdp error with direction  %d for rtp session %s", direction, m_session_key->m_cookie);
            ret = -1;
            goto errorProcess;
        }
    }
    return ret;

errorProcess:
    if(sdp)
    {
        delete sdp;
        sdp = NULL;
    }
    SDPRespArgs* arg = new SDPRespArgs(m_session_key->m_cookie, m_session_key->m_cookie_len);
    arg->sdp = NULL;
    arg->direction = direction;
    arg->result = -1;
    if(0 != ControlProcess::getInstance()->add_pipe_event(arg))
    {
        delete arg;
    }
    return ret;
}

int RtpSession::get_other_rtp_streams(RtpStream* from, RtpStream** to)
{
    if(from == m_internal)
    {
        *to = m_external;
    }
    else if(from == m_external)
    {
        *to = m_internal;
    }
    else
    {
        *to = NULL;
        return -1;
    }
    return 0;
}
