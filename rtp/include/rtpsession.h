#ifndef __RTP_SESSION_H__
#define __RTP_SESSION_H__

#include "sessionKey.h"
#include "sdp.h"
#include "udpSocket.h"
#include "crypto.h"


class RtpSession;
class RtpProcess;
class RtpStream
{
public:
    RtpStream(RtpSession* rtpsession);
    virtual ~RtpStream();
    int set_local_rtp_network(const char* ip, int net_type, RTPDirection direction);
    int set_remote_peer_rtp_network(Network_address* remote_perr_addr);
    int send(const unsigned char* buf, int len);
    int recv(unsigned char* buf, int len);
    unsigned short getLocalPort();
    int getLocalAddress(char* buf, int buflen);
    int chooseCrypto2Local(Sdp_session* remote_sdp, Crypto_Suite chiper);
    int addCrypto2External(Sdp_session* sdp, Crypto_Suite chiper);
    int produceLocalInternalSdp(Sdp_session* sdp);
public:
    cstr                        m_local_sdp;
private:
    Network_address             m_addr_peer; // peer address
    UdpSrvSocket*               m_socket;  // local address
    RtpSession*                 m_rtpSession;
    Crypto_context*             m_remote_cry_cxt; // external using encryption
    Crypto_context*             m_local_cry_cxt; // external using encryption
    RTPDirection                m_direction;
    unsigned char               m_bridged;
};

class RtpSession
{
public:
    RtpSession();
    RtpSession(SessionKey* key, RtpProcess* process);
    virtual ~RtpSession();
    int processSdp(Sdp_session* sdp, RTPDirection direction);

public:
    SessionKey*               m_session_key;
    RtpProcess*               m_rtp_sendrecv_process;
private:
    RtpStream*                m_external;
    RtpStream*                m_internal;
};

#endif
