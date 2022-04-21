#include <time.h>

#include "rtpsession.h"
#include "rtpSendRecvProcs.h"
#include "log.h"

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
    m_direction = MAX_DIRECTION;
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
}

int RtpStream::processCrypto(Sdp_session* remote_sdp)
{
// prefer to using AEAD_AES_256_GCM
// need to add the prefer suit chip into configuratiopn
// TBD
//
    int ret = 0;
    unsigned short local_rtp_port = 0;
    char local_internal_address[64];
    local_internal_address[0] = '\0';
    Attr_crypto* a = remote_sdp->getcryptoAttrFromAudioMedia(AEAD_AES_256_GCM);
    if(!a)
    {
        //a = remote_sdp->getLittleTagcryptoAttrFromAudioMedia();
        return -1;
    }
    if(m_remote_cry_cxt)
    {
        delete m_remote_cry_cxt;
    }
    m_remote_cry_cxt = new Crypto_context(AEAD_AES_256_GCM);
    m_remote_cry_cxt->set_crypto_param((Attr_crypto*)a);

    m_local_cry_cxt = new Crypto_context(AEAD_AES_256_GCM);
    randomString(m_local_cry_cxt->m_params.master_key,  m_local_cry_cxt->m_params.crypto_suite->master_key_len);
    randomString(m_local_cry_cxt->m_params.master_salt, m_local_cry_cxt->m_params.crypto_suite->master_salt_len);
    m_local_cry_cxt->m_params.mki = NULL;
    m_local_cry_cxt->m_params.mki_len = 0;
  
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
    return 0;
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

