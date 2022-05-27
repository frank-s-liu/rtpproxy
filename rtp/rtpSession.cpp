#include "rtpsession.h"
#include "log.h"
#include "sdp.h"
#include "rtpSendRecvProcs.h"
#include "args.h"
#include "rtpControlProcess.h"
#include "rtpConstStr.h"

RtpSession::RtpSession()
{
    m_external = NULL;
    m_internal = NULL;
    m_session_key = NULL;
    m_rtp_sendrecv_process = NULL;
    m_rtpstreams[0] = NULL;
    m_rtpstreams[1] = NULL;
}

RtpSession::RtpSession(SessionKey* key, RtpProcess* process)
{
    m_external = NULL;
    m_internal = NULL;
    m_session_key = key;
    m_rtpstreams[0] = NULL;
    m_rtpstreams[1] = NULL;
    m_rtp_sendrecv_process = process;
    process->putRtpSession(this);
}

RtpSession::~RtpSession()
{
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
    m_rtpstreams[0] = NULL;
    m_rtpstreams[1] = NULL;
    if(m_session_key)
    {
        delete m_session_key;
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
                m_external = new RtpStream(this, EXTERNAL_PEER);
                m_internal = new RtpStream(this, INTERNAL_PEER);;
                m_rtpstreams[EXTERNAL_PEER] = m_internal;
                m_rtpstreams[INTERNAL_PEER] = m_external;
                m_external->set_local_rtp_network("10.100.126.230", IPV4);
                m_external->set_remote_peer_rtp_network(sdp->m_con.address.address.s, peer_port);
                m_external->chooseCrypto2Local(sdp, AEAD_AES_256_GCM);

                m_internal->set_local_rtp_network("10.100.125.147", IPV4);
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
                m_internal->saveLocalSdpStr(sdp);
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
            }
            
            SDPRespArgs* arg = new SDPRespArgs(m_session_key->m_cookie, m_session_key->m_cookie_len);
            arg->sdp = sdp;
            sdp = NULL;
            arg->direction = direction;
            if(0 != ControlProcess::getInstance()->add_pipe_event(arg))
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"send SDPRespArgs to ControlProcess failed for cmd session %s", m_session_key->m_cookie);
                delete arg;
            }
            else
            {
                tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__,"send SDPRespArgs to ControlProcess for session%s", m_session_key->m_cookie);
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
                m_internal = new RtpStream(this, INTERNAL_PEER);
                m_external = new RtpStream(this, EXTERNAL_PEER);
                m_rtpstreams[EXTERNAL_PEER] = m_internal;
                m_rtpstreams[INTERNAL_PEER] = m_external;
                m_internal->set_local_rtp_network("10.100.125.147", IPV4);
                m_internal->set_remote_peer_rtp_network(sdp->m_con.address.address.s, peer_port);
                m_internal->produceLocalInternalSdp(sdp);
                m_external->set_local_rtp_network("10.100.126.230", IPV4);
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
    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"process sdp error from %s", g_RTPDirection_str[direction]);
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

