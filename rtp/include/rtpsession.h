#ifndef __RTP_SESSION_H__
#define __RTP_SESSION_H__

#include "sessionKey.h"
#include "sdp.h"
#include "udpSocket.h"
#include "crypto.h"
#include "rtpepoll.h"
#include "rtpHeader.h"

class RtpSession;
class RtpProcess;
class RtpStream
{
public:
    RtpStream(RtpSession* rtpsession);
    virtual ~RtpStream();
    int set_local_rtp_network(const char* ip, int net_type, RTPDirection direction);
    int set_remote_peer_rtp_network(const char* ip, unsigned short port);
    int send(const unsigned char* buf, int len);
    int recv(unsigned char* buf, int len);
    unsigned short getLocalPort();
    int getLocalAddress(char* buf, int buflen);
    int chooseCrypto2Local(Sdp_session* remote_sdp, Crypto_Suite chiper);
    int addCrypto2External(Sdp_session* sdp, Crypto_Suite chiper);
    int checkAndSetRemoteCrypto(Sdp_session* sdp);
    int produceLocalInternalSdp(Sdp_session* sdp);
    int readAndProcess();
    int writeProcess(cstr rtp);
private:
    int processExternalRtp();
    int processInternalRtp();
public:
    cstr                        m_local_sdp;
private:
    struct sockaddr_in          m_addr_peer; // peer address
    UdpSrvSocket*               m_socket;  // local address
    RtpSession*                 m_rtpSession;
    Crypto_context*             m_remote_cry_cxt; // external using encryption
    Crypto_context*             m_local_cry_cxt; // external using encryption
    RTP_send_recv_epoll_data*   m_data;
    RTPDirection                m_direction;
    unsigned short              m_local_crypto_tag; // to check crypto tag sdp answer from external remote
    Crypto_Suite                m_local_crypto_chiper;  // to check crypto-suit in sdp from external remote
    unsigned char               m_bridged;
    struct SSRC_CTX             m_peer_ssrc_ctx;
    struct SSRC_CTX             m_local_ssrc_ctx;
};

class RtpSession
{
public:
    RtpSession();
    RtpSession(SessionKey* key, RtpProcess* process);
    virtual ~RtpSession();
    int processSdp(Sdp_session* sdp, RTPDirection direction);
    int get_other_rtp_streams(RtpStream* from, RtpStream** to);
public:
    SessionKey*               m_session_key;
    RtpProcess*               m_rtp_sendrecv_process;
private:
    RtpStream*                m_external;
    RtpStream*                m_internal;
};

#endif
