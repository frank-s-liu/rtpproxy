#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "rtpsession.h"
#include "rtpSendRecvProcs.h"
#include "log.h"
#include "base64.h"
#include "rtpConstStr.h"
#include "rtpHeader.h"

static unsigned char randomChar[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                             'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F',
                             'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                             'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '@',
                             '#', '$', '+', '-', '%', '=', '/', '*', ':', ';', '?', ',', '<', '>', '{', '}'
                           };

static void randomString(unsigned char* buf, int buf_size)
{
    int index = 0;
    srand((unsigned int)time(NULL));
    for(index=0; index<buf_size; index++)
    {
        int i = rand()%sizeof(randomChar);
        buf[index] = randomChar[i];
    }
}

RtpStream::RtpStream(RtpSession* rtp_session, RTPDirection direction)
{
    m_socket = NULL;
    m_rtpSession = rtp_session;
    m_bridged = 0;
    m_remote_cry_cxt = NULL;
    m_local_cry_cxt = NULL;
    m_local_sdp.s = NULL;
    m_local_sdp.len = 0;
    m_direction = direction;
    m_local_crypto_tag = 0;
    m_local_crypto_chiper = AEAD_AES_256_GCM;
    m_data = NULL;
    m_addr_peer_ip = NULL;
    m_addr_peer_port = 0;
    memset(&m_peer_ssrc_ctx, 0, sizeof(m_peer_ssrc_ctx));
    memset(&m_local_ssrc_ctx, 0, sizeof(m_local_ssrc_ctx));
    tracelog("RTP", INFO_LOG, __FILE__, __LINE__,"create [%s] RtpStream instance for session [%s]", g_RTPDirection_str[m_direction], m_rtpSession->m_session_key->m_cookie);
}

RtpStream::~RtpStream()
{
    tracelog("RTP", INFO_LOG, __FILE__, __LINE__,"destroy [%s] RtpStream instance for session [%s]", g_RTPDirection_str[m_direction], m_rtpSession->m_session_key->m_cookie);
    if(m_socket)
    {
        delete m_socket;
        m_socket = NULL;
    }
    m_rtpSession = NULL;
    if(m_remote_cry_cxt)
    {
        delete m_remote_cry_cxt;
    }
    if(m_local_cry_cxt)
    {
        delete m_local_cry_cxt;
    }
    if(m_local_sdp.len > 0)
    {
        delete[] m_local_sdp.s;
    }
    if(m_data)
    {
        delete m_data;
    }
    if(m_addr_peer_ip)
    {
        delete[] m_addr_peer_ip;
        m_addr_peer_ip = NULL;
    }
}

int RtpStream::produceLocalInternalSdp(Sdp_session* remote_sdp)
{
    int ret = 0;
    unsigned short local_rtp_port = 0;
    char local_internal_address[64];
    local_internal_address[0] = '\0';
    getLocalAddress(local_internal_address, sizeof(local_internal_address));
    local_rtp_port = getLocalPort();
    remote_sdp->replaceOrigin(local_internal_address, strlen(local_internal_address));
    remote_sdp->replaceCon(local_internal_address, strlen(local_internal_address));
    ret = remote_sdp->replaceMedia(local_rtp_port, RTP_AVP);
    if(ret != 0)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s replace media line error",
                                                          m_rtpSession->m_session_key->m_cookie);
    }
    ret = remote_sdp->removeCryptoAttr();
    if(m_local_sdp.len >0)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s has local internal sdp str",
                                                           m_rtpSession->m_session_key->m_cookie);
        return -1;
    }
    m_local_sdp.s = new char[MAX_SDP_LEN];
    m_local_sdp.len = MAX_SDP_LEN;
    ret = remote_sdp->serialize(m_local_sdp.s, &m_local_sdp.len);
    if(ret != 0)
    {
        delete[] m_local_sdp.s;
        m_local_sdp.s = NULL;
        m_local_sdp.len = 0;
        return -1;
    }
    return 0;
}

int RtpStream::chooseCrypto2Local(Sdp_session* remote_sdp, Crypto_Suite chiper)
{
// prefer to using AEAD_AES_256_GCM
// need to add the prefer suit chip into configuratiopn
// TBD
//
    int ret = 0;
    unsigned short local_rtp_port = 0;
    char local_internal_address[64];
    local_internal_address[0] = '\0';
    Attr_crypto* a = remote_sdp->getcryptoAttrFromAudioMedia(chiper);
    unsigned char base64key[128];
    unsigned char srckey[64];
    unsigned int len = 0;
    if(!a)
    {
        //a = remote_sdp->getLittleTagcryptoAttrFromAudioMedia();
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session [%s] no chip suit %s",
                                                          m_rtpSession->m_session_key->m_cookie,
                                                          g_crypto_suite_str[chiper]);
        return -1;
    }
    if(m_remote_cry_cxt)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"already has one m_remote_cry_cxt for session %s", m_rtpSession->m_session_key->m_cookie);
        delete m_remote_cry_cxt;
    }
    tracelog("RTP", INFO_LOG, __FILE__, __LINE__,"create [%s] m_remote_cry_cxt for session [%s]", g_RTPDirection_str[m_direction], m_rtpSession->m_session_key->m_cookie);
    m_local_crypto_tag = a->tag;
    m_remote_cry_cxt = new Crypto_context(chiper);
    m_remote_cry_cxt->set_crypto_param((Attr_crypto*)a);

    m_local_cry_cxt = new Crypto_context(chiper);
    randomString(m_local_cry_cxt->m_params.master_key,  m_local_cry_cxt->m_params.crypto_suite->master_key_len);
    randomString(m_local_cry_cxt->m_params.master_salt, m_local_cry_cxt->m_params.crypto_suite->master_salt_len);
    len = snprintf((char*)srckey, sizeof(srckey), "%s%s",m_local_cry_cxt->m_params.master_key, m_local_cry_cxt->m_params.master_salt);
    if(len >= sizeof(srckey))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s produce random key error",
                                                          m_rtpSession->m_session_key->m_cookie);
        return -1;
    }
    base64Encode(srckey, len, base64key, sizeof(base64key));
    if(m_remote_cry_cxt->m_params.mki_len)
    {
        m_local_cry_cxt->m_params.mki = new unsigned char[m_remote_cry_cxt->m_params.mki_len+1];
        memcpy(m_local_cry_cxt->m_params.mki, m_remote_cry_cxt->m_params.mki, m_remote_cry_cxt->m_params.mki_len);
    }
    m_local_cry_cxt->m_params.mki_len = m_remote_cry_cxt->m_params.mki_len;
  
    getLocalAddress(local_internal_address, sizeof(local_internal_address));
    local_rtp_port = getLocalPort();
    remote_sdp->replaceOrigin(local_internal_address, strlen(local_internal_address));
    remote_sdp->replaceCon(local_internal_address, strlen(local_internal_address));
    ret = remote_sdp->replaceMedia(local_rtp_port, RTP_SAVP);
    if(ret != 0)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s replace media line error",
                                                          m_rtpSession->m_session_key->m_cookie);
    }
    ret = remote_sdp->removeCryptoAttrExclude(a->tag);
    a->replaceKeyParamter((char*)base64key, strlen((const char*)base64key));
    if(m_local_sdp.len >0)
    {
        delete[] m_local_sdp.s;
    }
    m_local_sdp.s = new char[MAX_SDP_LEN];
    m_local_sdp.len = MAX_SDP_LEN;
    ret = remote_sdp->serialize(m_local_sdp.s, &m_local_sdp.len);
    if(ret != 0)
    {
        delete[] m_local_sdp.s;
        m_local_sdp.s = NULL;
        m_local_sdp.len = 0;
        return -1;
    }
    tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__, "external local sdp [%s]", m_local_sdp.s);
    return 0;
}

int RtpStream::saveLocalSdpStr(Sdp_session* sdp)
{
    int ret = 0;
    if(m_local_sdp.len >0)
    {
        delete[] m_local_sdp.s;
    }
    m_local_sdp.s = new char[MAX_SDP_LEN];
    m_local_sdp.len = MAX_SDP_LEN;
    ret = sdp->serialize(m_local_sdp.s, &m_local_sdp.len);
    if(ret != 0)
    {
        delete[] m_local_sdp.s;
        m_local_sdp.s = NULL;
        m_local_sdp.len = 0;
        return -1;
    }
    return 0;
}

int RtpStream::checkAndSetRemoteCrypto(Sdp_session* remote_sdp)
{
    Attr_crypto* a = remote_sdp->getcryptoAttrFromAudioMedia(m_local_crypto_chiper);
    if(!a)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session [%s] no  chip suit %s",
                                                          m_rtpSession->m_session_key->m_cookie, 
                                                          g_crypto_suite_str[m_local_crypto_chiper]);
        return -1;
    }
    if(a->tag != m_local_crypto_tag)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session [%s] chip suit %s has wrong tag %d %d",
                                                          m_rtpSession->m_session_key->m_cookie, 
                                                          g_crypto_suite_str[m_local_crypto_chiper], a->tag, m_local_crypto_tag);
        return -1;
    }
    if(m_remote_cry_cxt)
    {
        tracelog("RTP", INFO_LOG, __FILE__, __LINE__, "rtp session %s has rmt_cry_cxt", m_rtpSession->m_session_key->m_cookie);
        delete m_remote_cry_cxt;
    }
    m_remote_cry_cxt = new Crypto_context(m_local_crypto_chiper);
    m_remote_cry_cxt->set_crypto_param((Attr_crypto*)a);
    return 0;
}

int RtpStream::addCrypto2External(Sdp_session* sdp, Crypto_Suite chiper)
{
    Attr_crypto* a = NULL;
    if(!m_local_cry_cxt)
    {
        unsigned char base64key[MAX_CRYPTO_SUIT_KEYSTR_LEN];
        unsigned char srckey[MAX_CRYPTO_SUIT_KEYSTR_LEN];
        a = new Attr_crypto();
        a->suite_str.s = new char[MAX_CRYPTO_SUIT_STR_LEN];
        int len = snprintf(a->suite_str.s, MAX_CRYPTO_SUIT_STR_LEN, "%s", g_crypto_suite_str[chiper]);
        a->suite_str.len = len;
        
        m_local_cry_cxt = new Crypto_context(chiper);
        randomString(m_local_cry_cxt->m_params.master_key,  m_local_cry_cxt->m_params.crypto_suite->master_key_len);
        randomString(m_local_cry_cxt->m_params.master_salt, m_local_cry_cxt->m_params.crypto_suite->master_salt_len);
        len = snprintf((char*)srckey, sizeof(srckey), "%s%s",m_local_cry_cxt->m_params.master_key, m_local_cry_cxt->m_params.master_salt);
        if(len >= (int)sizeof(srckey))
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s produce random key error",
                                                              m_rtpSession->m_session_key->m_cookie);
            goto errret;
        }
        base64Encode(srckey, len, base64key, sizeof(base64key));
        m_local_cry_cxt->m_params.mki_len = 0;
        a->tag = 1+chiper;
        a->key_params.s = new char[MAX_CRYPTO_SUIT_KEYSTR_LEN];
        len = snprintf(a->key_params.s, MAX_CRYPTO_SUIT_KEYSTR_LEN, "%s", base64key);
        a->key_params.len = len;
        a->parsed = 1;
        sdp->addCrypto2AudioMedia(a);
        m_local_crypto_tag = a->tag;
        m_local_crypto_chiper = chiper;
    }
    return 0;
errret:
    if(a)
    {
        delete a;
    }
    return -1;
}

unsigned short RtpStream::getLocalPort()
{
    return m_socket->getlocalPort();
}

int RtpStream::set_local_rtp_network(const char* local_ip, int type)
{
    if(type == IPV4)
    {
        m_data = new RTP_send_recv_epoll_data();
        m_data->m_epoll_fd_type = RTP_SEND_RECV_SOCKET_FD;
        m_data->m_data = this;
        int epoll_fd = m_rtpSession->m_rtp_sendrecv_process->getEpoll_fd();
        if(m_socket)
        {
            delete m_socket;
        }
        m_socket = new UdpSrvSocket(local_ip, m_direction);
        if(0 == m_socket->getStatus())
        {
            return -1;
        }
        m_socket->setnoblock();
        m_socket->add_read_event2EpollLoop(epoll_fd, m_data);
        return 0;
    }
    return -1;
}

int RtpStream::readAndProcess()
{
    char buf[2048];
    int ret = 0;
    uint32_t rtpIndex = 0;
    int recv_len = 0;
    Rtp_Fixed_header* rtpHdr = NULL;
    cstr payload;
    cstr rtp_raw;
    cstr to_auth;
    cstr pl_to_decrypt;
    cstr auth_tag;
    RtpStream* sendto = NULL;
    auth_tag.len = 0;
    auth_tag.s = NULL;
    pl_to_decrypt.len = 0;
    pl_to_decrypt.s = NULL;
    to_auth.len = 0;
    to_auth.s = NULL;
    rtp_raw.len = 0;
    rtp_raw.s = NULL;
    payload.len = 0;
    payload.s = NULL;
    recv_len = m_socket->recv_from(buf, sizeof(buf));
    m_rtpSession->get_other_rtp_streams(this, &sendto);
    if(-1 == recv_len)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session read err, errno:%d", errno);
        return -1;
    }
    rtp_raw.s = buf;
    rtp_raw.len = recv_len;
    ret = rtp_payload(&rtpHdr, &payload, &rtp_raw);
    if(0 != ret)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session read rtp with error format");
        return -1;
    }
    rtpIndex = packet_index(&m_peer_ssrc_ctx, rtpHdr);


    if(EXTERNAL_PEER == m_direction)
    {
        if(!m_remote_cry_cxt)
        {
            tracelog("RTP", INFO_LOG, __FILE__, __LINE__,"still not receive remote sdp for session[%s] [EXTERNAL] stream", m_rtpSession->m_session_key->m_cookie);
            return 0;
        }
        if (0 != srtp_payloads(&to_auth, &pl_to_decrypt, &auth_tag,
                            m_remote_cry_cxt->m_params.session_params.unauthenticated_srtp ? 0 : m_remote_cry_cxt->m_params.crypto_suite->srtp_auth_tag_len,
                            NULL, m_remote_cry_cxt->m_params.mki_len,
                            &rtp_raw, &payload))
        {
            return -1;    
        }
        // no authentication tag, decrypt srtp pl
        // AEAD_AES_256_GCM or AEAD_AES_128_GCM don't have authentication tag in srtp
        if (0 == auth_tag.len)
        {
            int prev_len = pl_to_decrypt.len;
            if(0 != m_remote_cry_cxt->m_params.crypto_suite->decrypt_rtp(m_remote_cry_cxt, rtpHdr, &pl_to_decrypt, rtpIndex))
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp stream decrypt_rtp error from remote peer address %s:%d", m_addr_peer_ip, m_addr_peer_port);
                return -1;
            }
            //tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__,"rtp stream decrypt_rtp success from remote peer address %s:%d", m_addr_peer_ip, m_addr_peer_port);
            rtp_raw.len -= (prev_len - pl_to_decrypt.len);
        }
        else
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp stream direction error, only support AEAD_AES_256_GCM");
            return -1;
        }
        goto sendrtp;
    }
    else if(INTERNAL_PEER == m_direction)
    {
        int prev_len = payload.len;
        if(!sendto || !sendto->m_local_cry_cxt)
        {
            return -1;
        }
        if(0 != sendto->m_local_cry_cxt->m_params.crypto_suite->encrypt_rtp(sendto->m_local_cry_cxt, rtpHdr, &payload, rtpIndex))
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp stream encrypt_rtp error from peer address %s:%d", m_addr_peer_ip, m_addr_peer_port);
            return -1;
        }
        rtp_raw.len += (payload.len - prev_len);
        // MKI and authentication tag
        goto sendrtp;
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp stream direction error %d", m_direction);
        return -1;
    }
    return 0;

    
sendrtp:
    //
    if(sendto)
    {
        int len = sendto->writeProcess(rtp_raw);
        if(len <= 0)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s send out rtp failed ", m_rtpSession->m_session_key->m_cookie);
        }
    }
    return 0;
}

int RtpStream::writeProcess(cstr rtp)
{
    struct sockaddr_in addr_peer;
    addr_peer.sin_family = AF_INET;
    addr_peer.sin_port = htons(m_addr_peer_port);
    addr_peer.sin_addr.s_addr = inet_addr(m_addr_peer_ip);
    return m_socket->send_to(rtp.s, rtp.len, 0, (struct sockaddr* )&addr_peer, sizeof(struct sockaddr_in));
}

static const int IP_MAX_LEN = 64;

int RtpStream::set_remote_peer_rtp_network(const char* ip, unsigned short port)
{
    if(!m_addr_peer_ip)
    {
        m_addr_peer_ip = new char[IP_MAX_LEN];
    }
    snprintf(m_addr_peer_ip, IP_MAX_LEN, "%s", ip);
    m_addr_peer_port = port;
    return 0;
}

int RtpStream::send(const unsigned char* buf, int len)
{
    return 0;
}

int RtpStream::recv(unsigned char* buf, int len)
{
    return 0;
}

int RtpStream::getLocalAddress(char* buf, int buflen)
{
    return m_socket->getLocalIp(buf, buflen);
}

