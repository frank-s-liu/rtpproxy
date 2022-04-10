#ifndef __RTP_SESSION_H__
#define __RTP_SESSION_H__

#include "sessionKey.h"
#include "sdp.h"
#include "udpSocket.h"


class RtpStream
{
public:
    RtpStream();
    virtual ~RtpStream();
    int set_local_rtp_network(const char* ip, int net_type, RTPDirection direction);
    int set_remote_peer_rtp_network(Network_address* remote_perr_addr);
    int send(const unsigned char* buf, int len);
    int recv(unsigned char* buf, int len);
private:
    Network_address     m_addr_peer; // peer address
    UdpSocket*          m_socket;  // local address
};

class RtpSession
{
public:
    RtpSession();
    RtpSession(SessionKey* key);
    virtual ~RtpSession();
    int processSdp(Sdp_session* sdp, RTPDirection direction);

public:
    SessionKey*               m_session_key;
private:
    RtpStream*                m_external;
    RtpStream*                m_internal;
};

#endif
