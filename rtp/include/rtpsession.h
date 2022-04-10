#ifndef __RTP_SESSION_H__
#define __RTP_SESSION_H__

#include "sessionKey.h"
#include "sdp.h"
#include "udpSocket.h"


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
private:
    Network_address     m_addr_peer; // peer address
    UdpSocket*          m_socket;  // local address
    RtpSession*         m_rtpSession;
    RTPDirection        m_direction;
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
private:
    RtpStream*                m_external;
    RtpStream*                m_internal;
    RtpProcess*               m_rtp_sendrecv_process;
};

#endif
