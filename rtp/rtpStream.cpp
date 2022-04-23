#include <time.h>

#include "rtpsession.h"
#include "rtpSendRecvProcs.h"
#include "log.h"
#include "base64.h"
#include "rtpConstStr.h"

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
    for(index=0; index<buf_size; buf_size++)
    {
        int i = rand()%sizeof(randomChar);
        buf[index] = randomChar[i];
    }
}

RtpStream::RtpStream(RtpSession* rtp_session)
{
    m_socket = NULL;
    m_rtpSession = rtp_session;
    m_bridged = 0;
    m_remote_cry_cxt = NULL;
    m_local_cry_cxt = NULL;
    m_local_sdp.s = NULL;
    m_local_sdp.len = 0;
    m_direction = MAX_DIRECTION;
    m_local_crypto_tag = 0;
    m_local_crypto_chiper = AEAD_AES_256_GCM;
}

RtpStream::~RtpStream()
{
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
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s no chip suit %s",
                                                          m_rtpSession->m_session_key->m_cookie,
                                                          s_crypto_suite_str[chiper]);
        return -1;
    }
    if(m_remote_cry_cxt)
    {
        delete m_remote_cry_cxt;
    }
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
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s has local external sdp str",
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

int RtpStream::checkAndSetRemoteCrypto(Sdp_session* remote_sdp)
{
    Attr_crypto* a = remote_sdp->getcryptoAttrFromAudioMedia(m_local_crypto_chiper);
    if(!a)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s no  chip suit %s",
                                                          m_rtpSession->m_session_key->m_cookie, 
                                                          s_crypto_suite_str[m_local_crypto_chiper]);
        return -1;
    }
    if(a->tag != m_local_crypto_tag)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s chip suit %s has wrong tag",
                                                          m_rtpSession->m_session_key->m_cookie, 
                                                          s_crypto_suite_str[m_local_crypto_chiper]);
        return -1;
    }
    if(m_remote_cry_cxt)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp session %s has rmt_cry_cxt", m_rtpSession->m_session_key->m_cookie);
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
        int len = snprintf(a->suite_str.s, MAX_CRYPTO_SUIT_STR_LEN, "%s", s_crypto_suite_str[chiper]);
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

int RtpStream::set_local_rtp_network(const char* local_ip, int type, RTPDirection direction)
{
    if(type == IPV4)
    {
        int epoll_fd = m_rtpSession->m_rtp_sendrecv_process->getEpoll_fd();
        if(m_socket)
        {
            delete m_socket;
        }
        m_socket = new UdpSrvSocket(local_ip, direction);
        if(0 == m_socket->getStatus())
        {
            return -1;
        }
        m_direction = direction;
        m_socket->add_read_event2EpollLoop(epoll_fd, this);
        return 0;
    }
    return -1;
}

int RtpStream::set_remote_peer_rtp_network(Network_address* remote_perr_addr)
{
    if(remote_perr_addr)
    {
        m_addr_peer = *remote_perr_addr;
        return 0;
    }
    return -1;
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

